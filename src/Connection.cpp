
#include "Connection.h"

namespace livechange {

  void Observation::addReactions(std::shared_ptr<Observable> observable) {
    auto disposeHandler = std::make_shared<std::function<void()>>(
        [observable, this]{
          this->removeObservable(observable);
        }
    );
    observable->onDispose.push_back(disposeHandler);
    auto respawnHandler = std::make_shared<std::function<void()>>(
        [observable, this]{
          this->addObservable(observable);
        }
    );
    observable->onDispose.push_back(respawnHandler);
  }

  void Observation::addObservable(std::shared_ptr<Observable> observable) {
    std::lock_guard<std::mutex> guard(stateMutex);
    observables.push_back(observable);
    if (observables.size() == 1 && connection->isConnected()) {
      nlohmann::json msg = {
          { "type", "observe" },
          { "what", path },
          { "pushed", false }
      };
      connection->send(msg);
    }
    Observer observer = observable->observer;
    for (auto signal : cachedSignals) {
      (*observer)(signal["signal"], signal["args"]);
    }
  }

  void Observation::handleDisconnect() {
  }

  void Observation::handleConnect() {
    cachedSignals.clear();
    if(observables.size() > 0) {
      nlohmann::json msg = {
          { "type", "observe" },
          { "what", path },
          { "pushed", false }
      };
      connection->send(msg);
    }
  }
  void Observation::handleNotifyMessage(const nlohmann::json& message) {
    this->cachedSignals.push_back(message);
    for(auto observable : observables) {
      Observer observer = observable->observer;
      (*observer)(message["signal"], message["args"]);
    }
  }

  void Observation::removeObservable(std::shared_ptr<Observable> observable) {
    std::lock_guard<std::mutex> guard(stateMutex);
    observables.erase(std::remove_if(observables.begin(), observables.end(),
                                   [&observable](auto o) { return o == observable; } ));
    if (observables.size() == 0) {
      if(connection->isConnected()) {
        nlohmann::json msg = {
            {"type",   "unobserve"},
            {"what",   path},
            {"pushed", false}
        };
        connection->send(msg.dump());
      }
      cachedSignals.clear();
      connection->observations.erase(path); // TODO: analyze if this can lead to observation duplication
    }
  }

  Request::Request(std::shared_ptr<Connection> connectionp, int requestIdp,
                   nlohmann::json msgp, RequestSettings settingsp)
                   : connection(connectionp), requestId(requestIdp),
                   message(msgp), settings(settingsp) {
    message["requestId"] = requestId;
    startPoint = std::chrono::steady_clock::now();
    timeoutPoint = startPoint + settings.timeout;
  }
  void Request::handleMessage(const nlohmann::json message) {
    if(message["type"] == "error") {
      resultPromise.reject(std::make_exception_ptr(RemoteError(message["error"])));
    } else {
      resultPromise.resolve(message["response"])
    }
  }
  void Request::handleDisconnect() {
    std::lock_guard<std::mutex> guard(stateMutex);
    if(settings.queueWhenDisconnected) {
      auto sentTimeout = std::chrono::steady_clock::now() + settings.sentTimeout;
      auto timeout = startPoint + settings.timeout;
      timeoutPoint = sentTimeout < timeout ? sentTimeout : timeout;
      std::shared_ptr<Connection> ptr = connection.lock();
      if(ptr) {
        ptr->requestsQueue.push_back(shared_from_this());
      }
    } else {
      resultPromise.reject(std::make_exception_ptr(DisconnectError()));
    }
  }
  void Request::handleTimeout() {
    resultPromise.reject(std::make_exception_ptr(TimeoutError()));
  }

  Connection::Connection(std::string urlp, nlohmann::json sessionIdp) : url(urlp), sessionId(sessionIdp),
    lastRequestId(0), connectedCounter(0) {
  }
  Connection::~Connection() {

  }
  void Connection::init() {
    std::lock_guard<std::mutex> guard(stateMutex);
    std::weak_ptr self = shared_from_this();
    timeoutThread = std::thread([self](){
      while(true) {
        std::chrono::steady_clock::time_point next_timeout;
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        bool nextFound = false;
        {
          std::shared_ptr<Connection> ptr = self.lock();
          if(!ptr) break;
          std::unique_lock<std::mutex> guard(ptr->stateMutex);
          // Run timeouts:
          for(int i = 0; i < ptr->waitingRequests.size(); i++) {
            auto request = ptr->waitingRequests[i];
            if(request->hasTimeout && request->timeoutPoint < now) {
              request->handleTimeout();
              ptr->waitingRequests.erase(ptr->waitingRequests.begin() + i);
              i--;
            }
          }
          for(int i = 0; i < ptr->requestsQueue.size(); i++) {
            auto request = ptr->requestsQueue[i];
            if(request->hasTimeout && request->timeoutPoint < now) {
              request->handleTimeout();
              ptr->requestsQueue.erase(ptr->requestsQueue.begin() + i);
              i--;
            }
          }
          // Find next timeout:
          for(auto request : ptr->waitingRequests) {
            if(request->hasTimeout && !nextFound || request->timeoutPoint < next_timeout) {
              nextFound = true;
              next_timeout = request->timeoutPoint;
            }
          }
          for(auto request : ptr->requestsQueue) {
            if(request->hasTimeout && !nextFound || request->timeoutPoint < next_timeout) {
              nextFound = true;
              next_timeout = request->timeoutPoint;
            }
          }
          if(!nextFound) {
            ptr->timeoutCondition.wait(guard);
          } else {
            ptr->timeoutCondition.wait_until(guard, next_timeout);
          }
        }
      }
    });
  }

  void Connection::send(const nlohmann::json& msg) {
    webSocket->send(msg.dump(), wsxx::WebSocket::PacketType::Text);
  }

  promise::Promise<nlohmann::json> Connection::sendRequest(
      const nlohmann::json& msg, RequestSettings settings) {
    std::lock_guard<std::mutex> guard(stateMutex);

    auto request = std::make_shared<Request>(shared_from_this(), ++lastRequestId, msg, settings);
    if(isConnected()) {
      waitingRequests.push_back(request);
    } else {
      requestsQueue.push_back(request);
    }
    send(request->message);
    timeoutCondition.notify_one();
  }

  void Connection::handleOpen() {
    std::lock_guard<std::mutex> guard(stateMutex);
    connectedCounter++;
    send({
      { "type", "initializeSession" },
      { "sessionId", sessionId }
    });
    for(auto pair : observations) {
      pair.second->handleConnect();
    }
    for(auto request : requestsQueue) {
      this->waitingRequests.push_back(request);
      send(request->message);
    }
    requestsQueue.clear();
  }
  void Connection::handleMessage(std::string data, wsxx::WebSocket::PacketType type) {
    std::lock_guard<std::mutex> guard(stateMutex);

    if(type == wsxx::WebSocket::PacketType::Text) {
      auto msg = nlohmann::json::parse(data);
      std::string type = msg["type"];
      if(type == "pong") {

      } else if(type == "ping") {
        msg["type"] = "pong";
        send(msg);
      } else if(type == "authenticationError") {
        // TODO: signal error
        this->webSocket->closeConnection();
      } else if(msg.contains("responseId")) {
        int responseId = msg["responseId"];
        for(int i = 0; i < waitingRequests.size(); i++) {
          auto request = waitingRequests[i];
          if(request->requestId == responseId) {
            request->handleMessage(msg);
            waitingRequests.erase(waitingRequests.begin() + i);
            timeoutCondition.notify_one();
            break;
          }
        }
      } else if(type == "notify") {
        auto it = observations.find(msg["what"]);
        if(it != observations.end()) {
          it->second->handleNotifyMessage(msg);
        }
      //} else if(type == "push") {
      //} else if(type == "unpush") {
      } else {
        throw std::runtime_error(std::string("unknown message type: ") + type);
      }
    }
  }
  void Connection::handleClose(int code, std::string reason, bool wasClean) {
    std::lock_guard<std::mutex> guard(stateMutex);
    for(auto request : waitingRequests) {
      request->handleDisconnect();
    }
    waitingRequests.clear();
    for(auto pair : observations) {
      pair.second->handleDisconnect();
    }
    timeoutCondition.notify_one();
  }

  bool Connection::isConnected() {
    if(webSocket == nullptr) {
      return false;
    }
    return webSocket->getState() == wsxx::WebSocket::State::Open;
  }

  void Connection::connect() {
    std::lock_guard<std::mutex> guard(stateMutex);
    std::weak_ptr self = shared_from_this(); // shared_ptr will make circular reference with webSocket
    auto onOpen = [self]() {
      std::shared_ptr ptr = self.lock();
      if(ptr) ptr->handleOpen();
    };
    auto onWsMessage = [self](std::string data, wsxx::WebSocket::PacketType type) {
      std::shared_ptr ptr = self.lock();
      if(ptr) ptr->handleMessage(data, type);
    };
    auto onWsClose = [self](int code, std::string reason, bool wasClean) {
      std::shared_ptr ptr = self.lock();
      if(ptr) ptr->handleClose(code, reason, wasClean);
    };
    webSocket = std::make_shared<wsxx::WebSocket>(url, onOpen, onWsMessage, onWsClose);
  }

}

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
    observables.push_back(observable);
    if (observables.size() == 1 && connection->isConnected()) {
      nlohmann::json msg = {
          { "type", "observe" },
          { "what", path },
          { "pushed", false }
      };
      connection->send(msg);
    }
    for (auto signal : cachedSignals) {
      Observer observer = observable->observer;
      (*observer)(signal["signal"], signal["args"]);
    }
  }

  void Observation::removeObservable(std::shared_ptr<Observable> observable) {
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

  Connection::Connection(std::string urlp) : url(urlp) {

  }
  Connection::~Connection() {

  }

  void Connection::send(const nlohmann::json& msg) {
    webSocket->send(msg.dump(), wsxx::WebSocket::PacketType::Text);
  }

  promise::Promise<nlohmann::json> sendRequest(
      const nlohmann::json& msg, RequestSettings settings = RequestSettings());

  void Connection::handleOpen() {

  }
  void Connection::handleMessage(std::string data, wsxx::WebSocket::PacketType type) {
    if(type == wsxx::WebSocket::PacketType::Text) {
      auto msg = nlohmann::json::parse(data);
      std::string type = msg["type"];
      if(type == "pong") {

      } else if(type == "ping") {
        msg["type"] = "pong";
        send(msg);
      } else if(type == "authenticationError") {

      } else if(msg.contains("responseId")) {

      } else if(type == "notify") {

      //} else if(type == "push") {
      //} else if(type == "unpush") {
      } else {
        throw std::runtime_error(std::string("unknown message type: ") + type);
      }
    }
  }
  void Connection::handleClose(int code, std::string reason, bool wasClean) {

  }

  bool Connection::isConnected() {
    if(webSocket == nullptr) {
      return false;
    }
    return webSocket->getState() == wsxx::WebSocket::State::Open;
  }

  void Connection::connect() {
    auto self = this;//shared_from_this(); // shared_ptr will make circular reference with webSocket
    auto onOpen = [self]() {
      self->handleOpen();
    };
    auto onWsMessage = [self](std::string data, wsxx::WebSocket::PacketType type) {
      self->handleMessage(data, type);
    };
    auto onWsClose = [self](int code, std::string reason, bool wasClean) {
      self->handleClose(code, reason, wasClean);
    };
    webSocket = std::make_shared<wsxx::WebSocket>(url, onOpen, onWsMessage, onWsClose);
  }

}
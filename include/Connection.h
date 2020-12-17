//
// Created by m8 on 12/1/20.
//

#ifndef LIVECHANGE_CONNECTION_H
#define LIVECHANGE_CONNECTION_H

#include <memory>
#include "Promise.h"
#include "nlohmann/json.hpp"
#include "Observable.h"
#include <WebSocket.h>
#include <condition_variable>

#ifndef _NOEXCEPT
#define _NOEXCEPT _GLIBCXX_USE_NOEXCEPT _GLIBCXX_TXN_SAFE_DYN
#endif

namespace livechange {

  class Connection;

  class DisconnectError : public std::exception {
  public:
    std::string message;
    DisconnectError() {}
    virtual const char* what() const _NOEXCEPT override { return "Server disconnected"; }
  };

  class TimeoutError : public std::exception {
  public:
    std::string message;
    TimeoutError() {}
    virtual const char* what() const _NOEXCEPT override { return "Server disconnected"; }
  };

  class RemoteError : public std::exception {
  public:
    std::string message;
    RemoteError(nlohmann::json error) {
      message = std::string("Remote error: ") + error.dump();
    }
    virtual const char* what() const _NOEXCEPT override { return message.c_str(); }
  };

  class Observation {
  protected:
    std::weak_ptr<Connection> connection; /// CIRCURAL REFERENCE, CONVERT TO WEAKPTR!!!
    nlohmann::json path;
    std::vector<std::shared_ptr<Observable>> observables;
    std::vector<nlohmann::json> cachedSignals;
    std::mutex stateMutex;

    void addObservable(std::shared_ptr<Observable> observable);
    void removeObservable(std::shared_ptr<Observable> observable);
    void addReactions(std::shared_ptr<Observable> observable);
  public:

    Observation(std::shared_ptr<Connection> connectionp, nlohmann::json pathp) : connection(connectionp), path(pathp) {
    }
    template<typename T> std::shared_ptr<T> observable() {
      int type = T::type;
      for(auto observable : observables) {
        if(observable->type == type) {
          return std::dynamic_pointer_cast<T>(observable);
        }
      }
      std::shared_ptr<T> observable = std::make_shared<T>();
      observable->init();
      addReactions(observable);
      addObservable(observable);
      return observable;
    }
    void handleDisconnect();
    void handleConnect();
    void handleNotifyMessage(const nlohmann::json& message);
  };

  class RequestSettings {
  public:
    std::chrono::steady_clock::duration timeout = std::chrono::duration<int,std::milli>(10000);
    std::chrono::steady_clock::duration sentTimeout = std::chrono::duration<int,std::milli>(2300);
    bool queueWhenDisconnected = false;
  };

class Request : public std::enable_shared_from_this<Request> {
  public:
    int requestId;
    nlohmann::json message;
    std::chrono::steady_clock::time_point startPoint;
    bool hasTimeout;
    std::chrono::steady_clock::time_point timeoutPoint;
    std::shared_ptr<promise::Promise<nlohmann::json>> resultPromise;
    RequestSettings settings;
    std::weak_ptr<Connection> connection;
    std::mutex stateMutex;

    Request(std::shared_ptr<Connection> connectionp, int requestIdp,
            nlohmann::json msgp, RequestSettings settingsp);
    void handleMessage(const nlohmann::json message);
    void handleDisconnect();
    void handleTimeout();
  };

  class Connection : public std::enable_shared_from_this<Connection> {
  protected:
    std::string url;
    nlohmann::json sessionId;

    std::map<nlohmann::json, std::shared_ptr<Observation>> observations;
    std::vector<std::shared_ptr<Request>> waitingRequests;
    std::vector<std::shared_ptr<Request>> requestsQueue;
    int lastRequestId;

    std::shared_ptr<wsxx::WebSocket> webSocket;
    friend class Observation;
    friend class Request;

    void handleOpen();
    void handleMessage(std::string data, wsxx::WebSocket::PacketType type);
    void handleClose(int code, std::string reason, bool wasClean);

    void send(const nlohmann::json& msg);
    std::shared_ptr<promise::Promise<nlohmann::json>> sendRequest(
        const nlohmann::json& msg, RequestSettings settings = RequestSettings());

    std::mutex stateMutex;
    int connectedCounter;
    std::thread timeoutThread;
    std::condition_variable timeoutCondition;

  public:
    Connection(std::string urlp, nlohmann::json sessionIdp);
    ~Connection();
    void init();

    std::shared_ptr<Observation> observation(nlohmann::json path) {
      auto it = observations.find(path);
      if(it != observations.end()) {
        return it->second;
      }
      auto observation = std::make_shared<Observation>(shared_from_this(), path);
      observations[path] = observation;
      return observation;
    }

    template<typename T> std::shared_ptr<T> observable(nlohmann::json path) {
      auto observationInstance = observation(path);
      return observationInstance->observable<T>();
    }

    std::shared_ptr<promise::Promise<nlohmann::json>> get(nlohmann::json path,
                                         RequestSettings settings = RequestSettings());
    std::shared_ptr<promise::Promise<nlohmann::json>> request(nlohmann::json method, nlohmann::json args,
                                             RequestSettings settings = RequestSettings());

    bool isConnected();

    void connect();
  };

}

#endif //LIVECHANGE_CONNECTION_H

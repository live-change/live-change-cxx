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

namespace livechange {

  class Connection;

  class Observation {
  protected:
    std::shared_ptr<Connection> connection;
    nlohmann::json path;
    std::vector<std::shared_ptr<Observable>> observables;
    std::vector<nlohmann::json> cachedSignals;

    void addObservable(std::shared_ptr<Observable> observable);
    void removeObservable(std::shared_ptr<Observable> observable);
    void addReactions(std::shared_ptr<Observable> observable);
  public:

    Observation(std::shared_ptr<Connection> connection, nlohmann::json pathp) : path(pathp) {
    }
    template<typename T> T observable(nlohmann::json path) {
      int type = T::type;
      for(auto observable : observables) {
        if(observable->type == type) {
          return observable;
        }
      }
      auto observable = std::make_shared<T>();
      addReactions(observable);
      addObservable(observable);
    }
  };

  class RequestSettings {
  public:
    int timeout = 10000;
    int sentTimeout = 2300;
    bool queueWhenDisconnected = false;
  };

  class Request {
  public:
    int requestId;
    std::chrono::steady_clock::time_point timeoutPoint;
    promise::Promise<nlohmann::json> resultPromise;
    RequestSettings settings;
    void handleMessage(const nlohmann::json message);
    void handleDisconnect();
    void handleTimeout();
  };

  class Connection : std::enable_shared_from_this<Connection> {
  protected:
    std::string url;

    std::map<nlohmann::json, std::shared_ptr<Observation>> observations;
    std::vector<std::shared_ptr<Request>> waitingRequests;
    std::vector<std::shared_ptr<Request>> requestsQueue;

    std::shared_ptr<wsxx::WebSocket> webSocket;
    friend class Observation;

    void handleOpen();
    void handleMessage(std::string data, wsxx::WebSocket::PacketType type);
    void handleClose(int code, std::string reason, bool wasClean);

    void send(const nlohmann::json& msg);
    promise::Promise<nlohmann::json> sendRequest(
        const nlohmann::json& msg, RequestSettings settings = RequestSettings());

  public:
    Connection(std::string urlp);
    ~Connection();

    promise::Promise<nlohmann::json> get(nlohmann::json path);
    std::shared_ptr<Observation> observation(nlohmann::json path) {
      auto it = observations.find(path);
      if(it != observations.end()) {
        return it->second;
      }
      auto observation = std::make_shared<Observation>(shared_from_this(), path);
      observations[path] = observation;
      return observation;
    }

    template<typename T> T observable(nlohmann::json path) {
      auto observationInstance = observation(path);
      return observationInstance->observable<T>();
    }

    promise::Promise<nlohmann::json> request(nlohmann::json method, nlohmann::json args);

    bool isConnected();

    void connect();
  };

}

#endif //LIVECHANGE_CONNECTION_H

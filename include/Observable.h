//
// Created by m8 on 11/27/20.
//

#ifndef LIVECHANGE_OBSERVABLE_H
#define LIVECHANGE_OBSERVABLE_H

#include <string>
#include <functional>
#include <nlohmann/json.hpp>

namespace livechange {

  using ObserverFunction = std::function<void (std::string singal, nlohmann::json args )>;
  using Observer = std::shared_ptr<ObserverFunction>;
  using DisposeCallback = std::shared_ptr<std::function<void ()>>;
  using RespawnCallback = std::shared_ptr<std::function<void ()>>;

  class Observable {
  protected:
    std::vector<Observer> observers;
    bool disposed;

    void fireObservers(const std::string& signal, const nlohmann::json& args) const;

    virtual void dispose();
    virtual void respawn();

  public:
    Observer observer;
    static const int type = 0x00;

    virtual int observableType();

    virtual void observe(const Observer observer);
    virtual void unobserve(const Observer observer);

    bool isUseless() {
      return observers.size() == 0;
    }
    bool isDisposed() {
      return disposed;
    }

    std::vector<DisposeCallback> onDispose;
    std::vector<RespawnCallback> onRespawn;
  };



}

#endif //LIVECHANGE_OBSERVABLE_H

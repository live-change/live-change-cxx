//
// Created by m8 on 11/27/20.
//

#ifndef PEERCONNECTIONTEST_OBSERVABLE_H
#define PEERCONNECTIONTEST_OBSERVABLE_H

#include <string>
#include <functional>
#include <nlohmann/json.hpp>

namespace livechange {

  using Observer = std::shared_ptr<std::function<void (std::string singal, nlohmann::json args )>>;

  class Observable {
  protected:
    std::vector<Observer> observers;
    bool disposed;

    void fireObservers(const std::string& signal, const nlohmann::json& args) const;

    virtual void dispose();
    virtual void respawn();

  public:

    virtual void observe(const Observer observer);
    virtual void unobserve(const Observer observer);

    bool isUseless() {
      return observers.size() == 0;
    }
    bool isDisposed() {
      return disposed;
    }
  };



}

#endif //PEERCONNECTIONTEST_OBSERVABLE_H

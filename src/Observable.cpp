#include "Observable.h"

namespace livechange {

  int Observable::observableType() {
    return Observable::type;
  }

  void Observable::fireObservers(const std::string& signal, const nlohmann::json& args) const {
    for(auto observer : observers) (*observer)(signal, args);
  }

  void Observable::dispose() {
    disposed = true;
    for(auto callback : onDispose) (*callback)();
  }
  void Observable::respawn() {
    disposed = false;
    for(auto callback : onRespawn) (*callback)();
  }

  void Observable::observe(const Observer observer) {
    observers.push_back(observer);
  }

  void Observable::unobserve(const Observer observer) {
    observers.erase(std::remove_if(observers.begin(), observers.end(),
                                   [&observer](Observer o) { return o == observer; } ));
  }

}
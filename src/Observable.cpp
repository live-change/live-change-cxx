#include "Observable.h"

namespace livechange {

  void Observable::fireObservers(const std::string& signal, const nlohmann::json& args) const {
    for(auto observer : observers) (*observer)(signal, args);
  }

  void Observable::dispose() {
    disposed = true;
  }
  void Observable::respawn() {
    disposed = false;
  }

  void Observable::observe(const Observer observer) {
    observers.push_back(observer);
  }

  void Observable::unobserve(const Observer observer) {
    observers.erase(std::remove_if(observers.begin(), observers.end(),
                                   [&observer](Observer o) { return o == observer; } ));
  }

}
#include <ObservableValue.h>

namespace livechange {

  int ObservableValue::observableType() {
    return ObservableValue::type;
  }

  void handleValueSignal(std::shared_ptr<ObservableValue> observable, std::string signal, nlohmann::json args) {
    if(signal == "set") {
      observable->set(args[0]);
    } else {
      throw new std::runtime_error("signal " + signal + " not implemented");
    }
  }

  ObservableValue::ObservableValue() {

  }
  void ObservableValue::init() {
    using namespace std::placeholders;
    std::function<void(std::string signal, nlohmann::json args)> f =
        std::bind(&handleValueSignal, shared_from_this(), _1, _2);
    observer = std::make_shared<std::function<void(std::string signal, nlohmann::json args)>>(f);
  }

  void ObservableValue::set(nlohmann::json value) {
    this->value = value;
    this->fireObservers("set", { value });
  }

  void ObservableValue::observe(const Observer observer) {
    observers.push_back(observer);
    (*observer)("set", { value });
  }

  void ObservableValue::unobserve(const Observer observer) {
    observers.erase(std::remove_if(observers.begin(), observers.end(),
                                   [&observer](Observer o) { return o == observer; } ));
  }
}

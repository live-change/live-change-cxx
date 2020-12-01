#include <ObservableList.h>

namespace livechange {

  void handleListSignal(std::shared_ptr<ObservableList> list, std::string signal, nlohmann::json args) {
    if(signal == "set") {
      list->set(args[0]);
    } else if(signal == "push") {
      list->push(args[0]);
    } else if(signal == "putByField") {
      list->putByField(args[0], args[1], args[2], args[3],
                       args.size() > 4 ? nlohmann::json(args[4]) : nlohmann::json(nullptr));
    } else if(signal == "removeByField") {
      list->removeByField(args[0], args[1],
                          args.size() > 2 ? nlohmann::json(args[2]) : nlohmann::json(nullptr));
    } else if(signal == "updateByField") {
      list->updateByField(args[0], args[1], args[2],
                          args.size() > 3 ? nlohmann::json(args[3]) : nlohmann::json(nullptr));
    } else {
      throw new std::runtime_error("signal " + signal + " not implemented");
    }
  }

  ObservableList::ObservableList() {
    using namespace std::placeholders;
    std::function<void(std::string signal, nlohmann::json args)> f =
        std::bind(&handleListSignal, shared_from_this(), _1, _2);
    observer = std::make_shared<std::function<void(std::string signal, nlohmann::json args)>>(f);
  }

  void ObservableList::set(nlohmann::json value) {
    list = value;
    this->fireObservers("set", { value });
  }

  void ObservableList::push(nlohmann::json value) {
    list.push_back(value);
    this->fireObservers("push", { value });
  }

  //void unshift(nlohmann::json value);
  //void pop();
  //void shift();
  //void splice(size_t at, size_t del, nlohmann::json value);
  void ObservableList::putByField(std::string field, nlohmann::json value, nlohmann::json element, bool reverse,
                  nlohmann::json oldElement) {
    if(!reverse) {
      for(size_t i = 0; i < list.size(); i++) {
        if(list[i][field] == value) {
          list[i] = element;
          goto done;
        } else if(this->list[i][field] > value) {
          list.insert(list.begin() + i, element);
          goto done;
        }
      }
      list.push_back(element);
    } else {
      for(size_t i = list.size()-1; i >= 0; i --) {
        if(list[i][field] == value) {
          list[i] = element;
          goto done;
        } else if(list[i][field] > value) {
          list.insert(list.begin()+i, element);
          goto done;
        }
      }
      list.insert(list.begin(), element);
    }
    done:
    fireObservers("putByField", { field, value, element, reverse, oldElement });
  }

  //void remove(nlohmann::json element);
  void ObservableList::removeByField(std::string field, nlohmann::json value, nlohmann::json oldElement) {
    for(size_t i = 0; i < list.size(); i++) {
      if(list[i][field] == value) {
        list.erase(i);
        i--;
      }
    }
    fireObservers("removeByField", { field, value, oldElement });
  }

  //void removeBy(nlohmann::json fields);
  //void update(nlohmann::json what, nlohmann::json with);
  void ObservableList::updateByField(std::string field, nlohmann::json value, nlohmann::json element,
                                     nlohmann::json oldElement) {
    for(size_t i = 0; i < list.size(); i++) {
      if(list[i][field] == value) {
        list[i] = element;
      }
    }
    fireObservers("updateByField", { field, value, element, oldElement });
  }
  //void updateBy(nlohmann::json fields, nlohmann::json with);

  void ObservableList::observe(const Observer observer) {
    observers.push_back(observer);
    (*observer)("set", { list });
  }

  void ObservableList::unobserve(const Observer observer) {
    observers.erase(std::remove_if(observers.begin(), observers.end(),
                                   [&observer](Observer o) { return o == observer; } ));
  }
}

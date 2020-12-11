//
// Created by m8 on 11/27/20.
//

#ifndef LIVECHANGE_OBSERVABLELIST_H
#define LIVECHANGE_OBSERVABLELIST_H

#include "Observable.h"

namespace livechange {

  class ObservableList : Observable, std::enable_shared_from_this<ObservableList> {
  protected:
    bool initialized;
  public:
    nlohmann::json list;

    ObservableList();

    void set(nlohmann::json value);
    void push(nlohmann::json value);
    //void unshift(nlohmann::json value);
    //void pop();
    //void shift();
    //void splice(size_t at, size_t del, nlohmann::json value);
    void putByField(std::string field, nlohmann::json value, nlohmann::json element, bool reverse,
                    nlohmann::json oldElement);
    //void remove(nlohmann::json element);
    void removeByField(std::string field, nlohmann::json value, nlohmann::json oldElement);
    //void removeBy(nlohmann::json fields);
    //void update(nlohmann::json what, nlohmann::json with);
    void updateByField(std::string field, nlohmann::json value, nlohmann::json element, nlohmann::json oldElement);
    //void updateBy(nlohmann::json fields, nlohmann::json with);

    static const int type = 0x02;
    virtual int observableType() override;

    virtual void observe(const Observer observer) override;
    virtual void unobserve(const Observer observer) override;

    bool isInitialized() {
      return initialized;
    }
  };

}

#endif //LIVECHANGE_OBSERVABLELIST_H

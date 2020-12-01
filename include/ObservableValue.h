//
// Created by m8 on 11/27/20.
//

#ifndef LIVECHANGE_OBSERVABLEVALUE_H
#define LIVECHANGE_OBSERVABLEVALUE_H

#include "Observable.h"

namespace livechange {

  class ObservableValue : Observable, std::enable_shared_from_this<ObservableValue> {
  protected:
    bool initialized;
  public:
    nlohmann::json value;
    Observer observer;

    ObservableValue();

    void set(nlohmann::json value);

    virtual void observe(const Observer observer) override;
    virtual void unobserve(const Observer observer) override;

    bool isInitialized() {
      return initialized;
    }
  };

}

#endif //LIVECHANGE_OBSERVABLEVALUE_H

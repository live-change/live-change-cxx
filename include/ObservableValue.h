//
// Created by m8 on 11/27/20.
//

#ifndef LIVECHANGE_OBSERVABLEVALUE_H
#define LIVECHANGE_OBSERVABLEVALUE_H

#include "Observable.h"

namespace livechange {

  class ObservableValue : public Observable, public std::enable_shared_from_this<ObservableValue> {
  protected:
    bool initialized;
  public:
    nlohmann::json value;

    ObservableValue();
    void init();

    void set(nlohmann::json value);

    static const int type = 0x01;
    virtual int observableType() override;

    virtual void observe(const Observer observer) override;
    virtual void unobserve(const Observer observer) override;

    bool isInitialized() {
      return initialized;
    }
  };

}

#endif //LIVECHANGE_OBSERVABLEVALUE_H

#ifndef FINANCE_APP_H
#define FINANCE_APP_H

#include <escher.h>
#include "parameters_controller.h"
#include "unknown_controller.h"
#include "tvm.h"
#include "../shared/text_field_delegate_app.h"
#include "../shared/shared_app.h"
#include "../shared/poincare_local_pool.h"

namespace Finance {

class App : public Shared::TextFieldDelegateApp {
public:
  class Descriptor : public ::App::Descriptor {
  public:
    I18n::Message name() override;
    I18n::Message upperName() override;
    const Image * icon() override;
  };
  class Snapshot : public ::SharedApp::Snapshot {
  public:
    Snapshot();
    App * unpack(Container * container) override;
    Descriptor * descriptor() override;
    void reset() override;
    double * values() { return m_values; }
  private:
    double m_values[NumberOfVariables];
  };
private:
  App(Snapshot * snapshot);
  Shared::PoincareLocalPool m_poincarePool;
  ParametersController m_parametersController;
  UnknownController m_unknownController;
  StackViewController m_stackViewController;
};

}

#endif

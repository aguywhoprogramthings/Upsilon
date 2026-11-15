#ifndef INFERENCE_APP_H
#define INFERENCE_APP_H

#include <escher.h>
#include "parameters_controller.h"
#include "results_controller.h"
#include "statistic_controller.h"
#include "statistics.h"
#include "../shared/text_field_delegate_app.h"
#include "../shared/shared_app.h"
#include "../shared/poincare_local_pool.h"

namespace Inference {

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
    double m_values[NumberOfParameters];
  };
private:
  App(Snapshot * snapshot);
  Shared::PoincareLocalPool m_poincarePool;
  ResultsController m_resultsController;
  ParametersController m_parametersController;
  StatisticController m_statisticController;
  StackViewController m_stackViewController;
};

}

#endif

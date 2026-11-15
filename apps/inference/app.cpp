#include "app.h"
#include "inference_icon.h"
#include <apps/i18n.h>
#include <new>

namespace Inference {

I18n::Message App::Descriptor::name() {
  return I18n::Message::InferenceApp;
}

I18n::Message App::Descriptor::upperName() {
  return I18n::Message::InferenceAppCapital;
}

const Image * App::Descriptor::icon() {
  return ImageStore::InferenceIcon;
}

App::Snapshot::Snapshot() {
  reset();
}

App * App::Snapshot::unpack(Container * container) {
  return new (container->currentAppBuffer()) App(this);
}

App::Descriptor * App::Snapshot::descriptor() {
  static Descriptor descriptor;
  return &descriptor;
}

void App::Snapshot::reset() {
  for (int i = 0; i < NumberOfParameters; i++) {
    m_values[i] = 0.0; // successes, means, observed counts, table cells
  }
  m_values[P0] = 0.5;
  m_values[SampleSize] = m_values[SampleSize2] = 30;
  m_values[SampleDeviation] = m_values[SampleDeviation2] = 1;
  m_values[PopulationDeviation] = m_values[PopulationDeviation2] = 1;
  m_values[ConfidenceLevel] = 0.95;
  m_values[Categories] = m_values[Rows] = m_values[Columns] = 2;
  for (int i = 0; i < MaxNumberOfCategories; i++) {
    m_values[Expected0 + i] = 1; // a zero expected count is not allowed
  }
}

App::App(Snapshot * snapshot) :
  TextFieldDelegateApp(snapshot, &m_stackViewController),
  m_resultsController(&m_stackViewController),
  m_parametersController(&m_stackViewController, this, snapshot->values(), &m_resultsController),
  m_statisticController(&m_stackViewController, &m_parametersController),
  m_stackViewController(&m_modalViewController, &m_statisticController)
{
}

}

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
  //                                           p0   x  n   mu0 mean s  sigma C
  constexpr double defaults[NumberOfParameters] = {0.5, 0, 30, 0, 0, 1, 1, 0.95};
  for (int i = 0; i < NumberOfParameters; i++) {
    m_values[i] = defaults[i];
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

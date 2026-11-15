#include "app.h"
#include "finance_icon.h"
#include <apps/i18n.h>
#include <new>

namespace Finance {

I18n::Message App::Descriptor::name() {
  return I18n::Message::FinanceApp;
}

I18n::Message App::Descriptor::upperName() {
  return I18n::Message::FinanceAppCapital;
}

const Image * App::Descriptor::icon() {
  return ImageStore::FinanceIcon;
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
  constexpr double defaults[NumberOfVariables] = {0, 0, 0, 0, 0, 12, 12};
  for (int i = 0; i < NumberOfVariables; i++) {
    m_values[i] = defaults[i];
  }
}

App::App(Snapshot * snapshot) :
  TextFieldDelegateApp(snapshot, &m_stackViewController),
  m_parametersController(&m_stackViewController, this, snapshot->values()),
  m_unknownController(&m_stackViewController, &m_parametersController),
  m_stackViewController(&m_modalViewController, &m_unknownController)
{
}

}

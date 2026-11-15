#include "unknown_controller.h"

namespace Finance {

UnknownController::UnknownController(Responder * parentResponder, ParametersController * parametersController) :
  ViewController(parentResponder),
  m_selectableTableView(this, this, this),
  m_parametersController(parametersController)
{
}

const char * UnknownController::title() {
  return I18n::translate(I18n::Message::FinanceChooseUnknown);
}

void UnknownController::didBecomeFirstResponder() {
  if (selectedRow() < 0) {
    selectCellAtLocation(0, 0);
  }
  Container::activeApp()->setFirstResponder(&m_selectableTableView);
}

bool UnknownController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::OK || event == Ion::Events::EXE || event == Ion::Events::Right) {
    m_parametersController->setUnknown(static_cast<Variable>(selectedRow()));
    static_cast<StackViewController *>(parentResponder())->push(m_parametersController);
    return true;
  }
  return false;
}

void UnknownController::willDisplayCellForIndex(HighlightCell * cell, int index) {
  static_cast<MessageTableCellWithChevron<> *>(cell)->setMessage(ParametersController::Label(index));
}

}

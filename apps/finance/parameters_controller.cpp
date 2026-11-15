#include "parameters_controller.h"
#include <cmath>

namespace Finance {

ParametersController::ParametersController(Responder * parentResponder, InputEventHandlerDelegate * inputEventHandlerDelegate, double * values) :
  FloatParameterController<double>(parentResponder),
  m_cells{},
  m_values(values),
  m_unknown(FutureValue)
{
  m_okButton.setMessage(I18n::Message::Compute);
  for (int i = 0; i < NumberOfVariables; i++) {
    m_cells[i].setParentResponder(&m_selectableTableView);
    m_cells[i].textField()->setDelegates(inputEventHandlerDelegate, this);
  }
}

I18n::Message ParametersController::Label(int variable) {
  constexpr I18n::Message labels[NumberOfVariables] = {
    I18n::Message::FinanceN,
    I18n::Message::FinanceInterestRate,
    I18n::Message::FinancePresentValue,
    I18n::Message::FinancePayment,
    I18n::Message::FinanceFutureValue,
    I18n::Message::FinancePaymentsPerYear,
    I18n::Message::FinanceCompoundingsPerYear
  };
  return labels[variable];
}

const char * ParametersController::title() {
  return I18n::translate(Label(m_unknown));
}

void ParametersController::willDisplayCellForIndex(HighlightCell * cell, int index) {
  if (index == numberOfRows() - 1) {
    return;
  }
  static_cast<MessageTableCellWithEditableText *>(cell)->setMessage(Label(index));
  FloatParameterController<double>::willDisplayCellForIndex(cell, index);
}

bool ParametersController::setParameterAtIndex(int parameterIndex, double f) {
  // Payments and compoundings per year divide the annual rate
  if (parameterIndex >= PaymentsPerYear && !(f > 0)) {
    Container::activeApp()->displayWarning(I18n::Message::ForbiddenValue);
    return false;
  }
  m_values[parameterIndex] = f;
  return true;
}

void ParametersController::buttonAction() {
  double result = SolveTVM(m_values, m_unknown);
  if (!std::isfinite(result)) {
    Container::activeApp()->displayWarning(I18n::Message::UndefinedValue);
    return;
  }
  m_values[m_unknown] = result;
  m_selectableTableView.reloadData();
  m_selectableTableView.selectCellAtLocation(0, m_unknown);
}

}

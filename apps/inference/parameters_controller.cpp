#include "parameters_controller.h"
#include "statistic_controller.h"

namespace Inference {

ParametersController::ParametersController(Responder * parentResponder, InputEventHandlerDelegate * inputEventHandlerDelegate, double * values, ResultsController * resultsController) :
  FloatParameterController<double>(parentResponder),
  m_cells{},
  m_values(values),
  m_resultsController(resultsController),
  m_statistic(TestProportion)
{
  m_okButton.setMessage(I18n::Message::Compute);
  for (int i = 0; i < MaxNumberOfParametersPerStatistic; i++) {
    m_cells[i].setParentResponder(&m_selectableTableView);
    m_cells[i].textField()->setDelegates(inputEventHandlerDelegate, this);
  }
}

const char * ParametersController::title() {
  return I18n::translate(StatisticController::Label(m_statistic));
}

void ParametersController::willDisplayCellForIndex(HighlightCell * cell, int index) {
  if (index == numberOfRows() - 1) {
    return;
  }
  constexpr I18n::Message labels[NumberOfParameters] = {
    I18n::Message::InferenceP0,
    I18n::Message::InferenceSuccesses,
    I18n::Message::InferenceSampleSize,
    I18n::Message::InferenceMu0,
    I18n::Message::InferenceSampleMean,
    I18n::Message::InferenceSampleDeviation,
    I18n::Message::InferencePopulationDeviation,
    I18n::Message::InferenceConfidenceLevel
  };
  static_cast<MessageTableCellWithEditableText *>(cell)->setMessage(labels[ParameterFor(m_statistic, index)]);
  FloatParameterController<double>::willDisplayCellForIndex(cell, index);
}

bool ParametersController::setParameterAtIndex(int parameterIndex, double f) {
  // Ranges depend on the statistic, so they are checked by Compute
  m_values[ParameterFor(m_statistic, parameterIndex)] = f;
  return true;
}

void ParametersController::buttonAction() {
  double results[NumberOfResults];
  if (!Compute(m_statistic, m_values, results)) {
    Container::activeApp()->displayWarning(I18n::Message::ForbiddenValue);
    return;
  }
  m_resultsController->setResults(m_statistic, results);
  stackController()->push(m_resultsController);
}

}

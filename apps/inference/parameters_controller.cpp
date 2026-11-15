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
  for (int i = 0; i < k_numberOfCells; i++) {
    m_cells[i].setParentResponder(&m_selectableTableView);
    m_cells[i].textField()->setDelegates(inputEventHandlerDelegate, this);
  }
}

const char * ParametersController::title() {
  return I18n::translate(StatisticController::Label(m_statistic));
}

static I18n::Message labelOf(Parameter parameter) {
  constexpr I18n::Message labels[NumberOfParameters] = {
    I18n::Message::InferenceP0,
    I18n::Message::InferenceSuccesses,
    I18n::Message::InferenceSampleSize,
    I18n::Message::InferenceMu0,
    I18n::Message::InferenceSampleMean,
    I18n::Message::InferenceSampleDeviation,
    I18n::Message::InferencePopulationDeviation,
    I18n::Message::InferenceConfidenceLevel,
    I18n::Message::InferenceSuccesses2,
    I18n::Message::InferenceSampleSize2,
    I18n::Message::InferenceSampleMean2,
    I18n::Message::InferenceSampleDeviation2,
    I18n::Message::InferencePopulationDeviation2,
    I18n::Message::InferenceCategories,
    I18n::Message::InferenceO1, I18n::Message::InferenceO2, I18n::Message::InferenceO3, I18n::Message::InferenceO4,
    I18n::Message::InferenceO5, I18n::Message::InferenceO6, I18n::Message::InferenceO7, I18n::Message::InferenceO8,
    I18n::Message::InferenceE1, I18n::Message::InferenceE2, I18n::Message::InferenceE3, I18n::Message::InferenceE4,
    I18n::Message::InferenceE5, I18n::Message::InferenceE6, I18n::Message::InferenceE7, I18n::Message::InferenceE8,
    I18n::Message::InferenceRows,
    I18n::Message::InferenceColumns,
    I18n::Message::InferenceN11, I18n::Message::InferenceN12, I18n::Message::InferenceN13, I18n::Message::InferenceN14,
    I18n::Message::InferenceN21, I18n::Message::InferenceN22, I18n::Message::InferenceN23, I18n::Message::InferenceN24,
    I18n::Message::InferenceN31, I18n::Message::InferenceN32, I18n::Message::InferenceN33, I18n::Message::InferenceN34,
    I18n::Message::InferenceN41, I18n::Message::InferenceN42, I18n::Message::InferenceN43, I18n::Message::InferenceN44
  };
  return labels[parameter];
}

void ParametersController::willDisplayCellForIndex(HighlightCell * cell, int index) {
  if (index == numberOfRows() - 1) {
    return;
  }
  static_cast<MessageTableCellWithEditableText *>(cell)->setMessage(labelOf(ParameterFor(m_statistic, index, m_values)));
  FloatParameterController<double>::willDisplayCellForIndex(cell, index);
}

bool ParametersController::setParameterAtIndex(int parameterIndex, double f) {
  Parameter parameter = ParameterFor(m_statistic, parameterIndex, m_values);
  // Sizes change the number of rows, so they are checked now; the rest in Compute
  if (!IsValidSize(parameter, f)) {
    Container::activeApp()->displayWarning(I18n::Message::ForbiddenValue);
    return false;
  }
  m_values[parameter] = f;
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

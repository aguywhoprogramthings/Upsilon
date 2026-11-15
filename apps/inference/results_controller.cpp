#include "results_controller.h"
#include "../shared/poincare_helpers.h"
#include <poincare/preferences.h>
#include <poincare/print_float.h>

using namespace Poincare;

namespace Inference {

ResultsController::ResultsController(Responder * parentResponder) :
  ViewController(parentResponder),
  m_selectableTableView(this, this, this),
  m_statistic(TestProportion),
  m_results{}
{
}

const char * ResultsController::title() {
  return I18n::translate(I18n::Message::InferenceResults);
}

void ResultsController::didBecomeFirstResponder() {
  selectCellAtLocation(0, 0);
  Container::activeApp()->setFirstResponder(&m_selectableTableView);
}

void ResultsController::setResults(Statistic statistic, const double results[NumberOfResults]) {
  m_statistic = statistic;
  for (int i = 0; i < NumberOfResults; i++) {
    m_results[i] = results[i];
  }
}

void ResultsController::willDisplayCellForIndex(HighlightCell * cell, int index) {
  constexpr I18n::Message testLabels[NumberOfResults] = {I18n::Message::InferenceZ, I18n::Message::InferencePTwoSided, I18n::Message::InferencePLess, I18n::Message::InferencePGreater};
  constexpr I18n::Message intervalLabels[NumberOfResults] = {I18n::Message::InferenceEstimate, I18n::Message::InferenceMargin, I18n::Message::InferenceLower, I18n::Message::InferenceUpper};
  constexpr I18n::Message chiSquaredLabels[] = {I18n::Message::InferenceChi, I18n::Message::InferenceDegreesOfFreedom, I18n::Message::InferencePValue};
  MessageTableCellWithBuffer * myCell = static_cast<MessageTableCellWithBuffer *>(cell);
  if (IsChiSquared(m_statistic)) {
    myCell->setMessage(chiSquaredLabels[index]);
  } else if (!IsTest(m_statistic)) {
    myCell->setMessage(intervalLabels[index]);
  } else if (index == 0 && UsesStudent(m_statistic)) {
    myCell->setMessage(I18n::Message::InferenceT);
  } else {
    myCell->setMessage(testLabels[index]);
  }
  constexpr int precision = Preferences::LargeNumberOfSignificantDigits;
  char buffer[PrintFloat::charSizeForFloatsWithPrecision(precision)];
  Shared::PoincareHelpers::ConvertFloatToText<double>(m_results[index], buffer, sizeof(buffer), precision);
  myCell->setAccessoryText(buffer);
}

}

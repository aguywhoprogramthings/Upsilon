#include "statistic_controller.h"
#include "parameters_controller.h"

namespace Inference {

StatisticController::StatisticController(Responder * parentResponder, ParametersController * parametersController) :
  ViewController(parentResponder),
  m_selectableTableView(this, this, this),
  m_parametersController(parametersController)
{
}

I18n::Message StatisticController::Label(Statistic statistic) {
  constexpr I18n::Message labels[NumberOfStatistics] = {
    I18n::Message::InferenceTestProportion,
    I18n::Message::InferenceTestMeanT,
    I18n::Message::InferenceTestMeanZ,
    I18n::Message::InferenceIntervalProportion,
    I18n::Message::InferenceIntervalMeanT,
    I18n::Message::InferenceIntervalMeanZ
  };
  return labels[statistic];
}

const char * StatisticController::title() {
  return I18n::translate(I18n::Message::InferenceChooseStatistic);
}

void StatisticController::didBecomeFirstResponder() {
  if (selectedRow() < 0) {
    selectCellAtLocation(0, 0);
  }
  Container::activeApp()->setFirstResponder(&m_selectableTableView);
}

bool StatisticController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::OK || event == Ion::Events::EXE || event == Ion::Events::Right) {
    m_parametersController->setStatistic(static_cast<Statistic>(selectedRow()));
    static_cast<StackViewController *>(parentResponder())->push(m_parametersController);
    return true;
  }
  return false;
}

void StatisticController::willDisplayCellForIndex(HighlightCell * cell, int index) {
  static_cast<MessageTableCellWithChevron<> *>(cell)->setMessage(Label(static_cast<Statistic>(index)));
}

}

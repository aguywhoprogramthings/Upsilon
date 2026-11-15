#ifndef INFERENCE_PARAMETERS_CONTROLLER_H
#define INFERENCE_PARAMETERS_CONTROLLER_H

#include <escher.h>
#include <apps/i18n.h>
#include "statistics.h"
#include "results_controller.h"
#include "../shared/float_parameter_controller.h"

namespace Inference {

// Edits the inputs of the chosen statistic; the button computes and shows the results.
class ParametersController : public Shared::FloatParameterController<double> {
public:
  ParametersController(Responder * parentResponder, InputEventHandlerDelegate * inputEventHandlerDelegate, double * values, ResultsController * resultsController);
  const char * title() override;
  int numberOfRows() const override { return NumberOfParametersFor(m_statistic) + 1; }
  void willDisplayCellForIndex(HighlightCell * cell, int index) override;
  void setStatistic(Statistic statistic) { m_statistic = statistic; }
private:
  HighlightCell * reusableParameterCell(int index, int type) override { return &m_cells[index]; }
  int reusableParameterCellCount(int type) override { return MaxNumberOfParametersPerStatistic; }
  double parameterAtIndex(int index) override { return m_values[ParameterFor(m_statistic, index)]; }
  bool setParameterAtIndex(int parameterIndex, double f) override;
  void buttonAction() override;
  MessageTableCellWithEditableText m_cells[MaxNumberOfParametersPerStatistic];
  double * m_values;
  ResultsController * m_resultsController;
  Statistic m_statistic;
};

}

#endif

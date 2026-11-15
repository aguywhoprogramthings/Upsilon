#ifndef FINANCE_PARAMETERS_CONTROLLER_H
#define FINANCE_PARAMETERS_CONTROLLER_H

#include <escher.h>
#include <apps/i18n.h>
#include "tvm.h"
#include "../shared/float_parameter_controller.h"

namespace Finance {

/* Edits the seven TVM values; the button solves for the chosen unknown and
 * writes the result into its row. */
class ParametersController : public Shared::FloatParameterController<double> {
public:
  ParametersController(Responder * parentResponder, InputEventHandlerDelegate * inputEventHandlerDelegate, double * values);
  static I18n::Message Label(int variable);
  const char * title() override;
  int numberOfRows() const override { return NumberOfVariables + 1; }
  void willDisplayCellForIndex(HighlightCell * cell, int index) override;
  void setUnknown(Variable unknown) { m_unknown = unknown; }
private:
  HighlightCell * reusableParameterCell(int index, int type) override { return &m_cells[index]; }
  int reusableParameterCellCount(int type) override { return NumberOfVariables; }
  double parameterAtIndex(int index) override { return m_values[index]; }
  bool setParameterAtIndex(int parameterIndex, double f) override;
  void buttonAction() override;
  MessageTableCellWithEditableText m_cells[NumberOfVariables];
  double * m_values;
  Variable m_unknown;
};

}

#endif

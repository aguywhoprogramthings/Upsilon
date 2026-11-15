#ifndef FINANCE_UNKNOWN_CONTROLLER_H
#define FINANCE_UNKNOWN_CONTROLLER_H

#include <escher.h>
#include <escher/metric.h>
#include "parameters_controller.h"

namespace Finance {

// First page: pick which of N, I%, PV, PMT, FV to compute.
class UnknownController : public ViewController, public SimpleListViewDataSource, public SelectableTableViewDataSource {
public:
  UnknownController(Responder * parentResponder, ParametersController * parametersController);
  View * view() override { return &m_selectableTableView; }
  const char * title() override;
  void didBecomeFirstResponder() override;
  bool handleEvent(Ion::Events::Event event) override;
  int numberOfRows() const override { return NumberOfUnknowns; }
  KDCoordinate cellHeight() override { return Metric::ParameterCellHeight; }
  HighlightCell * reusableCell(int index) override { return &m_cells[index]; }
  int reusableCellCount() const override { return NumberOfUnknowns; }
  void willDisplayCellForIndex(HighlightCell * cell, int index) override;
private:
  SelectableTableView m_selectableTableView;
  MessageTableCellWithChevron<> m_cells[NumberOfUnknowns];
  ParametersController * m_parametersController;
};

}

#endif

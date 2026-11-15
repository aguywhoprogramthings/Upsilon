#ifndef INFERENCE_RESULTS_CONTROLLER_H
#define INFERENCE_RESULTS_CONTROLLER_H

#include <escher.h>
#include <escher/metric.h>
#include <apps/i18n.h>
#include "statistics.h"

namespace Inference {

// Read-only list of the four computed values.
class ResultsController : public ViewController, public SimpleListViewDataSource, public SelectableTableViewDataSource {
public:
  ResultsController(Responder * parentResponder);
  View * view() override { return &m_selectableTableView; }
  const char * title() override;
  void didBecomeFirstResponder() override;
  void setResults(Statistic statistic, const double results[NumberOfResults]);
  int numberOfRows() const override { return NumberOfResults; }
  KDCoordinate cellHeight() override { return Metric::ParameterCellHeight; }
  HighlightCell * reusableCell(int index) override { return &m_cells[index]; }
  int reusableCellCount() const override { return NumberOfResults; }
  void willDisplayCellForIndex(HighlightCell * cell, int index) override;
private:
  SelectableTableView m_selectableTableView;
  MessageTableCellWithBuffer m_cells[NumberOfResults];
  Statistic m_statistic;
  double m_results[NumberOfResults];
};

}

#endif

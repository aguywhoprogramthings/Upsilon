#ifndef INFERENCE_STATISTIC_CONTROLLER_H
#define INFERENCE_STATISTIC_CONTROLLER_H

#include <escher.h>
#include <escher/metric.h>
#include <apps/i18n.h>
#include "statistics.h"

namespace Inference {

class ParametersController;

// First page: pick a test or a confidence interval.
class StatisticController : public ViewController, public SimpleListViewDataSource, public SelectableTableViewDataSource {
public:
  StatisticController(Responder * parentResponder, ParametersController * parametersController);
  static I18n::Message Label(Statistic statistic);
  View * view() override { return &m_selectableTableView; }
  const char * title() override;
  void didBecomeFirstResponder() override;
  bool handleEvent(Ion::Events::Event event) override;
  int numberOfRows() const override { return NumberOfStatistics; }
  KDCoordinate cellHeight() override { return Metric::ParameterCellHeight; }
  HighlightCell * reusableCell(int index) override { return &m_cells[index]; }
  int reusableCellCount() const override { return NumberOfStatistics; }
  void willDisplayCellForIndex(HighlightCell * cell, int index) override;
private:
  SelectableTableView m_selectableTableView;
  MessageTableCellWithChevron<> m_cells[NumberOfStatistics];
  ParametersController * m_parametersController;
};

}

#endif

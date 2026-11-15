#ifndef FINANCE_TVM_H
#define FINANCE_TVM_H

namespace Finance {

/* Time value of money, payments at the end of each period.
 * Cash-flow convention (money paid is negative):
 *   PV*(1+i)^N + PMT*((1+i)^N-1)/i + FV = 0
 * where i is the periodic rate derived from I% (annual), P/Y and C/Y. */
enum Variable {
  N,
  InterestRate,
  PresentValue,
  Payment,
  FutureValue,
  PaymentsPerYear,
  CompoundingsPerYear,
  NumberOfVariables
};
constexpr int NumberOfUnknowns = PaymentsPerYear; // N to FV can be solved for

// Value of unknown given the other variables; NAN or infinite when there is none.
double SolveTVM(const double values[NumberOfVariables], Variable unknown);

}

#endif

#include <quiz.h>
#include <cmath>
#include "../tvm.h"

using namespace Finance;

static void assert_solves(const double values[NumberOfVariables], Variable unknown, double expected, double tolerance) {
  double result = SolveTVM(values, unknown);
  quiz_assert(std::fabs(result - expected) <= tolerance * (1.0 + std::fabs(expected)));
}

QUIZ_CASE(finance_tvm_loan) {
  // 200000 borrowed at 6 % a year over 30 years, monthly payments
  double loan[NumberOfVariables] = {360, 6, 200000, -1199.1010503055, 0, 12, 12};
  assert_solves(loan, Payment, -1199.1010503055, 1e-9);
  assert_solves(loan, InterestRate, 6, 1e-7);
  assert_solves(loan, N, 360, 1e-7);
  assert_solves(loan, PresentValue, 200000, 1e-9);
}

QUIZ_CASE(finance_tvm_savings_and_edge_cases) {
  double savings[NumberOfVariables] = {10, 5, -1000, -100, 0, 1, 1};
  assert_solves(savings, FutureValue, 2886.683880332325, 1e-9);
  double zeroRate[NumberOfVariables] = {10, 0, 1000, -100, 0, 12, 12};
  assert_solves(zeroRate, FutureValue, 0, 1e-9);
  assert_solves(zeroRate, N, 10, 1e-9);
  // Quarterly compounding, monthly payments
  double quarterly[NumberOfVariables] = {12, 12, 1000, 0, 0, 12, 4};
  assert_solves(quarterly, FutureValue, -1000 * std::pow(1.03, 4), 1e-9);
  quarterly[FutureValue] = -1000 * std::pow(1.03, 4);
  assert_solves(quarterly, InterestRate, 12, 1e-7);
  double noRate[NumberOfVariables] = {10, 5, 1000, 100, 100, 12, 12};
  quiz_assert(std::isnan(SolveTVM(noRate, InterestRate)));
  double badPeriods[NumberOfVariables] = {10, 5, 1000, -100, 0, 0, 12};
  quiz_assert(std::isnan(SolveTVM(badPeriods, FutureValue)));
}

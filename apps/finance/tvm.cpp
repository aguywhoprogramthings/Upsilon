#include "tvm.h"
#include <cmath>

namespace Finance {

static constexpr double k_zeroRate = 1e-12;

// (1+i)^n and the annuity factor ((1+i)^n-1)/i, continuous at i = 0
static void growthAndAnnuity(double i, double n, double * growth, double * annuity) {
  if (std::fabs(i) < k_zeroRate) {
    *growth = 1.0;
    *annuity = n;
    return;
  }
  *growth = std::pow(1.0 + i, n);
  *annuity = (*growth - 1.0) / i;
}

static double balance(const double v[], double i) {
  double growth, annuity;
  growthAndAnnuity(i, v[N], &growth, &annuity);
  return v[PresentValue] * growth + v[Payment] * annuity + v[FutureValue];
}

// No closed form for the rate: bisection on the periodic rate, then back to I%.
static double solveInterestRate(const double v[]) {
  double low = -0.999999, high = 1.0;
  bool lowIsPositive = balance(v, low) > 0;
  while (high < 1e6 && (balance(v, high) > 0) == lowIsPositive) {
    high *= 2.0;
  }
  if ((balance(v, high) > 0) == lowIsPositive) {
    return NAN;
  }
  for (int k = 0; k < 200; k++) {
    double middle = (low + high) / 2.0;
    if ((balance(v, middle) > 0) == lowIsPositive) {
      low = middle;
    } else {
      high = middle;
    }
  }
  double i = (low + high) / 2.0;
  return 100.0 * v[CompoundingsPerYear] * (std::pow(1.0 + i, v[PaymentsPerYear] / v[CompoundingsPerYear]) - 1.0);
}

double SolveTVM(const double v[NumberOfVariables], Variable unknown) {
  if (!(v[PaymentsPerYear] > 0) || !(v[CompoundingsPerYear] > 0)) {
    return NAN;
  }
  if (unknown == InterestRate) {
    return solveInterestRate(v);
  }
  double i = std::pow(1.0 + v[InterestRate] / (100.0 * v[CompoundingsPerYear]), v[CompoundingsPerYear] / v[PaymentsPerYear]) - 1.0;
  if (unknown == N) {
    if (std::fabs(i) < k_zeroRate) {
      return -(v[PresentValue] + v[FutureValue]) / v[Payment];
    }
    double ratio = (v[Payment] - v[FutureValue] * i) / (v[Payment] + v[PresentValue] * i);
    return ratio > 0 ? std::log(ratio) / std::log(1.0 + i) : NAN;
  }
  double growth, annuity;
  growthAndAnnuity(i, v[N], &growth, &annuity);
  switch (unknown) {
    case PresentValue:
      return -(v[FutureValue] + v[Payment] * annuity) / growth;
    case Payment:
      return -(v[PresentValue] * growth + v[FutureValue]) / annuity;
    case FutureValue:
      return -(v[PresentValue] * growth + v[Payment] * annuity);
    default:
      return NAN;
  }
}

}

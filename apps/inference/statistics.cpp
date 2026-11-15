#include "statistics.h"
#include <poincare/normal_distribution.h>
#include <poincare/regularized_incomplete_beta_function.h>
#include <cmath>

namespace Inference {

static constexpr int k_parameters[NumberOfStatistics][MaxNumberOfParametersPerStatistic] = {
  {P0, Successes, SampleSize, -1},
  {Mu0, SampleMean, SampleDeviation, SampleSize},
  {Mu0, SampleMean, PopulationDeviation, SampleSize},
  {Successes, SampleSize, ConfidenceLevel, -1},
  {SampleMean, SampleDeviation, SampleSize, ConfidenceLevel},
  {SampleMean, PopulationDeviation, SampleSize, ConfidenceLevel},
};

bool IsTest(Statistic statistic) {
  return statistic <= TestMeanZ;
}

bool UsesStudent(Statistic statistic) {
  return statistic == TestMeanT || statistic == IntervalMeanT;
}

int NumberOfParametersFor(Statistic statistic) {
  int count = 0;
  while (count < MaxNumberOfParametersPerStatistic && k_parameters[statistic][count] >= 0) {
    count++;
  }
  return count;
}

Parameter ParameterFor(Statistic statistic, int index) {
  return static_cast<Parameter>(k_parameters[statistic][index]);
}

double StudentCumulativeDistribution(double t, double degreesOfFreedom) {
  // P(T > |t|) = I_{k/(k+t^2)}(k/2, 1/2) / 2
  double tail = 0.5 * Poincare::RegularizedIncompleteBetaFunction(degreesOfFreedom / 2.0, 0.5, degreesOfFreedom / (degreesOfFreedom + t * t));
  return t >= 0.0 ? 1.0 - tail : tail;
}

// degreesOfFreedom <= 0 means the standard normal distribution
static double cumulativeDistribution(double x, double degreesOfFreedom) {
  if (degreesOfFreedom > 0.0) {
    return StudentCumulativeDistribution(x, degreesOfFreedom);
  }
  return Poincare::NormalDistribution::CumulativeDistributiveFunctionAtAbscissa<double>(x, 0.0, 1.0);
}

static double inverseCumulativeDistribution(double probability, double degreesOfFreedom) {
  if (degreesOfFreedom <= 0.0) {
    return Poincare::NormalDistribution::CumulativeDistributiveInverseForProbability<double>(probability, 0.0, 1.0);
  }
  double low = -1.0, high = 1.0;
  while (low > -1e9 && StudentCumulativeDistribution(low, degreesOfFreedom) > probability) {
    low *= 2.0;
  }
  while (high < 1e9 && StudentCumulativeDistribution(high, degreesOfFreedom) < probability) {
    high *= 2.0;
  }
  for (int k = 0; k < 100; k++) {
    double middle = (low + high) / 2.0;
    if (StudentCumulativeDistribution(middle, degreesOfFreedom) < probability) {
      low = middle;
    } else {
      high = middle;
    }
  }
  return (low + high) / 2.0;
}

static void fillTest(double statistic, double degreesOfFreedom, double results[]) {
  double below = cumulativeDistribution(statistic, degreesOfFreedom);
  results[0] = statistic;
  results[1] = 2.0 * (below < 0.5 ? below : 1.0 - below);
  results[2] = below;
  results[3] = 1.0 - below;
}

static void fillInterval(double estimate, double standardError, double confidence, double degreesOfFreedom, double results[]) {
  double margin = inverseCumulativeDistribution((1.0 + confidence) / 2.0, degreesOfFreedom) * standardError;
  results[0] = estimate;
  results[1] = margin;
  results[2] = estimate - margin;
  results[3] = estimate + margin;
}

bool Compute(Statistic statistic, const double v[NumberOfParameters], double results[NumberOfResults]) {
  const double n = v[SampleSize];
  const double sqrtN = std::sqrt(n);
  const bool successesOK = v[Successes] >= 0.0 && v[Successes] <= n;
  const bool confidenceOK = v[ConfidenceLevel] > 0.0 && v[ConfidenceLevel] < 1.0;
  switch (statistic) {
    case TestProportion:
      if (!(n > 0.0 && successesOK && v[P0] > 0.0 && v[P0] < 1.0)) {
        return false;
      }
      fillTest((v[Successes] / n - v[P0]) / std::sqrt(v[P0] * (1.0 - v[P0]) / n), 0.0, results);
      return true;
    case TestMeanT:
      if (!(n >= 2.0 && v[SampleDeviation] > 0.0)) {
        return false;
      }
      fillTest((v[SampleMean] - v[Mu0]) / (v[SampleDeviation] / sqrtN), n - 1.0, results);
      return true;
    case TestMeanZ:
      if (!(n > 0.0 && v[PopulationDeviation] > 0.0)) {
        return false;
      }
      fillTest((v[SampleMean] - v[Mu0]) / (v[PopulationDeviation] / sqrtN), 0.0, results);
      return true;
    case IntervalProportion: {
      if (!(n > 0.0 && successesOK && confidenceOK)) {
        return false;
      }
      double p = v[Successes] / n;
      fillInterval(p, std::sqrt(p * (1.0 - p) / n), v[ConfidenceLevel], 0.0, results);
      return true;
    }
    case IntervalMeanT:
      if (!(n >= 2.0 && v[SampleDeviation] > 0.0 && confidenceOK)) {
        return false;
      }
      fillInterval(v[SampleMean], v[SampleDeviation] / sqrtN, v[ConfidenceLevel], n - 1.0, results);
      return true;
    case IntervalMeanZ:
      if (!(n > 0.0 && v[PopulationDeviation] > 0.0 && confidenceOK)) {
        return false;
      }
      fillInterval(v[SampleMean], v[PopulationDeviation] / sqrtN, v[ConfidenceLevel], 0.0, results);
      return true;
    default:
      return false;
  }
}

}

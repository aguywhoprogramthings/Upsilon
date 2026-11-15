#include "statistics.h"
#include "../probability/distribution/regularized_gamma.h"
#include <poincare/normal_distribution.h>
#include <poincare/regularized_incomplete_beta_function.h>
#include <cmath>
#include <float.h>

namespace Inference {

// Fixed inputs of the non chi-squared statistics, -1 terminated
static constexpr int k_maxFixedParameters = 7;
static constexpr int k_parameters[NumberOfStatistics][k_maxFixedParameters] = {
  {P0, Successes, SampleSize, -1},
  {Mu0, SampleMean, SampleDeviation, SampleSize, -1},
  {Mu0, SampleMean, PopulationDeviation, SampleSize, -1},
  {Successes, SampleSize, Successes2, SampleSize2, -1},
  {SampleMean, SampleDeviation, SampleSize, SampleMean2, SampleDeviation2, SampleSize2, -1},
  {SampleMean, PopulationDeviation, SampleSize, SampleMean2, PopulationDeviation2, SampleSize2, -1},
  {-1}, // TestChiSquaredGoodness: dynamic
  {-1}, // TestChiSquaredIndependence: dynamic
  {Successes, SampleSize, ConfidenceLevel, -1},
  {SampleMean, SampleDeviation, SampleSize, ConfidenceLevel, -1},
  {SampleMean, PopulationDeviation, SampleSize, ConfidenceLevel, -1},
  {Successes, SampleSize, Successes2, SampleSize2, ConfidenceLevel, -1},
  {SampleMean, SampleDeviation, SampleSize, SampleMean2, SampleDeviation2, SampleSize2, ConfidenceLevel},
  {SampleMean, PopulationDeviation, SampleSize, SampleMean2, PopulationDeviation2, SampleSize2, ConfidenceLevel},
};

bool IsTest(Statistic statistic) {
  return statistic < IntervalProportion;
}

bool IsChiSquared(Statistic statistic) {
  return statistic == TestChiSquaredGoodness || statistic == TestChiSquaredIndependence;
}

bool UsesStudent(Statistic statistic) {
  return statistic == TestMeanT || statistic == TestTwoMeansT || statistic == IntervalMeanT || statistic == IntervalTwoMeansT;
}

static bool isIntegerBetween(double x, int low, int high) {
  return x >= low && x <= high && std::floor(x) == x;
}

bool IsValidSize(Parameter parameter, double value) {
  if (parameter == Categories) {
    return isIntegerBetween(value, 2, MaxNumberOfCategories);
  }
  if (parameter == Rows || parameter == Columns) {
    return isIntegerBetween(value, 2, MaxTableSize);
  }
  return true;
}

// Sizes are validated on input; the clamp only guards stale or default values
static int boundedSize(const double values[], Parameter parameter, int maximum) {
  int n = static_cast<int>(values[parameter]);
  return n < 2 ? 2 : (n > maximum ? maximum : n);
}

int NumberOfParametersFor(Statistic statistic, const double values[NumberOfParameters]) {
  if (statistic == TestChiSquaredGoodness) {
    return 1 + 2 * boundedSize(values, Categories, MaxNumberOfCategories);
  }
  if (statistic == TestChiSquaredIndependence) {
    return 2 + boundedSize(values, Rows, MaxTableSize) * boundedSize(values, Columns, MaxTableSize);
  }
  int count = 0;
  while (count < k_maxFixedParameters && k_parameters[statistic][count] >= 0) {
    count++;
  }
  return count;
}

Parameter ParameterFor(Statistic statistic, int index, const double values[NumberOfParameters]) {
  if (statistic == TestChiSquaredGoodness) {
    // Categories, then O1, E1, O2, E2...
    if (index == 0) {
      return Categories;
    }
    int category = (index - 1) / 2;
    return static_cast<Parameter>(((index - 1) % 2 == 0 ? Observed0 : Expected0) + category);
  }
  if (statistic == TestChiSquaredIndependence) {
    // Rows, Columns, then the table row by row
    if (index < 2) {
      return index == 0 ? Rows : Columns;
    }
    int columns = boundedSize(values, Columns, MaxTableSize);
    int cell = index - 2;
    return static_cast<Parameter>(Cell0 + MaxTableSize * (cell / columns) + cell % columns);
  }
  return static_cast<Parameter>(k_parameters[statistic][index]);
}

int NumberOfResultsFor(Statistic statistic) {
  return IsChiSquared(statistic) ? 3 : NumberOfResults;
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

static bool fillChiSquared(double chiSquared, double degreesOfFreedom, double results[]) {
  double below;
  if (!regularizedGamma(degreesOfFreedom / 2.0, chiSquared / 2.0, DBL_EPSILON, 1000, &below)) {
    return false;
  }
  results[0] = chiSquared;
  results[1] = degreesOfFreedom;
  results[2] = 1.0 - below;
  results[3] = NAN;
  return true;
}

static bool chiSquaredGoodness(const double v[], double results[]) {
  int categories = boundedSize(v, Categories, MaxNumberOfCategories);
  double chiSquared = 0.0;
  for (int i = 0; i < categories; i++) {
    double observed = v[Observed0 + i], expected = v[Expected0 + i];
    if (!(observed >= 0.0 && expected > 0.0)) {
      return false;
    }
    chiSquared += (observed - expected) * (observed - expected) / expected;
  }
  return fillChiSquared(chiSquared, categories - 1, results);
}

static bool chiSquaredIndependence(const double v[], double results[]) {
  int rows = boundedSize(v, Rows, MaxTableSize), columns = boundedSize(v, Columns, MaxTableSize);
  double rowSums[MaxTableSize] = {}, columnSums[MaxTableSize] = {}, total = 0.0;
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {
      double n = v[Cell0 + MaxTableSize * i + j];
      if (!(n >= 0.0)) {
        return false;
      }
      rowSums[i] += n;
      columnSums[j] += n;
      total += n;
    }
  }
  double chiSquared = 0.0;
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {
      double expected = rowSums[i] * columnSums[j] / total;
      if (!(expected > 0.0)) {
        return false; // an empty row or column
      }
      double n = v[Cell0 + MaxTableSize * i + j];
      chiSquared += (n - expected) * (n - expected) / expected;
    }
  }
  return fillChiSquared(chiSquared, (rows - 1) * (columns - 1), results);
}

// Welch-Satterthwaite degrees of freedom for two sample variances
static double welchDegreesOfFreedom(double v1, double n1, double v2, double n2) {
  return (v1 + v2) * (v1 + v2) / (v1 * v1 / (n1 - 1.0) + v2 * v2 / (n2 - 1.0));
}

bool Compute(Statistic statistic, const double v[NumberOfParameters], double results[NumberOfResults]) {
  const double n = v[SampleSize], n2 = v[SampleSize2];
  const double sqrtN = std::sqrt(n);
  const bool successesOK = v[Successes] >= 0.0 && v[Successes] <= n;
  const bool successes2OK = n2 > 0.0 && v[Successes2] >= 0.0 && v[Successes2] <= n2;
  const bool confidenceOK = v[ConfidenceLevel] > 0.0 && v[ConfidenceLevel] < 1.0;
  // Variances of the two sample means
  const double varianceT = v[SampleDeviation] * v[SampleDeviation] / n + v[SampleDeviation2] * v[SampleDeviation2] / n2;
  const double varianceZ = v[PopulationDeviation] * v[PopulationDeviation] / n + v[PopulationDeviation2] * v[PopulationDeviation2] / n2;
  const bool twoMeansTOK = n >= 2.0 && n2 >= 2.0 && v[SampleDeviation] > 0.0 && v[SampleDeviation2] > 0.0;
  const bool twoMeansZOK = n > 0.0 && n2 > 0.0 && v[PopulationDeviation] > 0.0 && v[PopulationDeviation2] > 0.0;
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
    case TestTwoProportions: {
      if (!(n > 0.0 && successesOK && successes2OK)) {
        return false;
      }
      double pooled = (v[Successes] + v[Successes2]) / (n + n2);
      if (!(pooled > 0.0 && pooled < 1.0)) {
        return false;
      }
      fillTest((v[Successes] / n - v[Successes2] / n2) / std::sqrt(pooled * (1.0 - pooled) * (1.0 / n + 1.0 / n2)), 0.0, results);
      return true;
    }
    case TestTwoMeansT:
      if (!twoMeansTOK) {
        return false;
      }
      fillTest((v[SampleMean] - v[SampleMean2]) / std::sqrt(varianceT), welchDegreesOfFreedom(v[SampleDeviation] * v[SampleDeviation] / n, n, v[SampleDeviation2] * v[SampleDeviation2] / n2, n2), results);
      return true;
    case TestTwoMeansZ:
      if (!twoMeansZOK) {
        return false;
      }
      fillTest((v[SampleMean] - v[SampleMean2]) / std::sqrt(varianceZ), 0.0, results);
      return true;
    case TestChiSquaredGoodness:
      return chiSquaredGoodness(v, results);
    case TestChiSquaredIndependence:
      return chiSquaredIndependence(v, results);
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
    case IntervalTwoProportions: {
      if (!(n > 0.0 && successesOK && successes2OK && confidenceOK)) {
        return false;
      }
      double p1 = v[Successes] / n, p2 = v[Successes2] / n2;
      fillInterval(p1 - p2, std::sqrt(p1 * (1.0 - p1) / n + p2 * (1.0 - p2) / n2), v[ConfidenceLevel], 0.0, results);
      return true;
    }
    case IntervalTwoMeansT:
      if (!(twoMeansTOK && confidenceOK)) {
        return false;
      }
      fillInterval(v[SampleMean] - v[SampleMean2], std::sqrt(varianceT), v[ConfidenceLevel], welchDegreesOfFreedom(v[SampleDeviation] * v[SampleDeviation] / n, n, v[SampleDeviation2] * v[SampleDeviation2] / n2, n2), results);
      return true;
    case IntervalTwoMeansZ:
      if (!(twoMeansZOK && confidenceOK)) {
        return false;
      }
      fillInterval(v[SampleMean] - v[SampleMean2], std::sqrt(varianceZ), v[ConfidenceLevel], 0.0, results);
      return true;
    default:
      return false;
  }
}

}

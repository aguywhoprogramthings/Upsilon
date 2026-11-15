#ifndef INFERENCE_STATISTICS_H
#define INFERENCE_STATISTICS_H

namespace Inference {

constexpr int MaxNumberOfCategories = 8; // chi-squared goodness of fit
constexpr int MaxTableSize = 4;          // chi-squared independence, rows and columns

// Inputs are shared by all statistics, so switching keeps what was typed.
enum Parameter {
  P0,
  Successes,
  SampleSize,
  Mu0,
  SampleMean,
  SampleDeviation,
  PopulationDeviation,
  ConfidenceLevel,
  Successes2,
  SampleSize2,
  SampleMean2,
  SampleDeviation2,
  PopulationDeviation2,
  Categories,
  Observed0, // Observed0 + i, i < MaxNumberOfCategories
  Expected0 = Observed0 + MaxNumberOfCategories,
  Rows = Expected0 + MaxNumberOfCategories,
  Columns,
  Cell0, // Cell0 + MaxTableSize * row + column
  NumberOfParameters = Cell0 + MaxTableSize * MaxTableSize
};

enum Statistic {
  TestProportion,
  TestMeanT,
  TestMeanZ,
  TestTwoProportions,
  TestTwoMeansT,
  TestTwoMeansZ,
  TestChiSquaredGoodness,
  TestChiSquaredIndependence,
  IntervalProportion,
  IntervalMeanT,
  IntervalMeanZ,
  IntervalTwoProportions,
  IntervalTwoMeansT,
  IntervalTwoMeansZ,
  NumberOfStatistics
};

constexpr int MaxNumberOfParametersPerStatistic = 2 + MaxTableSize * MaxTableSize;
constexpr int NumberOfResults = 4;

bool IsTest(Statistic statistic);
bool IsChiSquared(Statistic statistic);
bool UsesStudent(Statistic statistic);
// The chi-squared input lists grow with the number of classes, rows and columns
int NumberOfParametersFor(Statistic statistic, const double values[NumberOfParameters]);
Parameter ParameterFor(Statistic statistic, int index, const double values[NumberOfParameters]);
int NumberOfResultsFor(Statistic statistic);
// Number of classes, rows or columns: false if out of its range
bool IsValidSize(Parameter parameter, double value);

/* Tests: statistic, p-value for H1 "≠", "<", ">".
 * Chi-squared: chi2, degrees of freedom, p-value.
 * Intervals: estimate, margin of error, lower bound, upper bound.
 * Returns false when the inputs are out of range. */
bool Compute(Statistic statistic, const double values[NumberOfParameters], double results[NumberOfResults]);

double StudentCumulativeDistribution(double t, double degreesOfFreedom);

}

#endif

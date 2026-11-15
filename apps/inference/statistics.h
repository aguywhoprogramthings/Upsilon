#ifndef INFERENCE_STATISTICS_H
#define INFERENCE_STATISTICS_H

namespace Inference {

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
  NumberOfParameters
};

enum Statistic {
  TestProportion,
  TestMeanT,
  TestMeanZ,
  IntervalProportion,
  IntervalMeanT,
  IntervalMeanZ,
  NumberOfStatistics
};

constexpr int MaxNumberOfParametersPerStatistic = 4;
constexpr int NumberOfResults = 4;

bool IsTest(Statistic statistic);
bool UsesStudent(Statistic statistic);
int NumberOfParametersFor(Statistic statistic);
Parameter ParameterFor(Statistic statistic, int index);

/* Tests: statistic, p-value for H1 "≠", "<", ">".
 * Intervals: estimate, margin of error, lower bound, upper bound.
 * Returns false when the inputs are out of range. */
bool Compute(Statistic statistic, const double values[NumberOfParameters], double results[NumberOfResults]);

double StudentCumulativeDistribution(double t, double degreesOfFreedom);

}

#endif

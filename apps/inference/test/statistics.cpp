#include <quiz.h>
#include <cmath>
#include "../statistics.h"

using namespace Inference;

static bool approximatelyEqual(double x, double y, double tolerance) {
  return std::fabs(x - y) <= tolerance * (1.0 + std::fabs(y));
}

static void assert_computes(Statistic statistic, const double values[NumberOfParameters], const double expected[NumberOfResults], double tolerance) {
  double results[NumberOfResults];
  quiz_assert(Compute(statistic, values, results));
  for (int i = 0; i < NumberOfResults; i++) {
    quiz_assert(approximatelyEqual(results[i], expected[i], tolerance));
  }
}

QUIZ_CASE(inference_student_distribution) {
  quiz_assert(approximatelyEqual(StudentCumulativeDistribution(2.262157163, 9), 0.975, 1e-6));
  quiz_assert(approximatelyEqual(StudentCumulativeDistribution(-2.262157163, 9), 0.025, 1e-6));
  quiz_assert(approximatelyEqual(StudentCumulativeDistribution(12.70620474, 1), 0.975, 1e-6));
}

QUIZ_CASE(inference_tests) {
  //                          P0   x   n    mu0 mean  s  sigma C
  double values[NumberOfParameters] = {0.5, 60, 100, 10, 10.5, 1, 1, 0.95};
  const double proportion[] = {2.0, 0.04550026389635864, 0.9772498680518207, 0.02275013194817932};
  assert_computes(TestProportion, values, proportion, 1e-6);
  values[SampleSize] = 16;
  const double meanT[] = {2.0, 0.06394500728471875, 0.9680274963576406, 0.03197250364235937};
  assert_computes(TestMeanT, values, meanT, 1e-6);
}

QUIZ_CASE(inference_intervals) {
  double values[NumberOfParameters] = {0.5, 60, 100, 10, 10.5, 1, 1, 0.95};
  const double proportion[] = {0.6, 0.09601823352710621, 0.5039817664728937, 0.6960182335271062};
  assert_computes(IntervalProportion, values, proportion, 1e-6);
  values[SampleSize] = 16;
  const double meanT[] = {10.5, 0.532862386389942, 9.967137613610058, 11.032862386389942};
  assert_computes(IntervalMeanT, values, meanT, 1e-6);
  const double meanZ[] = {10.5, 0.4899909961350136, 10.010009003864987, 10.989990996135013};
  assert_computes(IntervalMeanZ, values, meanZ, 1e-6);
  double results[NumberOfResults];
  values[ConfidenceLevel] = 1.5;
  quiz_assert(!Compute(IntervalMeanZ, values, results));
}

QUIZ_CASE(inference_two_samples) {
  double v[NumberOfParameters] = {};
  v[Successes] = 60; v[SampleSize] = 100; v[Successes2] = 45; v[SampleSize2] = 100; v[ConfidenceLevel] = 0.95;
  const double twoProportions[] = {2.1239769762143657, 0.03367206885634588, 0.9831639655718271, 0.01683603442817294};
  assert_computes(TestTwoProportions, v, twoProportions, 1e-6);
  const double twoProportionsInterval[] = {0.15, 0.13684703778629576, 0.15 - 0.13684703778629576, 0.15 + 0.13684703778629576};
  assert_computes(IntervalTwoProportions, v, twoProportionsInterval, 1e-6);
  // Welch t-test: unequal variances, df = 16.748
  v[SampleMean] = 5; v[SampleDeviation] = 1; v[SampleSize] = 10;
  v[SampleMean2] = 4; v[SampleDeviation2] = 2; v[SampleSize2] = 12;
  v[PopulationDeviation] = 1; v[PopulationDeviation2] = 2;
  const double welch[] = {1.5191090506254998, 0.14738714851298473, 0.9263064257435076, 0.07369357425649237};
  assert_computes(TestTwoMeansT, v, welch, 1e-6);
  const double twoMeansZ[] = {1.5191090506254998, 0.12873504895794796, 0.935632475521026, 0.06436752447897398};
  assert_computes(TestTwoMeansZ, v, twoMeansZ, 1e-6);
}

QUIZ_CASE(inference_chi_squared) {
  double v[NumberOfParameters] = {};
  double results[NumberOfResults];
  v[Categories] = 4;
  const double observed[] = {18, 22, 20, 40};
  for (int i = 0; i < 4; i++) {
    v[Observed0 + i] = observed[i];
    v[Expected0 + i] = 25;
  }
  quiz_assert(Compute(TestChiSquaredGoodness, v, results));
  quiz_assert(approximatelyEqual(results[0], 12.32, 1e-9) && results[1] == 3.0 && approximatelyEqual(results[2], 0.006363629995195241, 1e-6));
  // 2x2 table: with 1 degree of freedom, chi2 = z^2
  v[Rows] = 2; v[Columns] = 2;
  v[Cell0] = 10; v[Cell0 + 1] = 20; v[Cell0 + MaxTableSize] = 30; v[Cell0 + MaxTableSize + 1] = 40;
  quiz_assert(Compute(TestChiSquaredIndependence, v, results));
  quiz_assert(approximatelyEqual(results[0], 0.7936507936507936, 1e-9) && results[1] == 1.0 && approximatelyEqual(results[2], 0.3729984836134872, 1e-6));
  // Inputs grow with the sizes: k, then O1, E1, O2... and r, c, then the table row by row
  quiz_assert(NumberOfParametersFor(TestChiSquaredGoodness, v) == 9 && ParameterFor(TestChiSquaredGoodness, 2, v) == Expected0);
  quiz_assert(NumberOfParametersFor(TestChiSquaredIndependence, v) == 6 && ParameterFor(TestChiSquaredIndependence, 4, v) == Cell0 + MaxTableSize);
  quiz_assert(!IsValidSize(Categories, 9) && IsValidSize(Rows, 4) && !IsValidSize(Columns, 2.5));
  v[Expected0] = 0;
  quiz_assert(!Compute(TestChiSquaredGoodness, v, results));
}

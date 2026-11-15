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

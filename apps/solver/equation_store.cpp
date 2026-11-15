#include "equation_store.h"
#include "../constant.h"
#include "../shared/poincare_helpers.h"
#include "../exam_mode_configuration.h"
#include "../global_preferences.h"
#include <limits.h>

#include <poincare/constant.h>
#include <poincare/symbol.h>
#include <poincare/matrix.h>
#include <poincare/rational.h>
#include <poincare/opposite.h>
#include <poincare/addition.h>
#include <poincare/subtraction.h>
#include <poincare/multiplication.h>
#include <poincare/division.h>
#include <poincare/square_root.h>
#include <poincare/power.h>
#include <poincare/undefined.h>
#include <poincare/decimal.h>
#include <ion/unicode/code_point.h>
#include <algorithm>
#include <cmath>
#include <stdint.h>

using namespace Poincare;
using namespace Shared;

namespace Solver {

EquationStore::EquationStore() :
  ExpressionModelStore(),
  m_type(Type::LinearSystem),
  m_numberOfSolutions(0),
  m_exactSolutionExactLayouts{},
  m_exactSolutionApproximateLayouts{},
  m_numberOfUserVariables(0)
{
}

Ion::Storage::Record::ErrorStatus EquationStore::addEmptyModel() {
  char name[3] = {'e', '?', 0}; // name is going to be e0 or e1 or ... e5
  int currentNumber = 0;
  while (currentNumber < k_maxNumberOfEquations) {
    name[1] = '0'+currentNumber;
    if (Ion::Storage::sharedStorage()->recordBaseNamedWithExtension(name, Ion::Storage::eqExtension).isNull()) {
      break;
    }
    currentNumber++;
  }
  assert(currentNumber < k_maxNumberOfEquations);
  return Ion::Storage::sharedStorage()->createRecordWithExtension(name, Ion::Storage::eqExtension, nullptr, 0);
}

Shared::ExpressionModelHandle * EquationStore::setMemoizedModelAtIndex(int cacheIndex, Ion::Storage::Record record) const {
  assert(cacheIndex >= 0 && cacheIndex < maxNumberOfMemoizedModels());
  m_equations[cacheIndex] = Equation(record);
  return &m_equations[cacheIndex];
}

ExpressionModelHandle * EquationStore::memoizedModelAtIndex(int cacheIndex) const {
  assert(cacheIndex >= 0 && cacheIndex < maxNumberOfMemoizedModels());
  return &m_equations[cacheIndex];
}

void EquationStore::tidy() {
  ExpressionModelStore::tidy();
  tidySolution();
}

Poincare::Layout EquationStore::exactSolutionLayoutAtIndex(int i, bool exactLayout) {
  assert(m_type != Type::Monovariable && i >= 0 && (i < m_numberOfSolutions));
  if (exactLayout) {
    return m_exactSolutionExactLayouts[i];
  } else {
    return m_exactSolutionApproximateLayouts[i];
  }
}

double EquationStore::intervalBound(int index) const {
  assert(m_type == Type::Monovariable && index >= 0 && index < 2);
  return m_intervalApproximateSolutions[index];
}

void EquationStore::setIntervalBound(int index, double value) {
  assert(m_type == Type::Monovariable && index >= 0 && index < 2);
  m_intervalApproximateSolutions[index] = value;
  if (m_intervalApproximateSolutions[0] > m_intervalApproximateSolutions[1]) {
    if (index == 0) {
      m_intervalApproximateSolutions[1] = m_intervalApproximateSolutions[0]+1;
    } else {
      m_intervalApproximateSolutions[0] = m_intervalApproximateSolutions[1]-1;
    }
  }
}

void EquationStore::approximateSolve(Poincare::Context * context, bool shouldReplaceFunctionsButNotSymbols) {
  m_hasMoreThanMaxNumberOfApproximateSolution = false;
  Expression undevelopedExpression = modelForRecord(definedRecordAtIndex(0))->standardForm(context, shouldReplaceFunctionsButNotSymbols, ExpressionNode::ReductionTarget::SystemForApproximation);
  m_userVariablesUsed = !shouldReplaceFunctionsButNotSymbols;
  assert(m_variables[0][0] != 0 && m_variables[1][0] == 0);
  assert(m_type == Type::Monovariable);
  m_numberOfSolutions = 0;
  double start = m_intervalApproximateSolutions[0];
  double step = (m_intervalApproximateSolutions[1]-m_intervalApproximateSolutions[0])*k_precision;
  double root;
  for (int i = 0; i <= k_maxNumberOfApproximateSolutions; i++) {
    root = PoincareHelpers::NextRoot(undevelopedExpression, m_variables[0], start, step, m_intervalApproximateSolutions[1], context);
    if (i == k_maxNumberOfApproximateSolutions) {
      m_hasMoreThanMaxNumberOfApproximateSolution = !std::isnan(root);
      break;
    }
    m_approximateSolutions[i] = root;
    if (std::isnan(m_approximateSolutions[i])) {
      break;
    } else {
      start = m_approximateSolutions[i];
      m_numberOfSolutions++;
    }
  }
}

EquationStore::Error EquationStore::exactSolve(Poincare::Context * context, bool * replaceFunctionsButNotSymbols) {
  assert(replaceFunctionsButNotSymbols != nullptr);
  *replaceFunctionsButNotSymbols = false;
  Error e = privateExactSolve(context, false);
  if (m_numberOfUserVariables > 0 && (e != Error::NoError || numberOfSolutions() == 0)) {
    *replaceFunctionsButNotSymbols = true;
    e = privateExactSolve(context, true);
  }
  return e;
}

/* Equations are solved according to the following procedure :
 * 1) We develop the equations using the reduction target "SystemForAnalysis".
 * This expands structures like Newton multinoms and allows us to detect
 * polynoms afterwards. ("(x+2)^2" in this form is not detected but is if
 * expanded).
 * 2) We look for classic forms of equations for which we have algorithms
 * that output the exact answer. If one is recognized in the input equation,
 * the exact answer is given to the user.
 * 3) If no classic form has been found in the developped form, we need to use
 * numerical approximation. Therefore, to prevent precision losses, we work
 * with the undeveloped form of the equation. Therefore we set reductionTarget
 * to SystemForApproximation. Solutions are then numerically approximated
 * between the bounds provided by the user. */

EquationStore::Error EquationStore::privateExactSolve(Poincare::Context * context, bool replaceFunctionsButNotSymbols) {
  tidySolution();

  m_userVariablesUsed = !replaceFunctionsButNotSymbols;

  // Step 1. Get unknown and user-defined variables
  m_variables[0][0] = 0;
  int numberOfVariables = 0;

  // TODO we look twice for variables but not the same, is there a way to not do the same work twice?
  m_userVariables[0][0] = 0;
  m_numberOfUserVariables = 0;
  Expression simplifiedExpressions[k_maxNumberOfEquations];

  for (int i = 0; i < numberOfDefinedModels(); i++) {
    Shared::ExpiringPointer<Equation> eq = modelForRecord(definedRecordAtIndex(i));

    /* Start by looking for user variables, so that if we escape afterwards, we
     * know  if it might be due to a user variable. */
    if (m_numberOfUserVariables < Expression::k_maxNumberOfVariables) {
      const Expression eWithSymbols = eq->standardForm(context, true, ExpressionNode::ReductionTarget::SystemForAnalysis);
      /* if replaceFunctionsButNotSymbols is true we can memoize the expressions
       * for the rest of the function. Otherwise, we will memoize them at the
       * next call to standardForm*/
      if (replaceFunctionsButNotSymbols == true) {
        simplifiedExpressions[i] = eWithSymbols;
      }
      int varCount = eWithSymbols.getVariables(context, [](const char * symbol, Poincare::Context * context) { return context->expressionTypeForIdentifier(symbol, strlen(symbol)) == Poincare::Context::SymbolAbstractType::Symbol; }, (char *)m_userVariables, Poincare::SymbolAbstract::k_maxNameSize, m_numberOfUserVariables);
      m_numberOfUserVariables = varCount < 0 ? Expression::k_maxNumberOfVariables : varCount;
    }
    if (simplifiedExpressions[i].isUninitialized()) {
      // The expression was not memoized before.
      simplifiedExpressions[i] = eq->standardForm(context, replaceFunctionsButNotSymbols, ExpressionNode::ReductionTarget::SystemForAnalysis);
    }
    const Expression e = simplifiedExpressions[i];
    if (e.isUninitialized() || e.type() == ExpressionNode::Type::Undefined || e.recursivelyMatches(Expression::IsMatrix, context, replaceFunctionsButNotSymbols ? ExpressionNode::SymbolicComputation::ReplaceDefinedFunctionsWithDefinitions : ExpressionNode::SymbolicComputation::ReplaceAllDefinedSymbolsWithDefinition)) {
      return Error::EquationUndefined;
    }
    if (e.type() == ExpressionNode::Type::Unreal) {
      return Error::EquationUnreal;
    }
    numberOfVariables = e.getVariables(context, [](const char * symbol, Poincare::Context * context) { return true; }, (char *)m_variables, Poincare::SymbolAbstract::k_maxNameSize, numberOfVariables);
    if (numberOfVariables == -1) {
      return Error::TooManyVariables;
    }
    /* The equation has been parsed so there should be no
     * Error::VariableNameTooLong*/
    assert(numberOfVariables >= 0);
  }

  // Step 2. Linear System?

  /* Create matrix coefficients and vector constants as:
   *   coefficients * (x y z ...) = constants */
  Expression coefficients[k_maxNumberOfEquations][Expression::k_maxNumberOfVariables];
  Expression constants[k_maxNumberOfEquations];
  bool isLinear = true; // Invalid the linear system if one equation is non-linear
  Preferences * preferences = Preferences::sharedPreferences();
  for (int i = 0; i < numberOfDefinedModels(); i++) {
    isLinear = isLinear && simplifiedExpressions[i].getLinearCoefficients((char *)m_variables, Poincare::SymbolAbstract::k_maxNameSize, coefficients[i], &constants[i], context, updatedComplexFormat(context), preferences->angleUnit(), GlobalPreferences::sharedGlobalPreferences()->unitFormat(), replaceFunctionsButNotSymbols ? ExpressionNode::SymbolicComputation::ReplaceDefinedFunctionsWithDefinitions : ExpressionNode::SymbolicComputation::ReplaceAllDefinedSymbolsWithDefinition);
    if (!isLinear) {
    // TODO: should we clean pool allocated memory if the system is not linear
#if 0
      for (int j = 0; j < i; j++) {
        for (int k = 0; k < numberOfVariables; k++) {
          coefficients[j][k] = Expression();
        }
        constants[j] = Expression();
      }
#endif
      if (numberOfDefinedModels() > 1 || numberOfVariables > 1) {
        return Error::NonLinearSystem;
      } else {
        break;
      }
    }
  }

  // Initialize result
  Expression exactSolutions[k_maxNumberOfExactSolutions];
  Expression exactSolutionsApproximations[k_maxNumberOfExactSolutions];
  EquationStore::Error error;

  if (isLinear) {
    m_type = Type::LinearSystem;
    error = resolveLinearSystem(exactSolutions, exactSolutionsApproximations, coefficients, constants, context);
  } else {
    // Step 3. Polynomial & Monovariable?
    assert(numberOfVariables == 1 && numberOfDefinedModels() == 1);
    Expression polynomialCoefficients[Expression::k_maxNumberOfPolynomialCoefficients];
    int degree = simplifiedExpressions[0]
      .getPolynomialReducedCoefficients(
          m_variables[0],
          polynomialCoefficients,
          context,
          updatedComplexFormat(context),
          preferences->angleUnit(),
          GlobalPreferences::sharedGlobalPreferences()->unitFormat(),
          replaceFunctionsButNotSymbols ?
            ExpressionNode::SymbolicComputation::ReplaceDefinedFunctionsWithDefinitions :
            ExpressionNode::SymbolicComputation::ReplaceAllDefinedSymbolsWithDefinition);
    error = Error::RequireApproximateSolution;
    m_polynomialDegree = degree;
    if (degree == 2) {
      m_type = Type::PolynomialMonovariable;
      error = oneDimensionalPolynomialSolve(exactSolutions, exactSolutionsApproximations, polynomialCoefficients, degree, context);
    } else if (degree == 3) {
      m_type = Type::PolynomialMonovariable;
      error = cubicPolynomialSolve(exactSolutions, exactSolutionsApproximations, polynomialCoefficients, context);
    }
    if (error == Error::RequireApproximateSolution) {
      /* Step 4. Monovariable non-polynomial, polynomial with degree > 3 or
       * cubic with non-real coefficients */
      m_type = Type::Monovariable;
      m_intervalApproximateSolutions[0] = -10;
      m_intervalApproximateSolutions[1] = 10;
      return Error::RequireApproximateSolution;
    }
  }
  // Create the results' layouts
  // Some exam mode configuration requires to display only approximate solutions
  bool forbidExactSolutions = ExamModeConfiguration::exactExpressionsAreForbidden(GlobalPreferences::sharedGlobalPreferences()->examMode());
  int solutionIndex = 0;
  int initialNumberOfSolutions = m_numberOfSolutions <= k_maxNumberOfExactSolutions ? m_numberOfSolutions : -1;
  // We iterate through the solutions and the potential delta
  for (int i = 0; i < initialNumberOfSolutions; i++) {
    if (!exactSolutions[i].isUninitialized()) {
      assert(!exactSolutionsApproximations[i].isUninitialized());
      if (exactSolutionsApproximations[i].type() == ExpressionNode::Type::Unreal) {
        // Discard unreal solutions.
        m_numberOfSolutions--;
        continue;
      }
      m_exactSolutionExactLayouts[solutionIndex] = PoincareHelpers::CreateLayout(exactSolutions[i]);
      m_exactSolutionApproximateLayouts[solutionIndex] = PoincareHelpers::CreateLayout(exactSolutionsApproximations[i]);
      // Check for identity between exact and approximate layouts
      char exactBuffer[::Constant::MaxSerializedExpressionSize];
      char approximateBuffer[::Constant::MaxSerializedExpressionSize];
      m_exactSolutionExactLayouts[solutionIndex].serializeForParsing(exactBuffer, ::Constant::MaxSerializedExpressionSize);
      m_exactSolutionApproximateLayouts[solutionIndex].serializeForParsing(approximateBuffer, ::Constant::MaxSerializedExpressionSize);
      /* Cheat: declare exact and approximate solutions to be identical in when
       * 'forbidExactSolutions' is true to display only the approximate
       * solutions. */
      m_exactSolutionIdentity[solutionIndex] = forbidExactSolutions || strcmp(exactBuffer, approximateBuffer) == 0;
      if (!m_exactSolutionIdentity[solutionIndex]) {
        m_exactSolutionEquality[solutionIndex] = Expression::ParsedExpressionsAreEqual(exactBuffer, approximateBuffer, context, updatedComplexFormat(context), preferences->angleUnit(), GlobalPreferences::sharedGlobalPreferences()->unitFormat());
      }
      solutionIndex++;
    }
  }
  return error;
}

EquationStore::Error EquationStore::resolveLinearSystem(Expression exactSolutions[k_maxNumberOfExactSolutions], Expression exactSolutionsApproximations[k_maxNumberOfExactSolutions], Expression coefficients[k_maxNumberOfEquations][Expression::k_maxNumberOfVariables], Expression constants[k_maxNumberOfEquations], Context * context) {
  Preferences::AngleUnit angleUnit = Preferences::sharedPreferences()->angleUnit();
  Preferences::UnitFormat unitFormat = GlobalPreferences::sharedGlobalPreferences()->unitFormat();
  // n unknown variables
  int n = 0;
  while (n < Expression::k_maxNumberOfVariables && m_variables[n][0] != 0) {
    n++;
  }
  int m = numberOfDefinedModels(); // m equations
  /* Create the matrix (A | b) for the equation Ax=b */
  Matrix Ab = Matrix::Builder();
  for (int i = 0; i < m; i++) {
    for (int j = 0; j < n; j++) {
      Ab.addChildAtIndexInPlace(coefficients[i][j], Ab.numberOfChildren(), Ab.numberOfChildren());
    }
    Ab.addChildAtIndexInPlace(constants[i], Ab.numberOfChildren(), Ab.numberOfChildren());
  }
  Ab.setDimensions(m, n+1);

  // Compute the rank of (A | b)
  int rankAb = Ab.rank(context, updatedComplexFormat(context), angleUnit, unitFormat, true);

  // Initialize the number of solutions
  m_numberOfSolutions = INT_MAX;
  /* If the matrix has one null row except the last column, the system is
   * inconsistent (equivalent to 0 = x with x non-null */
  for (int j = m-1; j >= 0; j--) {
    bool rowWithNullCoefficients = true;
    for (int i = 0; i < n; i++) {
      if (Ab.matrixChild(j, i).nullStatus(context) != ExpressionNode::NullStatus::Null) {
        rowWithNullCoefficients = false;
        break;
      }
    }
    if (rowWithNullCoefficients && Ab.matrixChild(j, n).nullStatus(context) != ExpressionNode::NullStatus::Null) {
      // TODO: Handle ExpressionNode::NullStatus::Unknown
      m_numberOfSolutions = 0;
    }
  }
  if (m_numberOfSolutions > 0) {
    // if rank(A | b) < n, the system has an infinite number of solutions
    if (rankAb == n && n > 0) {
      // Otherwise, the system has n solutions which correspond to the last column
      m_numberOfSolutions = n;
      for (int i = 0; i < m_numberOfSolutions; i++) {
        exactSolutions[i] = Ab.matrixChild(i,n);
        exactSolutions[i].simplifyAndApproximate(&exactSolutions[i], &exactSolutionsApproximations[i], context, updatedComplexFormat(context), angleUnit, unitFormat);
      }
    }
  }
  return Error::NoError;
}

EquationStore::Error EquationStore::oneDimensionalPolynomialSolve(Expression exactSolutions[k_maxNumberOfExactSolutions], Expression exactSolutionsApproximations[k_maxNumberOfExactSolutions], Expression coefficients[Expression::k_maxNumberOfPolynomialCoefficients], int degree, Context * context) {
  /* Equation ax^2+bx+c = 0 */
  assert(degree == 2);
  // Compute delta = b*b-4ac
  Expression delta = Subtraction::Builder(Power::Builder(coefficients[1].clone(), Rational::Builder(2)), Multiplication::Builder(Rational::Builder(4), coefficients[0].clone(), coefficients[2].clone()));
  delta = delta.simplify(ExpressionNode::ReductionContext(context, updatedComplexFormat(context), Poincare::Preferences::sharedPreferences()->angleUnit(), GlobalPreferences::sharedGlobalPreferences()->unitFormat(), ExpressionNode::ReductionTarget::SystemForApproximation));
  if (delta.isUninitialized()) {
    delta = Poincare::Undefined::Builder();
  }
  if (delta.nullStatus(context) == ExpressionNode::NullStatus::Null) {
    // if delta = 0, x0=x1= -b/(2a)
    exactSolutions[0] = Division::Builder(Opposite::Builder(coefficients[1]), Multiplication::Builder(Rational::Builder(2), coefficients[2]));
    m_numberOfSolutions = 2;
  } else {
    // TODO: Handle ExpressionNode::NullStatus::Unknown
    // x0 = (-b-sqrt(delta))/(2a)
    exactSolutions[0] = Division::Builder(Subtraction::Builder(Opposite::Builder(coefficients[1].clone()), SquareRoot::Builder(delta.clone())), Multiplication::Builder(Rational::Builder(2), coefficients[2].clone()));
    // x1 = (-b+sqrt(delta))/(2a)
    exactSolutions[1] = Division::Builder(Addition::Builder(Opposite::Builder(coefficients[1]), SquareRoot::Builder(delta.clone())), Multiplication::Builder(Rational::Builder(2), coefficients[2]));
    m_numberOfSolutions = 3;
  }
  exactSolutions[m_numberOfSolutions-1] = delta;
  for (int i = 0; i < m_numberOfSolutions; i++) {
    exactSolutions[i].simplifyAndApproximate(&exactSolutions[i], &exactSolutionsApproximations[i], context, updatedComplexFormat(context), Poincare::Preferences::sharedPreferences()->angleUnit(), GlobalPreferences::sharedGlobalPreferences()->unitFormat());
  }
  return Error::NoError;
}

// Cubic: numeric roots first, then an exact rational root if there is one

static double cubicValue(const double c[4], double x) {
  return ((c[3] * x + c[2]) * x + c[1]) * x + c[0];
}

/* Real roots of c[3]x^3+c[2]x^2+c[1]x+c[0] in increasing order. Returns 3, or 1
 * when the two other roots are the complex conjugates re +/- i*im. */
static int cubicNumericRoots(const double c[4], double roots[3], double * re, double * im) {
  double bound = 1.0 + std::max(std::fabs(c[2]), std::max(std::fabs(c[1]), std::fabs(c[0]))) / std::fabs(c[3]);
  double low = -bound, high = bound;
  bool lowIsPositive = cubicValue(c, low) > 0.0;
  for (int k = 0; k < 200; k++) {
    double middle = (low + high) / 2.0;
    if ((cubicValue(c, middle) > 0.0) == lowIsPositive) {
      low = middle;
    } else {
      high = middle;
    }
  }
  roots[0] = (low + high) / 2.0;
  // Deflate to q2*x^2+q1*x+q0
  double q2 = c[3];
  double q1 = c[2] + q2 * roots[0];
  double q0 = c[1] + q1 * roots[0];
  double discriminant = q1 * q1 - 4.0 * q2 * q0;
  if (discriminant < 0.0) {
    *re = -q1 / (2.0 * q2);
    *im = std::sqrt(-discriminant) / (2.0 * std::fabs(q2));
    return 1;
  }
  roots[1] = (-q1 - std::sqrt(discriminant)) / (2.0 * q2);
  roots[2] = (-q1 + std::sqrt(discriminant)) / (2.0 * q2);
  for (int i = 0; i < 3; i++) {
    // Newton steps on the cubic itself remove the deflation error
    for (int k = 0; k < 3; k++) {
      double derivative = (3.0 * c[3] * roots[i] + 2.0 * c[2]) * roots[i] + c[1];
      if (derivative != 0.0) {
        roots[i] -= cubicValue(c, roots[i]) / derivative;
      }
    }
  }
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2 - i; j++) {
      if (roots[j] > roots[j + 1]) {
        double tmp = roots[j];
        roots[j] = roots[j + 1];
        roots[j + 1] = tmp;
      }
    }
  }
  return 3;
}

// Continued fractions: p/q with q <= 1000 matching x, if any
static bool rationalApproximation(double x, native_int_t * numerator, native_int_t * denominator) {
  constexpr int64_t k_maxDenominator = 1000;
  if (!(std::fabs(x) < 1e6)) {
    return false;
  }
  int64_t h0 = 1, h1 = 0, k0 = 0, k1 = 1;
  double v = x;
  for (int i = 0; i < 20; i++) {
    double a = std::floor(v);
    int64_t h2 = static_cast<int64_t>(a) * h0 + h1;
    int64_t k2 = static_cast<int64_t>(a) * k0 + k1;
    if (k2 > k_maxDenominator) {
      return false;
    }
    h1 = h0;
    h0 = h2;
    k1 = k0;
    k0 = k2;
    if (std::fabs(x - static_cast<double>(h0) / static_cast<double>(k0)) <= 1e-9 * (1.0 + std::fabs(x))) {
      *numerator = static_cast<native_int_t>(h0);
      *denominator = static_cast<native_int_t>(k0);
      return true;
    }
    double fractional = v - a;
    if (fractional < 1e-12) {
      return false;
    }
    v = 1.0 / fractional;
  }
  return false;
}

EquationStore::Error EquationStore::cubicPolynomialSolve(Expression exactSolutions[k_maxNumberOfExactSolutions], Expression exactSolutionsApproximations[k_maxNumberOfExactSolutions], Expression coefficients[Expression::k_maxNumberOfPolynomialCoefficients], Context * context) {
  /* Equation ax^3+bx^2+cx+d = 0, with coefficients[3] = a ... coefficients[0] = d */
  Preferences::ComplexFormat complexFormat = updatedComplexFormat(context);
  Preferences::AngleUnit angleUnit = Preferences::sharedPreferences()->angleUnit();
  Preferences::UnitFormat unitFormat = GlobalPreferences::sharedGlobalPreferences()->unitFormat();
  ExpressionNode::ReductionContext reductionContext(context, complexFormat, angleUnit, unitFormat, ExpressionNode::ReductionTarget::SystemForApproximation);
  double c[4];
  for (int i = 0; i < 4; i++) {
    c[i] = coefficients[i].approximateToScalar<double>(context, complexFormat, angleUnit);
    if (!std::isfinite(c[i])) {
      // Complex or undefined coefficients: keep the interval search
      return Error::RequireApproximateSolution;
    }
  }
  if (c[3] == 0.0) {
    return Error::RequireApproximateSolution;
  }
  Expression a = coefficients[3], b = coefficients[2], cc = coefficients[1], d = coefficients[0];
  // Delta = b^2c^2 + 18abcd - 4ac^3 - 4b^3d - 27a^2d^2
  Expression delta = Addition::Builder({
    Multiplication::Builder(Power::Builder(b.clone(), Rational::Builder(2)), Power::Builder(cc.clone(), Rational::Builder(2))),
    Multiplication::Builder({Rational::Builder(18), a.clone(), b.clone(), cc.clone(), d.clone()}),
    Multiplication::Builder(Rational::Builder(-4), a.clone(), Power::Builder(cc.clone(), Rational::Builder(3))),
    Multiplication::Builder(Rational::Builder(-4), Power::Builder(b.clone(), Rational::Builder(3)), d.clone()),
    Multiplication::Builder(Rational::Builder(-27), Power::Builder(a.clone(), Rational::Builder(2)), Power::Builder(d.clone(), Rational::Builder(2)))
  });
  delta = delta.simplify(reductionContext);
  if (delta.isUninitialized()) {
    delta = Undefined::Builder();
  }

  double roots[3], re = NAN, im = NAN;
  int numberOfRealRoots = cubicNumericRoots(c, roots, &re, &im);

  // Exact branch: x0 = p/q is a root, the rest is (ax^2+(b+a*x0)x+(c+(b+a*x0)*x0))
  for (int i = 0; i < numberOfRealRoots; i++) {
    native_int_t p, q;
    if (!rationalApproximation(roots[i], &p, &q)) {
      continue;
    }
    Expression value = Addition::Builder({
      Multiplication::Builder(a.clone(), Power::Builder(Rational::Builder(p, q), Rational::Builder(3))),
      Multiplication::Builder(b.clone(), Power::Builder(Rational::Builder(p, q), Rational::Builder(2))),
      Multiplication::Builder(cc.clone(), Rational::Builder(p, q)),
      d.clone()
    }).simplify(reductionContext);
    if (value.isUninitialized() || value.nullStatus(context) != ExpressionNode::NullStatus::Null) {
      continue;
    }
    Expression quadratic[Expression::k_maxNumberOfPolynomialCoefficients];
    quadratic[2] = a.clone();
    quadratic[1] = Addition::Builder(b.clone(), Multiplication::Builder(a.clone(), Rational::Builder(p, q))).simplify(reductionContext);
    quadratic[0] = Addition::Builder(cc.clone(), Multiplication::Builder(quadratic[1].clone(), Rational::Builder(p, q))).simplify(reductionContext);
    Expression quadraticSolutions[k_maxNumberOfExactSolutions];
    Expression quadraticApproximations[k_maxNumberOfExactSolutions];
    oneDimensionalPolynomialSolve(quadraticSolutions, quadraticApproximations, quadratic, 2, context);
    int numberOfQuadraticSolutions = m_numberOfSolutions - 1; // without its delta
    Expression rationalRoot = Rational::Builder(p, q);
    const double rationalRootValue = static_cast<double>(p) / static_cast<double>(q);
    bool rationalRootPlaced = false;
    int n = 0;
    for (int j = 0; j < numberOfQuadraticSolutions; j++) {
      /* A double root of the cubic is listed once. Compare exactly: the
       * simplified solutions are beautified (-1 becomes Opposite(1)). */
      Expression difference = Subtraction::Builder(quadraticSolutions[j].clone(), rationalRoot.clone()).simplify(reductionContext);
      if (!difference.isUninitialized() && difference.nullStatus(context) == ExpressionNode::NullStatus::Null) {
        continue;
      }
      // Increasing order; a complex (NAN) quadratic root goes after the real one
      double value = quadraticSolutions[j].approximateToScalar<double>(context, complexFormat, angleUnit);
      if (!rationalRootPlaced && !(value <= rationalRootValue)) {
        exactSolutions[n++] = rationalRoot.clone();
        rationalRootPlaced = true;
      }
      exactSolutions[n++] = quadraticSolutions[j];
    }
    if (!rationalRootPlaced) {
      exactSolutions[n++] = rationalRoot.clone();
    }
    exactSolutions[n++] = delta;
    m_numberOfSolutions = n;
    for (int j = 0; j < n; j++) {
      exactSolutions[j].simplifyAndApproximate(&exactSolutions[j], &exactSolutionsApproximations[j], context, complexFormat, angleUnit, unitFormat);
    }
    return Error::NoError;
  }

  // Numeric branch: real roots, then the complex pair (dropped in Real mode)
  int n = 0;
  for (int i = 0; i < numberOfRealRoots; i++) {
    exactSolutions[n++] = Decimal::Builder<double>(roots[i]);
  }
  if (numberOfRealRoots == 1) {
    exactSolutions[n++] = Subtraction::Builder(Decimal::Builder<double>(re), Multiplication::Builder(Decimal::Builder<double>(im), Poincare::Constant::Builder(UCodePointMathematicalBoldSmallI)));
    exactSolutions[n++] = Addition::Builder(Decimal::Builder<double>(re), Multiplication::Builder(Decimal::Builder<double>(im), Poincare::Constant::Builder(UCodePointMathematicalBoldSmallI)));
  }
  exactSolutions[n++] = delta;
  m_numberOfSolutions = n;
  for (int j = 0; j < n; j++) {
    exactSolutions[j].simplifyAndApproximate(&exactSolutions[j], &exactSolutionsApproximations[j], context, complexFormat, angleUnit, unitFormat);
  }
  return Error::NoError;
}

void EquationStore::tidySolution() {
  for (int i = 0; i < k_maxNumberOfExactSolutions; i++) {
    m_exactSolutionExactLayouts[i] = Layout();
    m_exactSolutionApproximateLayouts[i] = Layout();
  }
}

Preferences::ComplexFormat EquationStore::updatedComplexFormat(Context * context) {
  Preferences::ComplexFormat complexFormat = Preferences::sharedPreferences()->complexFormat();
  if (complexFormat == Preferences::ComplexFormat::Real && isExplicitlyComplex(context)) {
    return Preferences::ComplexFormat::Cartesian;
  }
  return complexFormat;
}

bool EquationStore::isExplicitlyComplex(Context * context) {
  for (int i = 0; i < numberOfDefinedModels(); i++) {
    if (modelForRecord(definedRecordAtIndex(i))->containsIComplex(context)) {
      return true;
    }
  }
  return false;
}

}

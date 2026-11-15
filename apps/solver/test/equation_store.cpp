#include <quiz.h>
#include "helpers.h"

QUIZ_CASE(equation_solve) {
  assert_solves_to_error("x+y+z+a+b+c+d=0", TooManyVariables);
  assert_solves_to_error("x^2+y=0", NonLinearSystem);
  assert_solves_to_error("cos(x)=0", RequireApproximateSolution);

  assert_solves_to_no_solution("2=0");
  assert_solves_to_no_solution("x-x+2=0");

  assert_solves_to_infinite_solutions("0=0");
  assert_solves_to_infinite_solutions("x-x=0");

  assert_solves_to("2x+3=4", "x=1/2");
  assert_solves_to("3×x^2-4x+4=2", {"x=2/3-(√(2)/3)𝐢", "x=2/3+(√(2)/3)𝐢", "delta=-8"});
  assert_solves_to("2×x^2-4×x+4=3", {"x=(-√(2)+2)/2", "x=(√(2)+2)/2", "delta=8"});
  assert_solves_to("2×x^2-4×x+2=0", {"x=1", "delta=0"});
  assert_solves_to(
    "x^2+x+1=3×x^2+π×x-√(5)",
    {
      "x=(√(π^2-2π+8√(5)+9)-π+1)/4",
      "x=(-√(π^2-2π+8×√(5)+9)-π+1)/4",
      "delta=π^2-2π+8√(5)+9"
    }
  );
  assert_solves_to("(x-3)^2=0", {"x=3", "delta=0"});

  // Cubic: a rational root, then the rest solved exactly as a quadratic
  assert_solves_to("x^3-6x^2+11x-6=0", {"x=1", "x=2", "x=3", "delta=4"});
  assert_solves_to("x^3-3x-2=0", {"x=-1", "x=2", "delta=0"});

  // Linear System
  assert_solves_to_infinite_solutions("x+y=0");
  assert_solves_to({"x+y=0", "3x+y=-5"}, {"x=-5/2", "y=5/2"});
  assert_solves_to(
    {
      "x+y=0",
      "3x+y+z=-5",
      "4z-π=0"
    },
    {
      "x=(-π-20)/8",
      "y=(π+20)/8",
      "z=π/4"
    }
  );
  assert_solves_to(
    {
      "x+y=0",
      "3x+y+z=-5",
      "4z-π=0",
      "a+b+c=0",
      "a=3",
      "c=a+2"
    },
    {
      "x=(-π-20)/8",
      "y=(π+20)/8",
      "z=π/4",
      "a=3",
      "b=-8",
      "c=5"
    }
  );

  /* This test case needs the user defined variable. Indeed, in the equation
   * store, m_variables is just before m_userVariables, so bad fetching in
   * m_variables might fetch into m_userVariables and create problems. */
  set("x", "0");
  assert_solves_to_infinite_solutions({
    "b=0",
    "D=0",
    "e=0",
    "x+y+z+t=0"
  });
  unset("x");

  // Monovariable non-polynomial equation
  Poincare::Preferences::sharedPreferences()->setAngleUnit(Degree);
  assert_solves_numerically_to("cos(x)=0", -100, 100, {-90.0, 90.0});
  assert_solves_numerically_to("cos(x)=0", -900, 1000, {-810.0, -630.0, -450.0, -270.0, -90.0, 90.0, 270.0, 450.0, 630.0, 810.0});
  assert_solves_numerically_to("√(y)=0", -900, 1000, {0}, "y");
  assert_solves_numerically_to("ℯ^x=0", -1000, 1000, {});
  assert_solves_numerically_to("ℯ^x/1000=0", -1000, 1000, {});

  // Long variable names
  assert_solves_to("2abcde+3=4", "abcde=1/2");
  assert_solves_to({"Big1+Big2=0", "3Big1+Big2=-5"}, {"Big1=-5/2", "Big2=5/2"});

  // conj(x)*x+1 = 0
  assert_solves_to_error("conj(x)*x+1=0", RequireApproximateSolution);
  assert_solves_numerically_to("conj(x)*x+1=0", -100, 100, {});

  assert_solves_to_error("(x-10)^7=0", RequireApproximateSolution);
  assert_solves_numerically_to("(x-10)^7=0", -100, 100, {10});
}


QUIZ_CASE(equation_solve_complex_real) {
  set_complex_format(Real);
  assert_solves_to("x+𝐢=0", "x=-𝐢"); // We still want complex solutions if the input has some complex value
  assert_solves_to_error("x+√(-1)=0", EquationUnreal);
  assert_solves_to("x^2+x+1=0", {"delta=-3"});
  assert_solves_to_error("x^2-√(-1)=0", EquationUnreal);
  assert_solves_to_error("x+√(-1)×√(-1)=0", EquationUnreal);
  assert_solves_to("root(-8,3)*x+3=0", "x=3/2");
  reset_complex_format();
}

QUIZ_CASE(equation_solve_complex_cartesian) {
  set_complex_format(Cartesian);
  assert_solves_to("x+𝐢=0", "x=-𝐢");
  assert_solves_to("x+√(-1)=0", "x=-𝐢");
  assert_solves_to({"x^2+x+1=0"}, {"x=-1/2-((√(3))/2)𝐢", "x=-1/2+((√(3))/2)𝐢", "delta=-3"});
  assert_solves_to("x^2-√(-1)=0", {"x=-√(2)/2-(√(2)/2)𝐢", "x=√(2)/2+(√(2)/2)𝐢", "delta=4𝐢"});
  assert_solves_to("x+√(-1)×√(-1)=0", "x=1");
  assert_solves_to("root(-8,3)*x+3=0", "x=-3/4+(3√(3)/4)*𝐢");
  reset_complex_format();
}

QUIZ_CASE(equation_solve_complex_polar) {
  set_complex_format(Polar);
  assert_solves_to("x+𝐢=0", "x=ℯ^(-(π/2)𝐢)");
  assert_solves_to("x+√(-1)=0", "x=ℯ^(-(π/2)𝐢)");
  assert_solves_to("x^2+x+1=0", {"x=ℯ^(-(2π/3)𝐢)", "x=ℯ^((2π/3)𝐢)", "delta=3ℯ^(π𝐢)"});
  assert_solves_to("x^2-√(-1)=0", {"x=ℯ^(-(3π/4)𝐢)", "x=ℯ^((π/4)𝐢)", "delta=4ℯ^((π/2)𝐢)"});
  assert_solves_to("root(-8,3)*x+3=0", "x=3/2×ℯ^((2π/3)𝐢)");
  reset_complex_format();
}

QUIZ_CASE(equation_and_symbolic_computation) {
  assert_solves_to_infinite_solutions("x+a=0");

  set("a", "-3");
  assert_solves_to("x+a=0", "x=3");

  assert_solves_to("a=0", "a=0");
  /* The equation has no solution since the user defined a = -3. So a is not
   * replaced with its context value, and the solution is a = 0. */

  set("b", "-4");
  assert_solves_to_infinite_solutions("a+b=0");
  /* The equation has no solution since the user defined a = -3 and b = -4.
   * So neither a nor b are replaced with their context values. Therefore the
   * solution is an infinity of solutions. */

  assert_solves_to("a+b+c=0", "c=7");

  assert_solves_to({"a+c=0", "a=3"}, {"a=3", "c=-3"});
  /* The system has no solution since the user defined a = -3. So a is not
   * replaced with its context value, and the solution is a = 3 and c = -3. */

  set("f(x)", "x+1");

  assert_solves_to("f(x)=0", "x=-1");

  assert_solves_to("f(a)=0", "a=-1");
  /* The equation has no solution since the user defined a = -3. So a is not
   * replaced with its context value, and the solution is a = -1. */

  set("g(x)", "a+x+2");

  assert_solves_to("g(x)=0", "x=1");

  assert_solves_to("g(a)=0", "a=-1");
  /* The equation has no solution since the user defined a = -3. So a is not
   * replaced with its context value, and the equation becomes a + a + 2 = 0.
   * The solution is therefore a = -1. */

  set("d", "5");
  set("c", "d");
  set("h(x)", "c+d+3");
  assert_solves_to({"h(x)=0", "c=-3"}, {"c=-3", "d=0"});
  // c and d context values should not be used

  assert_solves_to({"c+d=5", "c-d=1"}, {"c=3", "d=2"});

  set("e", "8_g");
  assert_solves_to({"e+1=0"}, {"e=-1"});

  unset("a");
  unset("b");
  unset("c");
  unset("d");
  unset("e");
  unset("f");
  unset("g");
  unset("h");
}

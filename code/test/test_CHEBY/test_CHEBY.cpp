#include <array>
#include <cmath>

#include <unity.h>

#include "ChebyshevFit.h"

// Fit values from Python coeffs.py that is someone else's non-broken implementation

// f(x) = x^2.8 on [0,1]
void test_coeffs_exp2_8(void)
{
    float expected_coeffs[] = { 0.32252400334891923, 0.47528279912239385, 0.17826349098369204, 0.024472637538378833 };
    std::array<float, 4> coeffs = ChebyshevFit::fit<float>([](float x) -> float { return std::pow(x, 2.8); }, 0, 1);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected_coeffs, coeffs.data(), 4);
}

// f(x) = 1/x on [0,100]
void test_coeffs_1_x(void)
{
    float expected_coeffs[] = { 0.08000000000000003, -0.12000000000000004, 0.08000000000000002, -0.04 };
    std::array<float, 4> coeffs = ChebyshevFit::fit<float>([](float x) -> float { return 1 / x; }, 0, 100);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected_coeffs, coeffs.data(), 4);
}

// f(x) = sin(x) on [-7,7]
void test_coeffs_exp(void)
{
    float expected_coeffs[] = { 1.2660656785395275, 1.1303149985117358, 0.27145036166053393, 0.04379392351181022 };
    std::array<float, 4> coeffs = ChebyshevFit::fit<float>([](float x) -> float { return std::exp(x); }, -1, 1);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected_coeffs, coeffs.data(), 4);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_coeffs_exp2_8);
    RUN_TEST(test_coeffs_1_x);
    RUN_TEST(test_coeffs_exp);
    UNITY_END();
}

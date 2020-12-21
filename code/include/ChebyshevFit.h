#pragma once
#include "gcem.hpp"

#include <array>

// C++17 still doesn't define this...
constexpr double pi = 3.14159265358979323846;

// Way over the top implementation of a minimax coefficient estimator to determine Chebyshev coefficients for an
// efficient gamma correction implementation (basically a fast pow(x,y) over [0,1] for fixed y)
struct ChebyshevFit {

    // Get the Chebyshev node i with n total nodes
    static constexpr double cheby(double i, double n)
    {
        auto val = ((2.0 * i - 1) / (2.0 * n)) * pi;
        return gcem::cos(val) * -1;
    }

    // Scale x values over [a,b] to u values over [-1,1]
    template <class T> constexpr static T u_to_x(T u, T a, T b) { return u * (b - a) / 2 + (a + b) / 2; }
    // Scale u values over [-1,1] to x values over [a,b]
    template <class T> constexpr static T x_to_u(T x, T a, T b) { return (2 * x - a - b) / (b - a); }

    template <class T, class Func> static constexpr std::array<T, 4> fit(Func f, T a, T b)
    {
        // NOTE: not generalized for other degrees. Wouldn't be hard, just lazy...
        constexpr auto degree = 3;
        std::array<T, degree + 1> coeffs { 0, 0, 0, 0 };

        // Get the first degree+1 Chebyshev nodes' u values
        std::array<T, coeffs.size()> N {};
        for (auto i = 0U; i < N.size(); i++)
            N[i] = cheby(i + 1, N.size());

        // Get the x-scaled y values for those Chebyshev nodes
        std::array<T, N.size()> y { 0, 0, 0, 0 };
        for (auto i = 0U; i < N.size(); i++)
            y[i] = f(u_to_x(N[i], a, b));

        // coeffs[0] is the average of y
        for (auto val : y) {
            coeffs[0] += val;
        }
        coeffs[0] /= N.size();

        // coeffs[1] is 2 * avg(u*y)
        for (auto i = 0U; i < N.size(); i++) {
            coeffs[1] += N[i] * y[i];
        }
        coeffs[1] *= 2.0 / N.size();

        // coeffs[2] is 2 * avg((2*u*u -1) * y)
        for (auto i = 0U; i < N.size(); i++) {
            coeffs[2] += (2 * N[i] * N[i] - 1) * y[i];
        }
        coeffs[2] *= 2.0 / N.size();

        // coeffs[3] is 2 * avg((4*u*u*u - 3*u) * y)
        for (auto i = 0U; i < N.size(); i++) {
            coeffs[3] += (4 * N[i] * N[i] * N[i] - 3 * N[i]) * y[i];
        }
        coeffs[3] *= 2.0 / N.size();

        return coeffs;
    }
};

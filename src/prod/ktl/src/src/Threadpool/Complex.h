#pragma once

#include <math.h>

namespace KtlThreadpool {

    class Complex
    {
    public:
        double r;
        double i;

        Complex() : r(0), i(0) {}
        Complex(double real) : r(real), i(0) {}
        Complex(double real, double imag) : r(real), i(imag) {}
        Complex(const Complex& other) : r(other.r), i(other.i) {}
    };

    inline Complex operator+(Complex left, Complex right)
    {
        return Complex(left.r + right.r, left.i + right.i);
    }

    inline Complex operator-(Complex left, Complex right)
    {
        return Complex(left.r - right.r, left.i - right.i);
    }

    inline Complex operator*(Complex left, Complex right)
    {
        return Complex(
            left.r * right.r - left.i * right.i,
            left.r * right.i + left.i * right.r);
    }

    inline Complex operator/(Complex left, Complex right)
    {
        double denom = right.r * right.r + right.i * right.i;
        return Complex(
            (left.r * right.r + left.i * right.i) / denom,
            (-left.r * right.i + left.i * right.r) / denom);
    }

    inline double abs(Complex c)
    {
        return sqrt(c.r * c.r + c.i * c.i);
    }
}

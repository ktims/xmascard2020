#!/usr/bin/env python3

import math
import numpy as np
import matplotlib.pyplot as plt

# experiment with Chevyshev approximation of pow(x,<const y>) over x[0,1]
# Ref: https://www.embeddedrelated.com/showarticle/152.php

# function to optimize


def f1(x: np.float):
    return np.power(x,2.8)
def f2(x: np.float):
    return 1/x
def f3(x: np.float):
    return np.exp(x)


def fa(x: np.longdouble):
    u = 2 * x - 1
    #u = x
    # coeffs = [ 0.32252400334891923, 0.47528279912239385, 0.17826349098369204, 0.024472637538378833]
    coeffs = [0.32252400334891923, 0.47528279912239385, 0.17826349098369204, 0.024472637538378833]
    return coeffs[0] + coeffs[1] * u + coeffs[2] * (2 * u * u - 1) + coeffs[3] * (4 * u * u * u - 3 * u)



def chebspace(npts):
    t = (np.array(range(0, npts)) + 0.5) / npts
    return -np.cos(t*math.pi)


def chebmat(u, N):
    T = np.column_stack((np.ones(len(u)), u))
    for n in range(2, N+1):
        Tnext = 2*u*T[:, n-1] - T[:, n-2]
        T = np.column_stack((T, Tnext))
    return T


class Cheby(object):
    def __init__(self, a, b, *coeffs):
        self.c = (a+b)/2.0
        self.m = (b-a)/2.0
        self.coeffs = np.array(coeffs, ndmin=1)

    def rangestart(self):
        return self.c-self.m

    def rangeend(self):
        return self.c+self.m

    def range(self):
        return (self.rangestart(), self.rangeend())

    def degree(self):
        return len(self.coeffs)-1

    def truncate(self, n):
        return Cheby(self.rangestart(), self.rangeend(), *self.coeffs[0:n+1])

    def asTaylor(self, x0=0, m0=1.0):
        n = self.degree()+1
        Tprev = np.zeros(n)
        T = np.zeros(n)
        Tprev[0] = 1
        T[1] = 1
        # evaluate y = Chebyshev functions as polynomials in u
        y = self.coeffs[0] * Tprev
        for co in self.coeffs[1:]:
            y = y + T*co
            xT = np.roll(T, 1)
            xT[0] = 0
            Tnext = 2*xT - Tprev
            Tprev = T
            T = Tnext
        # now evaluate y2 = polynomials in x
        P = np.zeros(n)
        y2 = np.zeros(n)
        P[0] = 1
        k0 = -self.c/self.m
        k1 = 1.0/self.m
        k0 = k0 + k1*x0
        k1 = k1/m0
        for yi in y:
            y2 = y2 + P*yi
            Pnext = np.roll(P, 1)*k1
            Pnext[0] = 0
            P = Pnext + k0*P
        return y2

    def __call__(self, x):
        xa = np.array(x, copy=False, ndmin=1)
        u = np.array((xa-self.c)/self.m)
        Tprev = np.ones(len(u))
        y = self.coeffs[0] * Tprev
        if self.degree() > 0:
            y = y + u*self.coeffs[1]
            T = u
        for n in range(2, self.degree()+1):
            Tnext = 2*u*T - Tprev
            Tprev = T
            T = Tnext
            y = y + T*self.coeffs[n]
        return y

    def __repr__(self):
        return "Cheby%s" % (self.range()+tuple(c for c in self.coeffs)).__repr__()

    @staticmethod
    def fit(func, a, b, degree):
        N = degree+1
        u = chebspace(N)
        x = (u*(b-a) + (b+a))/2.0
        y = func(x)
        T = chebmat(u, N=degree)
        c = 2.0/N * np.dot(y, T)
        c[0] = c[0]/2
        return Cheby(a, b, *c)



def showplots(f, approxlist, a, b):
    x = np.linspace(a, b, 1000)
    plt.figure(1)
    plt.subplot(211)
    plt.plot(x, f(x))
    if approxlist:
        vfuncs = [np.vectorize(approx) for approx in approxlist]
        for vf in vfuncs:
            plt.plot(x, vf(x))
        plt.xlim(a, b)
        plt.ylabel('f(x) and approximations fa(x)')
        plt.subplot(212)
        for vf in vfuncs:
            plt.plot(x, (f(x)-vf(x))*65535)
        plt.xlim(a, b)
        plt.ylabel('error = f(x)-fa(x)*65535')
        plt.xlabel('x')
    plt.show()

c = Cheby.fit(f1, 0, 1, 3)
print("f(x) = x^2.8 on [0,1] { " + ",".join(str(i) for i in list(c.coeffs)) + " }")
c = Cheby.fit(f2, 0, 100, 3)
print("f(x) = 1/x on [0,100] { " + ",".join(str(i) for i in list(c.coeffs)) + " }")
c = Cheby.fit(f3, -1, 1, 3)
print("f(x) = e^x on [-1,1] { " + ",".join(str(i) for i in list(c.coeffs)) + " }")
#showplots(f, [fa,c], 0, 1)

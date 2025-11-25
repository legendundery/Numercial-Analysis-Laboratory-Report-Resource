#include "calc.h"

#include <cmath>
#include <limits>

namespace calc
{
    std::vector<Iteration> bisection(const std::function<double(double)> &f,
                                     double a,
                                     double b,
                                     int maxIter,
                                     double tol)
    {
        std::vector<Iteration> iters;
        if (a > b)
            std::swap(a, b);
        double fa = f(a), fb = f(b);
        if (!(fa * fb < 0.0) || !std::isfinite(fa) || !std::isfinite(fb))
        {
            return iters; // 前置条件不满足
        }

        double prevX = std::numeric_limits<double>::quiet_NaN();
        for (int k = 1; k <= std::max(1, maxIter); ++k)
        {
            double m = 0.5 * (a + b);
            double fm = f(m);
            double err = std::isnan(prevX) ? std::numeric_limits<double>::infinity() : std::fabs(m - prevX);
            iters.push_back({k, m, fm, err});

            if (std::fabs(fm) < tol || err < tol)
                break;

            // 缩区间
            if (fa * fm < 0.0)
            {
                b = m;
                fb = fm;
            }
            else
            {
                a = m;
                fa = fm;
            }
            prevX = m;
        }
        return iters;
    }

    std::vector<Iteration> newton(const std::function<double(double)> &f,
                                  const std::function<double(double)> &df,
                                  double x0,
                                  int maxIter,
                                  double tol)
    {
        std::vector<Iteration> iters;
        double x = x0;
        double prevX = std::numeric_limits<double>::quiet_NaN();
        for (int k = 1; k <= std::max(1, maxIter); ++k)
        {
            double fx = f(x);
            double dfx = df(x);
            if (!std::isfinite(fx) || !std::isfinite(dfx))
                break;
            if (std::fabs(dfx) < 1e-14)
                break; // 导数过小，避免爆炸
            double x1 = x - fx / dfx;
            double err = std::isnan(prevX) ? std::numeric_limits<double>::infinity() : std::fabs(x1 - prevX);
            iters.push_back({k, x1, f(x1), err});
            if (std::fabs(x1 - x) < tol)
                break;
            prevX = x1;
            x = x1;
        }
        return iters;
    }
}

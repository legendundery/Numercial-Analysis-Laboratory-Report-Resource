#ifndef CALC_H
#define CALC_H

#include <functional>
#include <string>
#include <vector>

// 计算模块
namespace calc
{
    struct Iteration
    {
        int k = 0;
        double x = 0.0;
        double fx = 0.0;
        double error = 0.0; // 相邻迭代差 |x_k - x_{k-1}|
    };

    // 对分法（要求 f(a)*f(b) < 0）
    // 返回迭代表（含每步 x, f(x), 误差），若前置不满足返回空表
    std::vector<Iteration> bisection(const std::function<double(double)> &f,
                                     double a,
                                     double b,
                                     int maxIter,
                                     double tol);

    // 牛顿法
    std::vector<Iteration> newton(const std::function<double(double)> &f,
                                  const std::function<double(double)> &df,
                                  double x0,
                                  int maxIter,
                                  double tol);
}

#endif // CALC_H
#ifndef CALC_H
#define CALC_H

#include <functional>
#include <string>
#include <vector>

// 纯计算模块
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

    // ==================== 矩阵计算 ====================

    // 矩阵类（简单实现）
    class Matrix
    {
    public:
        Matrix() : rows_(0), cols_(0) {}
        Matrix(int rows, int cols, double initVal = 0.0);
        Matrix(const std::vector<std::vector<double>> &data);

        int rows() const { return rows_; }
        int cols() const { return cols_; }

        double &operator()(int i, int j) { return data_[i][j]; }
        double operator()(int i, int j) const { return data_[i][j]; }

        const std::vector<std::vector<double>> &data() const { return data_; }
        std::vector<std::vector<double>> &data() { return data_; }

        // 获取某一行
        std::vector<double> row(int i) const;
        // 获取某一列
        std::vector<double> col(int j) const;

        // 转换为字符串（用于显示）
        std::string toString(int precision = 6) const;

    private:
        int rows_, cols_;
        std::vector<std::vector<double>> data_;
    };

    // 高斯消元法结果
    struct GaussResult
    {
        bool success = false;              // 是否求解成功
        std::vector<double> solution;      // 解向量
        std::string errorMsg;              // 错误信息
        std::vector<Matrix> steps;         // 中间步骤（用于显示过程）
        std::vector<std::string> stepDesc; // 每步的描述
        Matrix L;                          // 下三角矩阵（LU分解）
        Matrix U;                          // 上三角矩阵（LU分解）
    };

    // 高斯消元法（列主元）
    // A: 系数矩阵 (n x n)
    // b: 右端向量 (n)
    // 返回：求解结果，包含解向量和中间步骤
    GaussResult gaussElimination(const Matrix &A, const std::vector<double> &b);

    // 克劳特消元法（Crout 分解，约定 u_ii = 1）
    // A: 系数矩阵 (n x n)
    // b: 右端向量 (n)
    // 返回：求解结果，包含解向量和 L/U 矩阵
    GaussResult croutElimination(const Matrix &A, const std::vector<double> &b);

    // 平方根法（Cholesky，A 为实对称正定）：A = L·L^T
    GaussResult choleskySolve(const Matrix &A, const std::vector<double> &b);

    // 追赶法（Thomas 算法，A 为三对角矩阵）
    GaussResult thomasTridiagonal(const Matrix &A, const std::vector<double> &b);

    // 列主元素法：每次选取当前列绝对值最大的元素交换到主元位置
    GaussResult columnPivoting(const Matrix &A, const std::vector<double> &b);

    // 全主元素法：每次选取全部剩余元素中绝对值最大的交换到主元位置
    GaussResult fullPivoting(const Matrix &A, const std::vector<double> &b);
}

#endif // CALC_H
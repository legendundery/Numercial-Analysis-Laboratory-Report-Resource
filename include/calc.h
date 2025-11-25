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
    // 其余方法复用即可

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

    // ==================== 矩阵工具函数 ====================

    // 矩阵范数
    double matrixNorm1(const Matrix &A);   // 1-范数（列和范数）
    double matrixNorm2(const Matrix &A);   // 2-范数（谱范数）
    double matrixNormInf(const Matrix &A); // ∞-范数（行和范数）
    double matrixNormF(const Matrix &A);   // F-范数（Frobenius范数）

    // 向量范数
    double vectorNorm2(const std::vector<double> &v);   // 2-范数
    double vectorNormInf(const std::vector<double> &v); // ∞-范数

    // 矩阵运算
    Matrix matrixMultiply(const Matrix &A, const Matrix &B);                                 // 矩阵乘法
    std::vector<double> matrixVectorMultiply(const Matrix &A, const std::vector<double> &x); // 矩阵向量乘法
    Matrix matrixAdd(const Matrix &A, const Matrix &B);                                      // 矩阵加法
    Matrix matrixSubtract(const Matrix &A, const Matrix &B);                                 // 矩阵减法
    Matrix matrixScale(const Matrix &A, double scalar);                                      // 矩阵数乘

    // 谱半径（最大特征值的绝对值）
    double spectralRadius(const Matrix &A, int maxIter = 100, double tol = 1e-6);

    // 检查矩阵是否严格对角占优
    bool isStrictlyDiagonallyDominant(const Matrix &A);

    // 计算雅可比迭代矩阵的谱半径（用于判断收敛性）
    double jacobiSpectralRadius(const Matrix &A);

    // 计算SOR方法的最优松弛因子（针对对角占优矩阵）
    double optimalOmegaSOR(const Matrix &A);

    // ==================== 迭代法求解线性方程组 ====================

    // 迭代法结果
    struct IterativeResult
    {
        bool success = false;                        // 是否收敛
        std::vector<double> solution;                // 解向量
        std::string errorMsg;                        // 错误信息
        std::vector<std::vector<double>> iterations; // 迭代过程（每行是一次迭代的解向量）
        std::vector<double> errors;                  // 每次迭代的误差
        Matrix iterationMatrix;                      // 迭代矩阵
        double spectralRadius;                       // 迭代矩阵的谱半径
        std::vector<std::string> stepDesc;           // 步骤描述
    };

    // 雅可比迭代法
    IterativeResult jacobiIteration(const Matrix &A, const std::vector<double> &b,
                                    const std::vector<double> &x0, int maxIter, double tol);

    // 高斯-赛德尔迭代法
    IterativeResult gaussSeidelIteration(const Matrix &A, const std::vector<double> &b,
                                         const std::vector<double> &x0, int maxIter, double tol);

    // 松弛迭代法（SOR）
    IterativeResult sorIteration(const Matrix &A, const std::vector<double> &b,
                                 const std::vector<double> &x0, int maxIter, double tol, double omega);
}

#endif // CALC_H
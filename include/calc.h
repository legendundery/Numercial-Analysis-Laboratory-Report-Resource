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

                                 
    // ==================== 插值法基础工具 ====================

    // 差分表（Forward Difference Table）
    // 输入：函数值序列 y = [y0, y1, ..., yn]
    // 输出：差分表，table[i][j] 表示 Δ^j y_i
    //       table[0] = [y0, Δy0, Δ²y0, ...]
    //       table[1] = [y1, Δy1, Δ²y1, ...]
    std::vector<std::vector<double>> forwardDifferenceTable(const std::vector<double> &y);

    // 后向差分表（Backward Difference Table）
    // 输入：函数值序列 y = [y0, y1, ..., yn]
    // 输出：后向差分表，table[i][j] 表示 ∇^j y_i
    std::vector<std::vector<double>> backwardDifferenceTable(const std::vector<double> &y);

    // 差商表（Divided Difference Table）
    // 输入：节点 x = [x0, x1, ..., xn] 和函数值 y = [y0, y1, ..., yn]
    // 输出：差商表，table[i][j] 表示 f[x_i, x_{i+1}, ..., x_{i+j}]
    //       table[0] = [f[x0], f[x0,x1], f[x0,x1,x2], ...]
    //       table[1] = [f[x1], f[x1,x2], ...]
    std::vector<std::vector<double>> dividedDifferenceTable(const std::vector<double> &x,
                                                            const std::vector<double> &y);

    // 广义组合数（Generalized Binomial Coefficient）
    // C(t, n) = t(t-1)(t-2)...(t-n+1) / n!
    // 用于牛顿插值公式中的系数计算
    double generalizedBinomial(double t, int n);

    // 阶乘
    long long factorial(int n);

    // 插值结果结构
    struct InterpolationResult
    {
        bool success = false;
        double value;                           // 插值结果
        std::string errorMsg;                   // 错误信息
        std::vector<std::vector<double>> table; // 差分表或差商表
        std::vector<double> coefficients;       // 插值多项式系数
        std::string polynomial;                 // 插值多项式表达式（字符串形式）
        std::vector<std::string> stepDesc;      // 计算步骤描述
        std::string method;                     // 使用的插值方法（前插/后插/斯梯林/贝塞尔）
        double t;                               // 归一化参数 t = (x - x0) / h
        int baseIndex;                          // 基准节点索引 x0 在数据中的位置
    };

    // 牛顿差商插值（不等距节点）
    // 输入：节点 x, 函数值 y, 插值点 xVal
    // 输出：插值结果，包含 P_n(xVal) 及计算过程
    InterpolationResult newtonDividedDifference(const std::vector<double> &x,
                                                const std::vector<double> &y,
                                                double xVal);

    // 牛顿前插公式（等距节点）
    // 输入：节点 x (等距), 函数值 y, 插值点 xVal
    // 输出：插值结果，包含 P_n(xVal) 及计算过程
    // 适用于 x 在前部区间，即 t = (xVal - x[0]) / h 在 [0, 1] 附近
    InterpolationResult newtonForwardDifference(const std::vector<double> &x,
                                                const std::vector<double> &y,
                                                double xVal);

    // 牛顿后插公式（等距节点）
    // 输入：节点 x (等距), 函数值 y, 插值点 xVal
    // 输出：插值结果，包含 P_n(xVal) 及计算过程
    // 适用于 x 在后部区间，即 t = (xVal - x[n]) / h 在 [-1, 0] 附近
    InterpolationResult newtonBackwardDifference(const std::vector<double> &x,
                                                 const std::vector<double> &y,
                                                 double xVal);

    // 斯梯林插值公式（等距节点）
    // 输入：节点 x (等距), 函数值 y, 插值点 xVal
    // 输出：插值结果，包含 P_n(xVal) 及计算过程
    // 适用于 x 在中部区间，使用中心差分
    InterpolationResult stirlingInterpolation(const std::vector<double> &x,
                                              const std::vector<double> &y,
                                              double xVal);

    // 贝塞尔插值公式（等距节点）
    // 输入：节点 x (等距), 函数值 y, 插值点 xVal
    // 输出：插值结果，包含 P_n(xVal) 及计算过程
    // 适用于 x 在中部区间（与斯梯林类似）
    InterpolationResult besselInterpolation(const std::vector<double> &x,
                                            const std::vector<double> &y,
                                            double xVal);

    // 拉格朗日插值公式
    // 输入：节点 x, 函数值 y, 插值点 xVal
    // 输出：插值结果，包含 L_n(xVal) 及计算过程
    // L_n(x) = Σ[i=0 to n] l_i(x)·f(x_i)
    // 其中 l_i(x) = Π[j≠i] (x-x_j)/(x_i-x_j)
    // 适用于任意节点分布，计算简单但数值稳定性较差
    InterpolationResult lagrangeInterpolation(const std::vector<double> &x,
                                              const std::vector<double> &y,
                                              double xVal);

    // 等距节点检查与插值方法选择
    // 输入：节点 x, 插值点 xVal
    // 输出：{是否等距, 间距h, 推荐方法名称, t值, 基准索引}
    struct InterpolationMethodInfo
    {
        bool isEquidistant = false;
        double h = 0.0;
        std::string recommendedMethod; // "forward", "backward", "stirling", "bessel"
        double t = 0.0;
        int baseIndex = 0;
        std::string reason; // 推荐原因
    };
    InterpolationMethodInfo selectInterpolationMethod(const std::vector<double> &x, double xVal);
}

#endif // CALC_H

#include "calc.h"

#include <cmath>
#include <limits>
#include <sstream>
#include <iomanip>
#include <algorithm>

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

    // ==================== 矩阵实现 ====================

    Matrix::Matrix(int rows, int cols, double initVal)
        : rows_(rows), cols_(cols), data_(rows, std::vector<double>(cols, initVal))
    {
    }

    Matrix::Matrix(const std::vector<std::vector<double>> &data)
        : rows_(data.size()), cols_(data.empty() ? 0 : data[0].size()), data_(data)
    {
    }

    std::vector<double> Matrix::row(int i) const
    {
        if (i < 0 || i >= rows_)
            return {};
        return data_[i];
    }

    std::vector<double> Matrix::col(int j) const
    {
        if (j < 0 || j >= cols_)
            return {};
        std::vector<double> result(rows_);
        for (int i = 0; i < rows_; ++i)
            result[i] = data_[i][j];
        return result;
    }

    std::string Matrix::toString(int precision) const
    {
        std::ostringstream oss;
        for (int i = 0; i < rows_; ++i)
        {
            for (int j = 0; j < cols_; ++j)
            {
                oss << std::fixed << std::setprecision(precision) << std::setw(precision + 4) << data_[i][j];
                if (j < cols_ - 1)
                    oss << " ";
            }
            if (i < rows_ - 1)
                oss << "\n";
        }
        return oss.str();
    }

    // 高斯消元法（列主元）
    GaussResult gaussElimination(const Matrix &A, const std::vector<double> &b)
    {
        GaussResult result;
        int n = A.rows();

        if (A.cols() != n || (int)b.size() != n)
        {
            result.success = false;
            result.errorMsg = "矩阵维度不匹配";
            return result;
        }

        // 初始化L和U矩阵
        result.L = Matrix(n, n, 0.0);
        result.U = Matrix(n, n, 0.0);

        // L矩阵对角线初始化为1
        for (int i = 0; i < n; ++i)
            result.L(i, i) = 1.0;

        // 构建增广矩阵 [A | b]
        Matrix augmented(n, n + 1);
        for (int i = 0; i < n; ++i)
        {
            for (int j = 0; j < n; ++j)
                augmented(i, j) = A(i, j);
            augmented(i, n) = b[i];
        }

        result.steps.push_back(augmented);
        result.stepDesc.push_back("初始增广矩阵 [A|b]");

        // 前向消元（列主元）
        for (int k = 0; k < n - 1; ++k)
        {
            // 选主元
            int maxRow = k;
            double maxVal = std::abs(augmented(k, k));
            for (int i = k + 1; i < n; ++i)
            {
                if (std::abs(augmented(i, k)) > maxVal)
                {
                    maxVal = std::abs(augmented(i, k));
                    maxRow = i;
                }
            }

            // 交换行
            if (maxRow != k)
            {
                for (int j = k; j <= n; ++j)
                    std::swap(augmented(k, j), augmented(maxRow, j));

                result.steps.push_back(augmented);
                std::ostringstream oss;
                oss << "交换第 " << (k + 1) << " 行和第 " << (maxRow + 1) << " 行";
                result.stepDesc.push_back(oss.str());
            }

            // 检查主元是否为零
            if (std::abs(augmented(k, k)) < 1e-14)
            {
                result.success = false;
                result.errorMsg = "矩阵奇异或接近奇异";
                return result;
            }

            // 消元
            for (int i = k + 1; i < n; ++i)
            {
                double factor = augmented(i, k) / augmented(k, k);
                result.L(i, k) = factor; // 记录L矩阵的系数
                for (int j = k; j <= n; ++j)
                {
                    augmented(i, j) -= factor * augmented(k, j);
                }
            }

            result.steps.push_back(augmented);
            std::ostringstream oss;
            oss << "消元第 " << (k + 1) << " 列";
            result.stepDesc.push_back(oss.str());
        }

        // 提取U矩阵（上三角矩阵）
        for (int i = 0; i < n; ++i)
        {
            for (int j = i; j < n; ++j)
            {
                result.U(i, j) = augmented(i, j);
            }
        }

        // 回代求解
        result.solution.resize(n);
        for (int i = n - 1; i >= 0; --i)
        {
            double sum = augmented(i, n);
            for (int j = i + 1; j < n; ++j)
            {
                sum -= augmented(i, j) * result.solution[j];
            }
            if (std::abs(augmented(i, i)) < 1e-14)
            {
                result.success = false;
                result.errorMsg = "矩阵奇异";
                return result;
            }
            result.solution[i] = sum / augmented(i, i);
        }

        result.success = true;
        return result;
    }
    GaussResult croutElimination(const Matrix &A, const std::vector<double> &b)
    {
        GaussResult result;
        int n = A.rows();

        if (A.cols() != n || (int)b.size() != n)
        {
            result.success = false;
            result.errorMsg = "矩阵维度不匹配";
            return result;
        }

        // 初始化L和U矩阵
        result.L = Matrix(n, n, 0.0);
        result.U = Matrix(n, n, 0.0);

        // U矩阵对角线初始化为1（Crout约定）
        for (int i = 0; i < n; ++i)
            result.U(i, i) = 1.0;

        result.stepDesc.push_back("开始克劳特分解（u_ii = 1）");

        // Crout分解：逐列求L，逐行求U
        for (int j = 0; j < n; ++j)
        {
            // 计算L第j列
            for (int i = j; i < n; ++i)
            {
                double sum = 0.0;
                for (int k = 0; k < j; ++k)
                    sum += result.L(i, k) * result.U(k, j);
                result.L(i, j) = A(i, j) - sum;
            }
            {
                std::ostringstream oss;
                oss << "计算 L 的第 " << (j + 1) << " 列";
                result.stepDesc.push_back(oss.str());
            }

            if (std::abs(result.L(j, j)) < 1e-14)
            {
                result.success = false;
                result.errorMsg = "矩阵奇异或接近奇异";
                return result;
            }

            // 计算U第j行（对角为1，从 j+1 到 n-1）
            for (int i = j + 1; i < n; ++i)
            {
                double sum = 0.0;
                for (int k = 0; k < j; ++k)
                    sum += result.L(j, k) * result.U(k, i);
                result.U(j, i) = (A(j, i) - sum) / result.L(j, j);
            }
            if (j < n - 1)
            {
                std::ostringstream oss;
                oss << "计算 U 的第 " << (j + 1) << " 行";
                result.stepDesc.push_back(oss.str());
            }
        }

        result.stepDesc.push_back("LU分解完成");

        // 前向替代 Ly = b
        std::vector<double> y(n);
        for (int i = 0; i < n; ++i)
        {
            double sum = 0.0;
            for (int j = 0; j < i; ++j)
                sum += result.L(i, j) * y[j];
            if (std::abs(result.L(i, i)) < 1e-14)
            {
                result.success = false;
                result.errorMsg = "矩阵奇异或接近奇异";
                return result;
            }
            y[i] = (b[i] - sum) / result.L(i, i);
        }
        result.stepDesc.push_back("前向替代求解 Ly = b");

        // 后向替代 Ux = y（u_ii=1）
        result.solution.resize(n);
        for (int i = n - 1; i >= 0; --i)
        {
            double sum = 0.0;
            for (int j = i + 1; j < n; ++j)
                sum += result.U(i, j) * result.solution[j];
            result.solution[i] = y[i] - sum;
        }
        result.stepDesc.push_back("后向替代求解 Ux = y");

        result.success = true;
        return result;
    }

    // 平方根法（Cholesky）
    GaussResult choleskySolve(const Matrix &A, const std::vector<double> &b)
    {
        GaussResult result;
        int n = A.rows();
        if (A.cols() != n || (int)b.size() != n)
        {
            result.success = false;
            result.errorMsg = "矩阵维度不匹配";
            return result;
        }
        for (int i = 0; i < n; ++i)
            for (int j = i + 1; j < n; ++j)
                if (std::fabs(A(i, j) - A(j, i)) > 1e-10)
                {
                    result.success = false;
                    result.errorMsg = "A 非对称";
                    return result;
                }

        result.L = Matrix(n, n, 0.0);
        for (int i = 0; i < n; ++i)
        {
            double sum = 0.0;
            for (int k = 0; k < i; ++k)
                sum += result.L(i, k) * result.L(i, k);
            double diag = A(i, i) - sum;
            if (diag <= 0)
            {
                result.success = false;
                result.errorMsg = "A 非正定";
                return result;
            }
            result.L(i, i) = std::sqrt(diag);
            for (int j = i + 1; j < n; ++j)
            {
                double s = 0.0;
                for (int k = 0; k < i; ++k)
                    s += result.L(j, k) * result.L(i, k);
                result.L(j, i) = (A(j, i) - s) / result.L(i, i);
            }
            {
                std::ostringstream oss;
                oss << "完成第 " << (i + 1) << " 列/行的分解";
                result.stepDesc.push_back(oss.str());
            }
        }
        result.U = Matrix(n, n, 0.0);
        for (int i = 0; i < n; ++i)
            for (int j = i; j < n; ++j)
                result.U(i, j) = result.L(j, i);

        std::vector<double> y(n, 0.0);
        for (int i = 0; i < n; ++i)
        {
            double s = 0.0;
            for (int k = 0; k < i; ++k)
                s += result.L(i, k) * y[k];
            y[i] = (b[i] - s) / result.L(i, i);
        }
        result.stepDesc.push_back("前向替代 Ly = b");
        result.solution.assign(n, 0.0);
        for (int i = n - 1; i >= 0; --i)
        {
            double s = 0.0;
            for (int k = i + 1; k < n; ++k)
                s += result.L(k, i) * result.solution[k];
            result.solution[i] = (y[i] - s) / result.L(i, i);
        }
        result.stepDesc.push_back("回代 L^T x = y");
        result.success = true;
        return result;
    }

    // 追赶法（Thomas）
    GaussResult thomasTridiagonal(const Matrix &A, const std::vector<double> &b)
    {
        GaussResult result;
        int n = A.rows();
        if (A.cols() != n || (int)b.size() != n)
        {
            result.success = false;
            result.errorMsg = "矩阵维度不匹配";
            return result;
        }
        std::vector<double> a(n, 0.0), d(n, 0.0), c(n, 0.0), cp(n, 0.0), dp(n, 0.0);
        for (int i = 0; i < n; ++i)
        {
            d[i] = A(i, i);
            if (i < n - 1)
                c[i] = A(i, i + 1);
            if (i > 0)
                a[i] = A(i, i - 1);
        }
        if (std::fabs(d[0]) < 1e-14)
        {
            result.success = false;
            result.errorMsg = "主对角含零";
            return result;
        }
        cp[0] = c[0] / d[0];
        dp[0] = b[0] / d[0];
        result.stepDesc.push_back("初始化 c'0, d'0");
        for (int i = 1; i < n; ++i)
        {
            double denom = d[i] - a[i] * cp[i - 1];
            if (std::fabs(denom) < 1e-14)
            {
                result.success = false;
                result.errorMsg = "分母为零";
                return result;
            }
            cp[i] = (i == n - 1) ? 0.0 : c[i] / denom;
            dp[i] = (b[i] - a[i] * dp[i - 1]) / denom;
            std::ostringstream oss;
            oss << "i=" << i + 1 << ": 更新 c' 与 d'";
            result.stepDesc.push_back(oss.str());
        }
        result.solution.assign(n, 0.0);
        result.solution[n - 1] = dp[n - 1];
        for (int i = n - 2; i >= 0; --i)
        {
            result.solution[i] = dp[i] - cp[i] * result.solution[i + 1];
        }
        result.stepDesc.push_back("回代求解 x");
        result.success = true;
        return result;
    }

    // 列主元素法
    GaussResult columnPivoting(const Matrix &A, const std::vector<double> &b)
    {
        GaussResult result;
        int n = A.rows();

        if (A.cols() != n || (int)b.size() != n)
        {
            result.success = false;
            result.errorMsg = "矩阵维度不匹配";
            return result;
        }

        result.L = Matrix(n, n, 0.0);
        result.U = Matrix(n, n, 0.0);
        for (int i = 0; i < n; ++i)
            result.L(i, i) = 1.0;

        Matrix augmented(n, n + 1);
        for (int i = 0; i < n; ++i)
        {
            for (int j = 0; j < n; ++j)
                augmented(i, j) = A(i, j);
            augmented(i, n) = b[i];
        }

        result.steps.push_back(augmented);
        result.stepDesc.push_back("初始增广矩阵 [A|b]");

        for (int k = 0; k < n - 1; ++k)
        {
            // 列主元：在第k列的k行及以下找最大元素
            int maxRow = k;
            double maxVal = std::abs(augmented(k, k));
            for (int i = k + 1; i < n; ++i)
            {
                if (std::abs(augmented(i, k)) > maxVal)
                {
                    maxVal = std::abs(augmented(i, k));
                    maxRow = i;
                }
            }

            if (maxRow != k)
            {
                for (int j = k; j <= n; ++j)
                    std::swap(augmented(k, j), augmented(maxRow, j));
                result.steps.push_back(augmented);
                std::ostringstream oss;
                oss << "列主元：交换第 " << (k + 1) << " 行和第 " << (maxRow + 1) << " 行";
                result.stepDesc.push_back(oss.str());
            }

            if (std::abs(augmented(k, k)) < 1e-14)
            {
                result.success = false;
                result.errorMsg = "矩阵奇异或接近奇异";
                return result;
            }

            for (int i = k + 1; i < n; ++i)
            {
                double factor = augmented(i, k) / augmented(k, k);
                result.L(i, k) = factor;
                for (int j = k; j <= n; ++j)
                    augmented(i, j) -= factor * augmented(k, j);
            }

            result.steps.push_back(augmented);
            std::ostringstream oss;
            oss << "消元第 " << (k + 1) << " 列";
            result.stepDesc.push_back(oss.str());
        }

        for (int i = 0; i < n; ++i)
            for (int j = i; j < n; ++j)
                result.U(i, j) = augmented(i, j);

        result.solution.resize(n);
        for (int i = n - 1; i >= 0; --i)
        {
            double sum = augmented(i, n);
            for (int j = i + 1; j < n; ++j)
                sum -= augmented(i, j) * result.solution[j];
            if (std::abs(augmented(i, i)) < 1e-14)
            {
                result.success = false;
                result.errorMsg = "矩阵奇异";
                return result;
            }
            result.solution[i] = sum / augmented(i, i);
        }

        result.success = true;
        return result;
    }

    // 全主元素法
    GaussResult fullPivoting(const Matrix &A, const std::vector<double> &b)
    {
        GaussResult result;
        int n = A.rows();

        if (A.cols() != n || (int)b.size() != n)
        {
            result.success = false;
            result.errorMsg = "矩阵维度不匹配";
            return result;
        }

        result.L = Matrix(n, n, 0.0);
        result.U = Matrix(n, n, 0.0);
        for (int i = 0; i < n; ++i)
            result.L(i, i) = 1.0;

        Matrix augmented(n, n + 1);
        for (int i = 0; i < n; ++i)
        {
            for (int j = 0; j < n; ++j)
                augmented(i, j) = A(i, j);
            augmented(i, n) = b[i];
        }

        result.steps.push_back(augmented);
        result.stepDesc.push_back("初始增广矩阵 [A|b]");

        std::vector<int> colOrder(n);
        for (int i = 0; i < n; ++i)
            colOrder[i] = i;

        for (int k = 0; k < n - 1; ++k)
        {
            // 全主元：在剩余子矩阵中找最大元素
            int maxRow = k, maxCol = k;
            double maxVal = std::abs(augmented(k, k));
            for (int i = k; i < n; ++i)
            {
                for (int j = k; j < n; ++j)
                {
                    if (std::abs(augmented(i, j)) > maxVal)
                    {
                        maxVal = std::abs(augmented(i, j));
                        maxRow = i;
                        maxCol = j;
                    }
                }
            }

            bool swapped = false;
            if (maxRow != k)
            {
                for (int j = k; j <= n; ++j)
                    std::swap(augmented(k, j), augmented(maxRow, j));
                swapped = true;
            }

            if (maxCol != k)
            {
                for (int i = 0; i < n; ++i)
                    std::swap(augmented(i, k), augmented(i, maxCol));
                std::swap(colOrder[k], colOrder[maxCol]);
                swapped = true;
            }

            if (swapped)
            {
                result.steps.push_back(augmented);
                std::ostringstream oss;
                oss << "全主元：交换至 (" << (k + 1) << "," << (k + 1) << ")";
                if (maxRow != k)
                    oss << " [行" << (maxRow + 1) << "]";
                if (maxCol != k)
                    oss << " [列" << (maxCol + 1) << "]";
                result.stepDesc.push_back(oss.str());
            }

            if (std::abs(augmented(k, k)) < 1e-14)
            {
                result.success = false;
                result.errorMsg = "矩阵奇异或接近奇异";
                return result;
            }

            for (int i = k + 1; i < n; ++i)
            {
                double factor = augmented(i, k) / augmented(k, k);
                result.L(i, k) = factor;
                for (int j = k; j <= n; ++j)
                    augmented(i, j) -= factor * augmented(k, j);
            }

            result.steps.push_back(augmented);
            std::ostringstream oss;
            oss << "消元第 " << (k + 1) << " 列";
            result.stepDesc.push_back(oss.str());
        }

        for (int i = 0; i < n; ++i)
            for (int j = i; j < n; ++j)
                result.U(i, j) = augmented(i, j);

        std::vector<double> tempSol(n);
        for (int i = n - 1; i >= 0; --i)
        {
            double sum = augmented(i, n);
            for (int j = i + 1; j < n; ++j)
                sum -= augmented(i, j) * tempSol[j];
            if (std::abs(augmented(i, i)) < 1e-14)
            {
                result.success = false;
                result.errorMsg = "矩阵奇异";
                return result;
            }
            tempSol[i] = sum / augmented(i, i);
        }

        result.solution.resize(n);
        for (int i = 0; i < n; ++i)
            result.solution[colOrder[i]] = tempSol[i];

        result.success = true;
        return result;
    }

    // ==================== 矩阵工具函数实现 ====================

    // 1-范数（列和范数）
    double matrixNorm1(const Matrix &A)
    {
        double maxColSum = 0.0;
        for (int j = 0; j < A.cols(); ++j)
        {
            double colSum = 0.0;
            for (int i = 0; i < A.rows(); ++i)
                colSum += std::abs(A(i, j));
            maxColSum = std::max(maxColSum, colSum);
        }
        return maxColSum;
    }

    // 2-范数（谱范数，最大奇异值）
    double matrixNorm2(const Matrix &A)
    {
        // 简化实现：使用幂法近似计算最大奇异值
        // 实际上是计算 A^T * A 的最大特征值的平方根
        int n = A.cols();
        Matrix AtA(n, n, 0.0);
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j)
                for (int k = 0; k < A.rows(); ++k)
                    AtA(i, j) += A(k, i) * A(k, j);
        double rho = spectralRadius(AtA, 100, 1e-6);
        return std::sqrt(rho);
    }

    // ∞-范数（行和范数）
    double matrixNormInf(const Matrix &A)
    {
        double maxRowSum = 0.0;
        for (int i = 0; i < A.rows(); ++i)
        {
            double rowSum = 0.0;
            for (int j = 0; j < A.cols(); ++j)
                rowSum += std::abs(A(i, j));
            maxRowSum = std::max(maxRowSum, rowSum);
        }
        return maxRowSum;
    }

    // F-范数（Frobenius范数）
    double matrixNormF(const Matrix &A)
    {
        double sum = 0.0;
        for (int i = 0; i < A.rows(); ++i)
            for (int j = 0; j < A.cols(); ++j)
                sum += A(i, j) * A(i, j);
        return std::sqrt(sum);
    }

    // 向量2-范数
    double vectorNorm2(const std::vector<double> &v)
    {
        double sum = 0.0;
        for (double val : v)
            sum += val * val;
        return std::sqrt(sum);
    }

    // 向量∞-范数
    double vectorNormInf(const std::vector<double> &v)
    {
        double maxVal = 0.0;
        for (double val : v)
            maxVal = std::max(maxVal, std::abs(val));
        return maxVal;
    }

    // 矩阵乘法
    Matrix matrixMultiply(const Matrix &A, const Matrix &B)
    {
        if (A.cols() != B.rows())
            return Matrix();
        Matrix C(A.rows(), B.cols(), 0.0);
        for (int i = 0; i < A.rows(); ++i)
            for (int j = 0; j < B.cols(); ++j)
                for (int k = 0; k < A.cols(); ++k)
                    C(i, j) += A(i, k) * B(k, j);
        return C;
    }

    // 矩阵向量乘法
    std::vector<double> matrixVectorMultiply(const Matrix &A, const std::vector<double> &x)
    {
        std::vector<double> result(A.rows(), 0.0);
        for (int i = 0; i < A.rows(); ++i)
            for (int j = 0; j < A.cols(); ++j)
                result[i] += A(i, j) * x[j];
        return result;
    }

    // 矩阵加法
    Matrix matrixAdd(const Matrix &A, const Matrix &B)
    {
        if (A.rows() != B.rows() || A.cols() != B.cols())
            return Matrix();
        Matrix C(A.rows(), A.cols());
        for (int i = 0; i < A.rows(); ++i)
            for (int j = 0; j < A.cols(); ++j)
                C(i, j) = A(i, j) + B(i, j);
        return C;
    }

    // 矩阵减法
    Matrix matrixSubtract(const Matrix &A, const Matrix &B)
    {
        if (A.rows() != B.rows() || A.cols() != B.cols())
            return Matrix();
        Matrix C(A.rows(), A.cols());
        for (int i = 0; i < A.rows(); ++i)
            for (int j = 0; j < A.cols(); ++j)
                C(i, j) = A(i, j) - B(i, j);
        return C;
    }

    // 矩阵数乘
    Matrix matrixScale(const Matrix &A, double scalar)
    {
        Matrix C(A.rows(), A.cols());
        for (int i = 0; i < A.rows(); ++i)
            for (int j = 0; j < A.cols(); ++j)
                C(i, j) = A(i, j) * scalar;
        return C;
    }

    // 谱半径（幂法近似）
    double spectralRadius(const Matrix &A, int maxIter, double tol)
    {
        int n = A.rows();
        if (n != A.cols())
            return 0.0;

        std::vector<double> v(n, 1.0);
        double lambda = 0.0;

        for (int iter = 0; iter < maxIter; ++iter)
        {
            std::vector<double> Av = matrixVectorMultiply(A, v);
            double lambdaNew = 0.0;
            for (int i = 0; i < n; ++i)
                if (std::abs(Av[i]) > std::abs(lambdaNew))
                    lambdaNew = Av[i];

            if (std::abs(lambdaNew) < 1e-14)
                return 0.0;

            for (int i = 0; i < n; ++i)
                v[i] = Av[i] / lambdaNew;

            if (std::abs(lambdaNew - lambda) < tol)
                return std::abs(lambdaNew);

            lambda = lambdaNew;
        }
        return std::abs(lambda);
    }

    // 检查矩阵是否严格对角占优
    bool isStrictlyDiagonallyDominant(const Matrix &A)
    {
        int n = A.rows();
        if (A.cols() != n)
            return false;

        for (int i = 0; i < n; ++i)
        {
            double diagAbs = std::abs(A(i, i));
            double rowSum = 0.0;
            for (int j = 0; j < n; ++j)
            {
                if (j != i)
                    rowSum += std::abs(A(i, j));
            }
            if (diagAbs <= rowSum) // 需要严格大于
                return false;
        }
        return true;
    }

    // 计算雅可比迭代矩阵的谱半径
    double jacobiSpectralRadius(const Matrix &A)
    {
        int n = A.rows();
        Matrix BJ(n, n, 0.0);
        for (int i = 0; i < n; ++i)
        {
            if (std::abs(A(i, i)) < 1e-14)
                return 999.0; // 对角元素为0，无法计算
            for (int j = 0; j < n; ++j)
            {
                if (i != j)
                    BJ(i, j) = -A(i, j) / A(i, i);
            }
        }
        return spectralRadius(BJ, 100, 1e-6);
    }

    // 计算SOR方法的最优松弛因子
    double optimalOmegaSOR(const Matrix &A)
    {
        double rhoJ = jacobiSpectralRadius(A);
        if (rhoJ >= 1.0 || rhoJ < 0.0)
            return 1.0;

        double mu = rhoJ;
        double omegaOpt = 2.0 / (1.0 + std::sqrt(1.0 - mu * mu));

        if (omegaOpt <= 0.0 || omegaOpt >= 2.0)
            return 1.0;

        return omegaOpt;
    }

    // ==================== 迭代法实现 ====================

    // 雅可比迭代法
    IterativeResult jacobiIteration(const Matrix &A, const std::vector<double> &b,
                                    const std::vector<double> &x0, int maxIter, double tol)
    {
        IterativeResult result;
        int n = A.rows();

        if (A.cols() != n || (int)b.size() != n || (int)x0.size() != n)
        {
            result.success = false;
            result.errorMsg = "矩阵维度不匹配";
            return result;
        }

        // 检查对角元素非零
        for (int i = 0; i < n; ++i)
        {
            if (std::abs(A(i, i)) < 1e-14)
            {
                result.success = false;
                result.errorMsg = "矩阵对角元素含零";
                return result;
            }
        }

        // 构造迭代矩阵 B_J = -D^(-1)(L+U)
        Matrix BJ(n, n, 0.0);
        for (int i = 0; i < n; ++i)
        {
            for (int j = 0; j < n; ++j)
            {
                if (i != j)
                    BJ(i, j) = -A(i, j) / A(i, i);
            }
        }
        result.iterationMatrix = BJ;
        result.spectralRadius = spectralRadius(BJ, 100, 1e-6);

        std::vector<double> x = x0;
        result.iterations.push_back(x);
        result.stepDesc.push_back("初始向量 x(0)");

        for (int iter = 1; iter <= maxIter; ++iter)
        {
            std::vector<double> xNew(n);
            for (int i = 0; i < n; ++i)
            {
                double sum = 0.0;
                for (int j = 0; j < n; ++j)
                {
                    if (j != i)
                        sum += A(i, j) * x[j];
                }
                xNew[i] = (b[i] - sum) / A(i, i);
            }

            // 计算误差：||x_new - x_old||_∞
            double error = 0.0;
            for (int i = 0; i < n; ++i)
                error = std::max(error, std::abs(xNew[i] - x[i]));

            result.iterations.push_back(xNew);
            result.errors.push_back(error);

            std::ostringstream oss;
            oss << "迭代 " << iter << ": error = " << error;
            result.stepDesc.push_back(oss.str());

            if (error < tol)
            {
                result.success = true;
                result.solution = xNew;
                return result;
            }

            x = xNew;
        }

        result.success = false;
        result.errorMsg = "达到最大迭代次数未收敛";
        result.solution = x;
        return result;
    }

    // 高斯-赛德尔迭代法
    IterativeResult gaussSeidelIteration(const Matrix &A, const std::vector<double> &b,
                                         const std::vector<double> &x0, int maxIter, double tol)
    {
        IterativeResult result;
        int n = A.rows();

        if (A.cols() != n || (int)b.size() != n || (int)x0.size() != n)
        {
            result.success = false;
            result.errorMsg = "矩阵维度不匹配";
            return result;
        }

        for (int i = 0; i < n; ++i)
        {
            if (std::abs(A(i, i)) < 1e-14)
            {
                result.success = false;
                result.errorMsg = "矩阵对角元素含零";
                return result;
            }
        }

        // 构造迭代矩阵 B_GS = -(D+L)^(-1)U
        // B_GS[i][j] 通过求解 (D+L)B = -U 得到
        Matrix BGS(n, n, 0.0);
        for (int j = 0; j < n; ++j)
        {
            // 对每一列求解 (D+L) * B[:,j] = -U[:,j]
            std::vector<double> rhs(n, 0.0);
            for (int i = 0; i < n; ++i)
            {
                if (i < j)
                    rhs[i] = -A(i, j); // U的元素
            }
            // 前代法求解下三角方程组
            for (int i = 0; i < n; ++i)
            {
                double sum = 0.0;
                for (int k = 0; k < i; ++k)
                    sum += A(i, k) * BGS(k, j);
                BGS(i, j) = (rhs[i] - sum) / A(i, i);
            }
        }
        result.iterationMatrix = BGS;
        result.spectralRadius = spectralRadius(BGS, 100, 1e-6);

        std::vector<double> x = x0;
        result.iterations.push_back(x);
        result.stepDesc.push_back("初始向量 x(0)");

        for (int iter = 1; iter <= maxIter; ++iter)
        {
            std::vector<double> xOld = x;
            // 高斯-赛德尔：原地更新，使用最新计算的值
            for (int i = 0; i < n; ++i)
            {
                double sum = 0.0;
                for (int j = 0; j < n; ++j)
                {
                    if (j != i)
                        sum += A(i, j) * x[j]; // 使用x：j<i时是新值，j>i时是旧值
                }
                x[i] = (b[i] - sum) / A(i, i);
            }

            double error = 0.0;
            for (int i = 0; i < n; ++i)
                error = std::max(error, std::abs(x[i] - xOld[i]));

            result.iterations.push_back(x);
            result.errors.push_back(error);

            std::ostringstream oss;
            oss << "迭代 " << iter << ": error = " << error;
            result.stepDesc.push_back(oss.str());

            if (error < tol)
            {
                result.success = true;
                result.solution = x;
                return result;
            }
        }

        result.success = false;
        result.errorMsg = "达到最大迭代次数未收敛";
        result.solution = x;
        return result;
    }

    // 松弛迭代法（SOR）
    IterativeResult sorIteration(const Matrix &A, const std::vector<double> &b,
                                 const std::vector<double> &x0, int maxIter, double tol, double omega)
    {
        IterativeResult result;
        int n = A.rows();

        if (A.cols() != n || (int)b.size() != n || (int)x0.size() != n)
        {
            result.success = false;
            result.errorMsg = "矩阵维度不匹配";
            return result;
        }

        if (omega <= 0.0 || omega >= 2.0)
        {
            result.success = false;
            result.errorMsg = "松弛因子 ω 必须在 (0, 2) 范围内";
            return result;
        }

        for (int i = 0; i < n; ++i)
        {
            if (std::abs(A(i, i)) < 1e-14)
            {
                result.success = false;
                result.errorMsg = "矩阵对角元素含零";
                return result;
            }
        }

        // 构造迭代矩阵 B_ω = (D+ωL)^{-1}[(1-ω)D - ωU]
        // 通过求解 (D+ωL)B = (1-ω)D - ωU 得到
        Matrix BSOR(n, n, 0.0);
        for (int j = 0; j < n; ++j)
        {
            // 对每一列求解 (D+ωL) * B[:,j] = [(1-ω)D - ωU][:,j]
            std::vector<double> rhs(n, 0.0);
            for (int i = 0; i < n; ++i)
            {
                rhs[i] = (i == j ? (1.0 - omega) * A(i, i) : 0.0); // (1-ω)D部分
                if (i < j)
                    rhs[i] -= omega * A(i, j); // -ωU部分
            }
            // 前代法求解 (D+ωL)x = rhs
            for (int i = 0; i < n; ++i)
            {
                double sum = 0.0;
                for (int k = 0; k < i; ++k)
                    sum += omega * A(i, k) * BSOR(k, j);
                BSOR(i, j) = (rhs[i] - sum) / A(i, i);
            }
        }
        result.iterationMatrix = BSOR;
        result.spectralRadius = spectralRadius(BSOR, 100, 1e-6);

        std::vector<double> x = x0;
        result.iterations.push_back(x);
        std::ostringstream oss0;
        oss0 << "初始向量 x(0), ω = " << omega;
        result.stepDesc.push_back(oss0.str());

        for (int iter = 1; iter <= maxIter; ++iter)
        {
            std::vector<double> xOld = x;
            // SOR: 原地更新，使用最新计算的值
            for (int i = 0; i < n; ++i)
            {
                double sum = 0.0;
                for (int j = 0; j < n; ++j)
                {
                    if (j != i)
                        sum += A(i, j) * x[j]; // 使用x：j<i时是新值，j>i时是旧值
                }
                // SOR公式: x_i = (1-ω)x_i^old + (ω/a_ii)(b_i - sum)
                x[i] = (1.0 - omega) * x[i] + (omega / A(i, i)) * (b[i] - sum);
            }

            double error = 0.0;
            for (int i = 0; i < n; ++i)
                error = std::max(error, std::abs(x[i] - xOld[i]));

            result.iterations.push_back(x);
            result.errors.push_back(error);

            std::ostringstream oss;
            oss << "迭代 " << iter << ": error = " << error;
            result.stepDesc.push_back(oss.str());

            if (error < tol)
            {
                result.success = true;
                result.solution = x;
                return result;
            }
        }

        result.success = false;
        result.errorMsg = "达到最大迭代次数未收敛";
        result.solution = x;
        return result;
    }

    // ==================== 插值法基础工具 ====================

    // 阶乘
    long long factorial(int n)
    {
        if (n < 0)
            return 0;
        if (n == 0 || n == 1)
            return 1;
        long long result = 1;
        for (int i = 2; i <= n; ++i)
            result *= i;
        return result;
    }

    // 广义组合数 C(t, n) = t(t-1)(t-2)...(t-n+1) / n!
    double generalizedBinomial(double t, int n)
    {
        if (n < 0)
            return 0.0;
        if (n == 0)
            return 1.0;

        double numerator = 1.0;
        for (int i = 0; i < n; ++i)
            numerator *= (t - i);

        double denominator = static_cast<double>(factorial(n));
        return numerator / denominator;
    }

    // 前向差分表
    std::vector<std::vector<double>> forwardDifferenceTable(const std::vector<double> &y)
    {
        int n = y.size();
        if (n == 0)
            return {};

        // table[i][j] 表示 Δ^j y_i
        std::vector<std::vector<double>> table(n);

        // 第 0 列：原始函数值
        for (int i = 0; i < n; ++i)
        {
            table[i].resize(n - i);
            table[i][0] = y[i];
        }

        // 计算各阶差分
        for (int j = 1; j < n; ++j) // 差分阶数
        {
            for (int i = 0; i < n - j; ++i) // 起始位置
            {
                table[i][j] = table[i + 1][j - 1] - table[i][j - 1];
            }
        }

        return table;
    }

    // 后向差分表
    std::vector<std::vector<double>> backwardDifferenceTable(const std::vector<double> &y)
    {
        int n = y.size();
        if (n == 0)
            return {};

        // table[i][j] 表示 ∇^j y_i
        std::vector<std::vector<double>> table(n);

        // 第 0 列：原始函数值
        for (int i = 0; i < n; ++i)
        {
            table[i].resize(i + 1);
            table[i][0] = y[i];
        }

        // 计算各阶后向差分
        for (int j = 1; j < n; ++j) // 差分阶数
        {
            for (int i = j; i < n; ++i) // 起始位置（从第 j 个元素开始）
            {
                table[i][j] = table[i][j - 1] - table[i - 1][j - 1];
            }
        }

        return table;
    }

    // 差商表
    std::vector<std::vector<double>> dividedDifferenceTable(const std::vector<double> &x,
                                                            const std::vector<double> &y)
    {
        int n = x.size();
        if (n == 0 || x.size() != y.size())
            return {};

        // table[i][j] 表示 f[x_i, x_{i+1}, ..., x_{i+j}]
        std::vector<std::vector<double>> table(n);

        // 第 0 列：原始函数值 f[x_i]
        for (int i = 0; i < n; ++i)
        {
            table[i].resize(n - i);
            table[i][0] = y[i];
        }

        // 计算各阶差商
        for (int j = 1; j < n; ++j) // 差商阶数
        {
            for (int i = 0; i < n - j; ++i) // 起始位置
            {
                // f[x_i, ..., x_{i+j}] = (f[x_{i+1}, ..., x_{i+j}] - f[x_i, ..., x_{i+j-1}]) / (x_{i+j} - x_i)
                table[i][j] = (table[i + 1][j - 1] - table[i][j - 1]) / (x[i + j] - x[i]);
            }
        }

        return table;
    }

    // ==================== 插值方法选择 ====================

    InterpolationMethodInfo selectInterpolationMethod(const std::vector<double> &x, double xVal)
    {
        InterpolationMethodInfo info;
        int n = x.size();

        if (n < 2)
        {
            info.reason = "节点数量不足（需要至少2个节点）";
            return info;
        }

        // 检查等距性
        double h = x[1] - x[0];
        info.h = h;
        info.isEquidistant = true;
        const double tol = 1e-6;

        for (int i = 2; i < n; ++i)
        {
            double diff = x[i] - x[i - 1];
            if (std::fabs(diff - h) > tol)
            {
                info.isEquidistant = false;
                info.reason = "节点不等距，建议使用牛顿差商公式";
                info.recommendedMethod = "divided_difference";
                return info;
            }
        }

        if (n < 5)
        {
            info.reason = "等距节点数量不足5个，建议补充节点";
            info.recommendedMethod = "forward";
            info.baseIndex = 0;
            info.t = (xVal - x[0]) / h;
            return info;
        }

        // 等距节点，判断 xVal 的位置
        // 计算 xVal 相对于各节点的位置
        if (xVal < x[0])
        {
            // 外推：在最前面
            info.recommendedMethod = "forward";
            info.baseIndex = 0;
            info.t = (xVal - x[0]) / h;
            info.reason = "x在区间左侧，推荐前插公式";
        }
        else if (xVal > x[n - 1])
        {
            // 外推：在最后面
            info.recommendedMethod = "backward";
            info.baseIndex = n - 1;
            info.t = (xVal - x[n - 1]) / h;
            info.reason = "x在区间右侧，推荐后插公式";
        }
        else
        {
            // 在区间内部
            // 找到 xVal 所在的子区间
            int idx = 0;
            for (int i = 0; i < n - 1; ++i)
            {
                if (xVal >= x[i] && xVal <= x[i + 1])
                {
                    idx = i;
                    break;
                }
            }

            // 计算相对位置
            double relPos = static_cast<double>(idx) / (n - 1);

            if (relPos < 0.25)
            {
                // 前部区间，使用前插
                info.recommendedMethod = "forward";
                info.baseIndex = 0;
                info.t = (xVal - x[0]) / h;
                info.reason = "x在区间前1/4部分，推荐前插公式";
            }
            else if (relPos > 0.75)
            {
                // 后部区间，使用后插
                info.recommendedMethod = "backward";
                info.baseIndex = n - 1;
                info.t = (xVal - x[n - 1]) / h;
                info.reason = "x在区间后1/4部分，推荐后插公式";
            }
            else
            {
                // 中部区间，使用斯梯林或贝塞尔
                // 找最近的中心节点
                int centerIdx = n / 2;
                double tCenter = (xVal - x[centerIdx]) / h;

                if (std::fabs(tCenter) < 0.5)
                {
                    // 靠近整数节点，使用斯梯林
                    info.recommendedMethod = "stirling";
                    info.baseIndex = centerIdx;
                    info.t = tCenter;
                    info.reason = "x在区间中部且靠近节点，推荐斯梯林公式";
                }
                else
                {
                    // 靠近半整数节点，使用贝塞尔
                    info.recommendedMethod = "bessel";
                    info.baseIndex = centerIdx;
                    info.t = tCenter;
                    info.reason = "x在区间中部且靠近半节点，推荐贝塞尔公式";
                }
            }
        }

        return info;
    }

    // ==================== 牛顿差商插值（不等距） ====================

    InterpolationResult newtonDividedDifference(const std::vector<double> &x,
                                                const std::vector<double> &y,
                                                double xVal)
    {
        InterpolationResult result;
        int n = x.size();

        if (n == 0 || x.size() != y.size())
        {
            result.errorMsg = "输入数据为空或维度不匹配";
            return result;
        }

        // 计算差商表
        auto table = dividedDifferenceTable(x, y);
        if (table.empty())
        {
            result.errorMsg = "差商表计算失败";
            return result;
        }

        result.table = table;

        // 牛顿差商公式：P_n(x) = f[x_0] + (x-x_0)f[x_0,x_1] + (x-x_0)(x-x_1)f[x_0,x_1,x_2] + ...
        double value = table[0][0]; // f[x_0]
        double term = 1.0;

        std::ostringstream polyStream;
        polyStream << std::fixed << std::setprecision(6);
        polyStream << "P_n(x) = " << table[0][0];

        for (int i = 1; i < n; ++i)
        {
            term *= (xVal - x[i - 1]);
            double increment = term * table[0][i];
            value += increment;

            result.coefficients.push_back(table[0][i]);

            polyStream << "\n       + " << table[0][i] << "·";
            for (int j = 0; j < i; ++j)
            {
                polyStream << "(x-" << x[j] << ")";
            }
        }

        result.value = value;
        result.polynomial = polyStream.str();
        result.method = "牛顿差商公式（不等距）";
        result.success = true;

        std::ostringstream stepStream;
        stepStream << "使用牛顿差商公式计算 P_n(" << xVal << ")";
        result.stepDesc.push_back(stepStream.str());

        return result;
    }

    // ==================== 牛顿前插公式（等距） ====================

    InterpolationResult newtonForwardDifference(const std::vector<double> &x,
                                                const std::vector<double> &y,
                                                double xVal)
    {
        InterpolationResult result;
        int n = x.size();

        if (n == 0)
        {
            result.errorMsg = "输入数据为空";
            return result;
        }

        double h = x[1] - x[0];
        double x0 = x[0];
        double t = (xVal - x0) / h;

        result.t = t;
        result.baseIndex = 0;

        // 计算前向差分表
        auto table = forwardDifferenceTable(y);
        if (table.empty())
        {
            result.errorMsg = "前向差分表计算失败";
            return result;
        }

        result.table = table;

        // 牛顿前插公式：P_n(x) = y_0 + C(t,1)Δy_0 + C(t,2)Δ²y_0 + ... + C(t,n)Δⁿy_0
        double value = table[0][0]; // y_0

        std::ostringstream polyStream;
        polyStream << std::fixed << std::setprecision(6);
        polyStream << "P_n(x) = " << table[0][0];

        for (int i = 1; i < n && i < (int)table[0].size(); ++i)
        {
            double coeff = generalizedBinomial(t, i);
            double increment = coeff * table[0][i];
            value += increment;

            result.coefficients.push_back(table[0][i]);

            polyStream << "\n       + C(t," << i << ")·Δ^" << i << "y_0 = " << coeff << " × " << table[0][i];
        }

        result.value = value;
        result.polynomial = polyStream.str();
        result.method = "牛顿前插公式";
        result.success = true;

        std::ostringstream stepStream;
        stepStream << "使用前插公式，x0=" << x0 << ", h=" << h << ", t=" << t;
        result.stepDesc.push_back(stepStream.str());

        return result;
    }

    // ==================== 牛顿后插公式（等距） ====================

    InterpolationResult newtonBackwardDifference(const std::vector<double> &x,
                                                 const std::vector<double> &y,
                                                 double xVal)
    {
        InterpolationResult result;
        int n = x.size();

        if (n == 0)
        {
            result.errorMsg = "输入数据为空";
            return result;
        }

        double h = x[1] - x[0];
        double xn = x[n - 1];
        double t = (xVal - xn) / h;

        result.t = t;
        result.baseIndex = n - 1;

        // 计算后向差分表
        auto table = backwardDifferenceTable(y);
        if (table.empty())
        {
            result.errorMsg = "后向差分表计算失败";
            return result;
        }

        result.table = table;

        // 牛顿后插公式：P_n(x) = y_n + C(t,1)∇y_n + C(t,2)∇²y_n + ... + C(t,n)∇ⁿy_n
        double value = table[n - 1][0]; // y_n

        std::ostringstream polyStream;
        polyStream << std::fixed << std::setprecision(6);
        polyStream << "P_n(x) = " << table[n - 1][0];

        for (int i = 1; i < n && i < (int)table[n - 1].size(); ++i)
        {
            double coeff = generalizedBinomial(t, i);
            double increment = coeff * table[n - 1][i];
            value += increment;

            result.coefficients.push_back(table[n - 1][i]);

            polyStream << "\n       + C(t," << i << ")·∇^" << i << "y_n = " << coeff << " × " << table[n - 1][i];
        }

        result.value = value;
        result.polynomial = polyStream.str();
        result.method = "牛顿后插公式";
        result.success = true;

        std::ostringstream stepStream;
        stepStream << "使用后插公式，xn=" << xn << ", h=" << h << ", t=" << t;
        result.stepDesc.push_back(stepStream.str());

        return result;
    }

    // ==================== 斯梯林插值公式 ====================

    InterpolationResult stirlingInterpolation(const std::vector<double> &x,
                                              const std::vector<double> &y,
                                              double xVal)
    {
        InterpolationResult result;
        int n = x.size();

        if (n < 3)
        {
            result.errorMsg = "斯梯林公式需要至少3个节点";
            return result;
        }

        // 选择中心节点
        int centerIdx = n / 2;
        double x0 = x[centerIdx];
        double h = x[1] - x[0];
        double t = (xVal - x0) / h;

        result.t = t;
        result.baseIndex = centerIdx;

        // 计算前向和后向差分表
        auto fwdTable = forwardDifferenceTable(y);
        auto bwdTable = backwardDifferenceTable(y);

        if (fwdTable.empty() || bwdTable.empty())
        {
            result.errorMsg = "差分表计算失败";
            return result;
        }

        result.table = fwdTable; // 存储前向差分表作为参考

        // 斯梯林公式：使用中心差分的平均
        // P(x) = y_0 + t·(Δy_{-1}+Δy_0)/2 + C(t,2)·Δ²y_{-1} + ...
        // 简化实现：使用前插和后插的平均
        auto fwdResult = newtonForwardDifference(x, y, xVal);
        auto bwdResult = newtonBackwardDifference(x, y, xVal);

        if (!fwdResult.success || !bwdResult.success)
        {
            result.errorMsg = "前插或后插计算失败";
            return result;
        }

        result.value = (fwdResult.value + bwdResult.value) / 2.0;
        result.method = "斯梯林插值公式";
        result.polynomial = "斯梯林公式（前插与后插平均）";
        result.success = true;

        std::ostringstream stepStream;
        stepStream << "使用斯梯林公式，中心节点 x0=" << x0 << ", h=" << h << ", t=" << t;
        result.stepDesc.push_back(stepStream.str());
        stepStream.str("");
        stepStream << "前插值=" << fwdResult.value << ", 后插值=" << bwdResult.value << ", 平均=" << result.value;
        result.stepDesc.push_back(stepStream.str());

        return result;
    }

    // ==================== 贝塞尔插值公式 ====================

    InterpolationResult besselInterpolation(const std::vector<double> &x,
                                            const std::vector<double> &y,
                                            double xVal)
    {
        InterpolationResult result;
        int n = x.size();

        if (n < 3)
        {
            result.errorMsg = "贝塞尔公式需要至少3个节点";
            return result;
        }

        // 选择中心节点
        int centerIdx = n / 2;
        double x0 = x[centerIdx];
        double h = x[1] - x[0];
        double t = (xVal - x0) / h;

        result.t = t;
        result.baseIndex = centerIdx;

        // 贝塞尔公式适用于 t 在 [-0.5, 0.5] 附近
        // 简化实现：类似斯梯林，使用前插和后插的加权平均
        auto fwdResult = newtonForwardDifference(x, y, xVal);
        auto bwdResult = newtonBackwardDifference(x, y, xVal);

        if (!fwdResult.success || !bwdResult.success)
        {
            result.errorMsg = "前插或后插计算失败";
            return result;
        }

        // 贝塞尔的权重根据 t 调整
        double w = 0.5 + t; // 简单权重
        if (w < 0)
            w = 0;
        if (w > 1)
            w = 1;

        result.value = w * fwdResult.value + (1 - w) * bwdResult.value;
        result.method = "贝塞尔插值公式";
        result.polynomial = "贝塞尔公式（前插与后插加权平均）";
        result.success = true;
        result.table = fwdResult.table; // 使用前插的差分表

        std::ostringstream stepStream;
        stepStream << "使用贝塞尔公式，中心节点 x0=" << x0 << ", h=" << h << ", t=" << t;
        result.stepDesc.push_back(stepStream.str());
        stepStream.str("");
        stepStream << "前插值=" << fwdResult.value << ", 后插值=" << bwdResult.value;
        stepStream.str("");
        stepStream << "权重w=" << w << ", 结果=" << result.value;
        result.stepDesc.push_back(stepStream.str());

        return result;
    }

    // ==================== 拉格朗日插值公式 ====================

    InterpolationResult lagrangeInterpolation(const std::vector<double> &x,
                                              const std::vector<double> &y,
                                              double xVal)
    {
        InterpolationResult result;
        int n = x.size();

        if (n == 0 || x.size() != y.size())
        {
            result.errorMsg = "输入数据为空或维度不匹配";
            return result;
        }

        if (n == 1)
        {
            result.value = y[0];
            result.polynomial = "L_0(x) = " + std::to_string(y[0]);
            result.method = "拉格朗日插值公式";
            result.success = true;
            return result;
        }

        // 拉格朗日插值公式：L_n(x) = Σ l_i(x)·f(x_i)
        // 其中 l_i(x) = Π[j≠i] (x-x_j)/(x_i-x_j)
        double value = 0.0;
        std::ostringstream polyStream;
        polyStream << std::fixed << std::setprecision(6);
        polyStream << "L_n(x) = ";

        std::vector<double> lagrangeBasis(n); // 存储每个基函数 l_i(xVal) 的值

        for (int i = 0; i < n; ++i)
        {
            // 计算第 i 个拉格朗日基函数 l_i(xVal)
            double li = 1.0;
            for (int j = 0; j < n; ++j)
            {
                if (j != i)
                {
                    li *= (xVal - x[j]) / (x[i] - x[j]);
                }
            }
            lagrangeBasis[i] = li;
            value += li * y[i];

            // 构建多项式表达式
            if (i > 0)
                polyStream << "\n       + ";

            polyStream << "l_" << i << "(x)·f(x_" << i << ") = " << li << " × " << y[i];

            // 添加基函数的详细表达式到步骤说明中
            std::ostringstream basisDesc;
            basisDesc << std::fixed << std::setprecision(6);
            basisDesc << "l_" << i << "(x) = ";
            for (int j = 0; j < n; ++j)
            {
                if (j != i)
                {
                    if (j > 0 && j != i + 1 && (i == 0 || j > 1))
                        basisDesc << " × ";
                    basisDesc << "(x-" << x[j] << ")/(" << x[i] << "-" << x[j] << ")";
                }
            }
            basisDesc << " = " << li;
            result.stepDesc.push_back(basisDesc.str());
        }

        result.value = value;
        result.polynomial = polyStream.str();
        result.method = "拉格朗日插值公式";
        result.success = true;

        // 存储基函数值作为系数
        result.coefficients = lagrangeBasis;

        std::ostringstream summaryStream;
        summaryStream << "使用拉格朗日插值公式计算 L_n(" << xVal << ") = " << value;
        result.stepDesc.insert(result.stepDesc.begin(), summaryStream.str());

        return result;
    }
}

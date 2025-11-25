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
}

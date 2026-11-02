#include "core.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
namespace tb {
void validate(const Rod &r) {
    if (r.n < 2 || r.n > 200000)
        throw std::runtime_error("Elements must be an integer from 2 to 200000.");
    for (double v : {r.L, r.A, r.k, r.q, r.left, r.right})
        if (!std::isfinite(v))
            throw std::runtime_error("All rod parameters must be finite.");
    if (r.L <= 0 || r.A <= 0 || r.k <= 0 || r.q < 0)
        throw std::runtime_error(
            "Length, area and conductivity must be positive; heat generation must be nonnegative.");
    if (r.L > 1e4 || r.L < 1e-6 || r.A > 1e4 || r.A < 1e-12 || r.k > 1e8 || r.k < 1e-6 ||
        r.q > 1e10 || std::abs(r.left) > 1e6 || std::abs(r.right) > 1e6)
        throw std::runtime_error("Parameters exceed the documented numerical range.");
}
double exact(const Rod &r, double x) {
    return r.left + (r.right - r.left) * x / r.L + r.q / (2 * r.k) * x * (r.L - x);
}
Solution solve(const Rod &r) {
    validate(r);
    const int n = r.n;
    const double h = r.L / n, ke = r.k * r.A / h, fe = r.q * r.A * h / 2;
    std::vector<double> diag(n + 1, 0), upper(n, 0), lower(n, 0), rhs(n + 1, 0);
    // Two-node element: ke*[1,-1;-1,1], fe*[1;1].
    for (int e = 0; e < n; ++e) {
        diag[e] += ke;
        diag[e + 1] += ke;
        upper[e] -= ke;
        lower[e] -= ke;
        rhs[e] += fe;
        rhs[e + 1] += fe;
    }
    // Eliminate prescribed endpoint columns before replacing endpoint rows.
    rhs[1] -= lower[0] * r.left;
    rhs[n - 1] -= upper[n - 1] * r.right;
    lower[0] = upper[0] = lower[n - 1] = upper[n - 1] = 0;
    diag[0] = diag[n] = 1;
    rhs[0] = r.left;
    rhs[n] = r.right;
    for (int i = 1; i <= n; ++i) {
        double m = lower[i - 1] / diag[i - 1];
        diag[i] -= m * upper[i - 1];
        rhs[i] -= m * rhs[i - 1];
    }
    Solution s;
    s.x.resize(n + 1);
    s.t.resize(n + 1);
    s.t[n] = rhs[n] / diag[n];
    for (int i = n - 1; i >= 0; --i)
        s.t[i] = (rhs[i] - upper[i] * s.t[i + 1]) / diag[i];
    for (int i = 0; i <= n; ++i) {
        s.x[i] = i * h;
        if (!std::isfinite(s.t[i]))
            throw std::runtime_error("Non-finite solve: reduce parameter magnitudes.");
    }
    for (int e = 0; e < n; ++e)
        s.midpoint_error = std::max(s.midpoint_error,
                                    std::abs(.5 * (s.t[e] + s.t[e + 1]) - exact(r, (e + .5) * h)));
    return s;
}
std::string thermal_csv(const Rod &r, const Solution &s) {
    std::ostringstream o;
    o << std::setprecision(17)
      << "sample,x_m,FE_temperature_C,exact_temperature_C,absolute_error_C\n";
    for (int i = 0; i <= r.n; ++i) {
        double x = s.x[i];
        o << "node," << x << ',' << s.t[i] << ',' << exact(r, x) << ','
          << std::abs(s.t[i] - exact(r, x)) << '\n';
        if (i < r.n) {
            x = (i + .5) * r.L / r.n;
            double t = .5 * (s.t[i] + s.t[i + 1]);
            o << "midpoint," << x << ',' << t << ',' << exact(r, x) << ','
              << std::abs(t - exact(r, x)) << '\n';
        }
    }
    return o.str();
}
} // namespace tb

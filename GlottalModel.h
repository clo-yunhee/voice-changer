#ifndef GLOTTALMODEL_H
#define GLOTTALMODEL_H

#include <cmath>

constexpr double PI = 3.1415926535897932384626433832795028841971693993751058209749445923078164062;

namespace vc::model {

class LFGenerator {
public:
    LFGenerator(double Rd = 1.7, int periodInSamples = 256);

    void setRd(double Rd);
    void setPeriod(int periodInSamples);

    double generateFrame(double *time);

private:
    void calculateTimeParameters();
    void calculateImplicitParameters();

    double m_Rd;
    int mPeriodInSamples;
    double m_t;

    // Calculated parameters.
    double m_Tc;
    double m_Tp;
    double m_Te;
    double m_Ta;
    double m_epsilon;
    double m_alpha;
};

template<typename Func, typename Func2>
double fzero(const Func& f, const Func2& df, double x0,
                const double tol = 1e-7,
                const double eps = 1e-13,
                const int maxIter = 50)
{
    for (int iter = 0; iter < maxIter; ++iter) {
        double y = f(x0);
        double dy = df(x0);
        if (std::abs(dy) < eps)
            return x0;
        double x1 = x0 - y / dy;
        if (std::abs(x1 - x0) <= tol)
            return x1;
        x0 = x1;
    }
    return x0;
}

}

#endif // GLOTTALMODEL_H
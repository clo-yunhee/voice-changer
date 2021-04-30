#include "Lpc.h"
#include "AberthRoots.h"
#include <cmath>
#include <complex>
#include <random>
#include <iostream>
#include <wx/app.h>

std::vector<double> vc::model::Lpc::sWindow;

std::vector<double> vc::model::Lpc::analyze(const std::vector<double>& data, double preemphFrequency, double sampleRate)
{
    const int N = static_cast<int>(data.size());

    if (sWindow.size() != N)
        makeWindow(N);

    const int lpcOrder = (int) std::round(2.5 + sampleRate / 1000);

    auto x = applyWindowAndPreemphasis(data, preemphFrequency, sampleRate);
    auto lpca = calculatePredictionCoefficients(data, lpcOrder);

    return lpca;
}

std::vector<double> vc::model::Lpc::applyFilter(const std::vector<double>& lpc, const std::vector<double>& x)
{
    const int n = static_cast<int>(x.size());
    const int m = static_cast<int>(lpc.size());

    std::vector<double> y(n);
    for (int i = 0; i < n; ++i) {
        y[i] = x[i];
        for (int j = 0; j < m; ++j)
            if (i - 1 - j >= 0)
                y[i] -= lpc[j] * y[i - 1 - j];
    }
    return y;
}

const std::vector<double>& vc::model::Lpc::window(int N)
{
    if (sWindow.size() != N)
        makeWindow(N);
    return sWindow;
}

void vc::model::Lpc::makeWindow(const int N)
{
    sWindow.resize(N);
    const double edge = std::exp(-12.0);
    for (int i = 0; i < N; ++i) {
    const double imid = 0.5 * (N + 1);
        sWindow[i] = (std::exp(-48.0 * (i - imid) * (i - imid) / (N + 1) / (N + 1)) - edge) / (1.0 - edge);
    }
}

std::vector<double> vc::model::Lpc::applyWindowAndPreemphasis(
        const std::vector<double>& data,
        double preemphFrequency, double sampleRate)
{
    const double preemph = std::exp(-(2.0 * PI * preemphFrequency) / sampleRate);
    const int N = static_cast<int>(data.size());

    std::vector<double> x(data.begin(), data.end());
    for (int i = N - 1; i >= 1; --i)
        x[i] = sWindow[i] * (x[i] - preemph * x[i - 1]);
    x[0] *= sWindow[0];
    return x;
}

std::vector<double> vc::model::Lpc::calculatePredictionCoefficients(const std::vector<double>& data, const int m)
{
    const int n = static_cast<int>(data.size());

    std::vector<double> aut(m + 1);
    std::vector<double> lpc(m);
    double error;
    double epsilon;
    int i, j;

    j = m + 1;
    while (j--) {
        double d = 0.0;
        for (i = j; i < n; ++i)
            d += data[i] * data[i - j];
        aut[j] = d;
    }

    error = aut[0] * (1.0 + 1e-10);
    epsilon = 1e-9 * aut[0] + 1e-10;

    for (i = 0; i < m; ++i) {
        double r = -aut[i + 1];

        if (error < epsilon) {
            for (j = i; j < m; ++j)
                lpc[j] = 0.0;
            goto done;
        }

        for (j = 0; j < i; ++j)
            r -= lpc[j] * aut[i - j];
        r /= error;

        lpc[i] = r;
        for (j = 0; j < i / 2; ++j) {
            double tmp = lpc[j];
            lpc[j] += r * lpc[i - 1 - j];
            lpc[i - 1 - j] += r * tmp;
        }
        if (i & 1)
            lpc[j] += lpc[j] * r;

        error *= 1.0 - r * r;
    }

done:
    /*double g = 0.99;
    double damp = g;
    for (j = 0; j < m; ++j) {
        lpc[j] *= damp;
        damp *= g;
    }*/

    return lpc;
}

static std::vector<double> polyFromRoots(const std::vector<std::complex<double>>& R)
{
    std::vector<std::complex<double>> P(R.size() + 1, 0.0);

    P[0] = 1.0;
    for (int j = 0; j < R.size(); ++j) {
        auto& zj = R[j];
        for (int i = j; i >= 0; --i) {
            P[i + 1] -= zj * P[i];
        }
    }

    std::vector<double> Pr(P.size());
    for (int i = 0; i < P.size(); ++i) {
        Pr[i] = P[i].real();
    }
    return Pr;
}

std::vector<double> vc::model::Lpc::applyFrequencyShift(const std::vector<double>& lpca, double shiftFactor)
{
    //  The process is as follows:
    // 1. Turn LPC into polynomial representation.
    // 2. Solve the polynomial for complex roots.
    // 3. Apply phase shift to each root.
    // 4. Find the coefficients for the new polynomial with shifted roots.
    
    std::vector<double> P(lpca.size() + 1);
    P[0] = 1.0;
    for (int i = 0; i < lpca.size(); ++i) {
        P[i + 1] = lpca[i];
    }

    std::vector<std::complex<double>> R = solveRoots(P);

    std::vector<std::complex<double>> Rs;
    for (const auto& z : R) {
        const double mag = std::abs(z);
        const double phi = std::arg(z);

        if (std::abs(shiftFactor * phi) < PI) {
            Rs.push_back(std::polar(mag, shiftFactor * phi));
        }
    }

    std::vector<double> Ps = polyFromRoots(Rs);

    std::vector<double> lpcaShifted(Ps.begin() + 1, Ps.end());

    return lpcaShifted;
}
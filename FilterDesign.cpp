#include "FilterDesign.h"

#include <algorithm>
#include <iostream>

static std::pair<std::vector<std::complex<double>>, std::vector<double>>
    cplxreal(const std::vector<std::complex<double>>& z_)
{
    constexpr double tol = 100.0 * std::numeric_limits<double>::epsilon();

    std::vector<std::complex<double>> z(z_.begin(), z_.end());
    std::sort(z.begin(), z.end(), [](const auto& x, const auto& y) { return x.real() < y.real(); });

    std::vector<std::complex<double>> cplx;
    std::vector<double> real;

    for (const auto& x : z) {
        if (std::abs(x.imag()) / (std::abs(x) + std::numeric_limits<double>::min()) <= tol) {
            real.push_back(x.real());
        }
        else {
            cplx.push_back(x);
        }
    }

    const int m = static_cast<int>(cplx.size());
    std::vector<std::complex<double>> zc;

    for (int i = 0; i < m; i += 2) {
        double v = std::numeric_limits<double>::max();
        int idx;
        for (int k = i + 1; k < m; ++k) {
            double vk = std::abs(z[k] - std::conj(z[i]));
            if (vk < v) {
                v = vk;
                idx = k;
            }
        }
        if (v >= tol * std::abs(z[i])) {
            std::cerr << "cplxreal: Could not pair all complex numbers" << std::endl;
            break;
        }
        if (z[i].imag() < 0) {
            std::swap(z[i], z[idx]);
        }
        std::swap(z[idx], z[i + 1]);
        zc.push_back(z[i]);
    }
    
    return { std::move(zc), std::move(real) };
}

std::vector<std::array<double, 6>> vc::model::zpk2sos(
    const std::vector<std::complex<double>> &z,
    const std::vector<std::complex<double>> &p,
    double k)
{
    auto [zc, zr] = cplxreal(z);
    auto [pc, pr] = cplxreal(p);

    const int nzc = static_cast<int>(zc.size());
    const int npc = static_cast<int>(pc.size());

    int nzr = static_cast<int>(zr.size());
    int npr = static_cast<int>(pr.size());

    // Pair up real zeroes.
    int nzrsec;
    std::vector<double> zrms, zrp;
    if (nzr > 0) {
        if (nzr % 2 == 1) {
            zr.push_back(0.0);
            nzr++;
        }
        nzrsec = nzr / 2;
        zrms.resize(nzrsec);
        zrp.resize(nzrsec);
        for (int i = 0; i < nzr - 1; i += 2) {
            zrms[i / 2] = -zr[i] - zr[i + 1];
            zrp[i / 2]  =  zr[i] * zr[i + 1];
        }
    }
    else {
        nzrsec = 0;
    }

    // Pair up real poles.
    int nprsec;
    std::vector<double> prms, prp;
    if (npr > 0) {
        if (npr % 2 == 1) {
            pr.push_back(0.0);
            npr++;
        }
        nprsec = npr / 2;
        prms.resize(nprsec);
        prp.resize(nprsec);
        for (int i = 0; i < npr - 1; i += 2) {
            prms[i / 2] = -pr[i] - pr[i + 1];
            prp[i / 2]  =  pr[i] * pr[i + 1];
        }
    }
    else {
        nprsec = 0;
    }

    const int nsecs = std::max(nzc + nzrsec, npc + nprsec);

    std::vector<double> zcm2r(nzc), zca2(nzc);
    for (int i = 0; i < nzc; ++i) {
        zcm2r[i] = -2.0 * zc[i].real();
        double a = std::abs(zc[i]);
        zca2[i] = a * a;
    }

    std::vector<double> pcm2r(npc), pca2(npc);
    for (int i = 0; i < npc; ++i) {
        pcm2r[i] = -2.0 * pc[i].real();
        double a = std::abs(pc[i]);
        pca2[i] = a * a;
    }

    std::vector<std::array<double, 6>> sos(nsecs);
    
    for (int i = 0; i < nsecs; ++i) {
        sos[i][0] = 1.0;
        sos[i][3] = 1.0;
    }

    const int nzrl = nzc + nzrsec;
    const int nprl = npc + nprsec;

    for (int i = 0; i < nsecs; ++i) {
        if (i < nzc) {
            sos[i][1] = zcm2r[i];
            sos[i][2] = zca2[i];
        }
        else if (i < nzrl) {
            sos[i][1] = zrms[i - nzc];
            sos[i][2] = zrp[i - nzc];
        }
        else {
            sos[i][1] = sos[i][2] = 0.0;
        }

        if (i < npc) {
            sos[i][4] = pcm2r[i];
            sos[i][5] = pca2[i];
        }
        else if (i < nprl) {
            sos[i][4] = prms[i - npc];
            sos[i][5] = prp[i - npc];
        }
        else {
            sos[i][4] = sos[i][5] = 0.0;
        }
    }

    if (sos.size() > 0) {
        for (int i = 0; i < 3; ++i) {
            sos[0][i] *= k;
        }
    }

    return sos;
}

std::vector<double> vc::model::sosfilter(const std::vector<std::array<double, 6>>& sos, const std::vector<double>& x)
{
    std::vector<double> y(x);
    std::vector<std::array<double, 2>> zf(sos.size(), std::array{0.0, 0.0});

    for (int k = 0; k < x.size(); ++k) {
        double x_cur = x[k];
        double x_new;
        for (int s = 0; s < sos.size(); ++s) {
            x_new    = sos[s][0] * x_cur + zf[s][0];
            zf[s][0] = sos[s][1] * x_cur - sos[s][4] * x_new + zf[s][1];
            zf[s][1] = sos[s][2] * x_cur - sos[s][5] * x_new;
            x_cur    = x_new;
        }
        y[k] = x_cur;
    }

    return y;
}

std::vector<std::array<double, 6>> vc::model::Butterworth::lowPass(int N, double fc, double fs)
{
    const double Wn = fc / (fs / 2.0);
    const double Wo = std::tan(Wn * PI / 2.0);

    std::vector<std::complex<double>> p;

    // Step 1. Get Butterworth analog lowpass prototype.
    for (int i = 2 + N - 1; i <= 3 * N - 1; i += 2) {
        p.push_back(std::polar<double>(1, (PI * i) / (2.0 * N)));
    }

    // Step 2. Transform to low pass filter.
    std::complex<double> Sg = 1.0,
                        prodSp = 1.0;

    std::vector<std::complex<double>> Sp(p.size()), Sz(0);

    for (int i = 0; i < p.size(); ++i) {
        Sg *= Wo;
        Sp[i] = Wo * p[i];
        prodSp *= (1.0 - Sp[i]);
    }

    // Step 3. Transform to digital filter.
    std::vector<std::complex<double>> P(Sp.size()), Z(Sp.size(), -1);
   
    double G = std::real(Sg / prodSp);

    for (int i = 0; i < Sp.size(); ++i) {
        P[i] = (1.0 + Sp[i]) / (1.0 - Sp[i]);
    }
    
    // Step 6. Convert to SOS.
    
    return zpk2sos(Z, P, G);
}

std::vector<std::array<double, 6>> vc::model::Butterworth::highPass(int N, double fc, double fs)
{
    const double Wn = fc / (fs / 2.0);
    const double Wo = std::tan(Wn * PI / 2.0);

    std::vector<std::complex<double>> p;

    // Step 1. Get Butterworth analog lowpass prototype.
    for (int i = 2 + N - 1; i <= 3 * N - 1; i += 2) {
        p.push_back(std::polar<double>(1, (PI * i) / (2.0 * N)));
    }

    // Step 2. Transform to high pass filter.
    std::complex<double> Sg = 1.0,
                        prodSp = 1.0,
                        prodSz = 1.0;

    std::vector<std::complex<double>> Sp(p.size()), Sz(p.size());

    for (int i = 0; i < p.size(); ++i) {
        Sg *= -p[i];
        Sp[i] = Wo / p[i];
        Sz[i] = 0.0;
        prodSp *= (1.0 - Sp[i]);
        prodSz *= (1.0 - Sz[i]);
    }
    Sg = 1.0 / Sg;

    // Step 3. Transform to digital filter.
    std::vector<std::complex<double>> P(Sp.size()), Z(Sp.size());
    
    double G = std::real(Sg * prodSz / prodSp);

    for (int i = 0; i < Sp.size(); ++i) {
        P[i] = (1.0 + Sp[i]) / (1.0 - Sp[i]);
        Z[i] = (1.0 + Sz[i]) / (1.0 - Sz[i]);
    }
    
    // Step 6. Convert to SOS.
    
    return zpk2sos(Z, P, G);
}
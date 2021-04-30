#ifndef FILTERDESIGN_H
#define FILTERDESIGN_H

#include <vector>
#include <array>
#include <complex>

#define PI 3.1415926535897932384626433832795028841971693993751058209749445923078164062

namespace vc::model {

std::vector<std::array<double, 6>> zpk2sos(
    const std::vector<std::complex<double>>& zeros,
    const std::vector<std::complex<double>>& poles,
    double k);

std::vector<double> sosfilter(const std::vector<std::array<double, 6>>& sos, const std::vector<double>& x);

class Butterworth {
public:
    static std::vector<std::array<double, 6>> lowPass(int N, double fc, double fs);
    static std::vector<std::array<double, 6>> highPass(int N, double fc, double fs);
};

}

#endif // FILTERDESIGN_H
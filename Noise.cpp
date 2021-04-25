#include "Noise.h"
#include <array>

std::random_device               vc::model::Noise::rd;
std::mt19937                     vc::model::Noise::gen(rd());
std::uniform_real_distribution<> vc::model::Noise::dis(-1.0, 1.0);

std::vector<double> vc::model::Noise::white(const int length)
{
    std::vector<double> out(length);
    for (int i = 0; i < length; ++i)
        out[i] = dis(gen);
    return out;
}

std::vector<double> vc::model::Noise::tilted(const int length, const double alpha)
{
    std::array<double, 64> filter;
    filter[0] = 1.0;
    for (int k = 1; k < 64; ++k) {
        filter[k] = (k - 1.0 - alpha / 2.0) * filter[k - 1] / double(k);
    }

    auto noise = white(length);
    std::vector<double> out(length);
    for (int i = 0; i < length; ++i) {
        out[i] = 0.0;
        for (int j = 0; j < filter.size(); ++j)
            if (i - j >= 0)
                out[i] += filter[j] * noise[i - j];
    }
    return out;
}
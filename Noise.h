#ifndef NOISE_H
#define NOISE_H

#include <vector>
#include <random>

namespace vc::model {

class Noise {
public:
    static std::vector<double> white(int length);
    static std::vector<double> tilted(int length, double alpha = -1.5);

private:
    static std::random_device rd;
    static std::mt19937 gen;
    static std::uniform_real_distribution<> dis;
};

}

#endif // NOISE_H
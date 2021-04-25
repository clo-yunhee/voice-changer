#ifndef LPC_H
#define LPC_H

#include <vector>

constexpr double PI = 3.1415926535897932384626433832795028841971693993751058209749445923078164062;

namespace vc::model {

class Lpc {
public:
    static std::vector<double> analyze(const std::vector<double>& data, double preemphFrequency, double sampleRate);

    static std::vector<double> applyFilter(const std::vector<double>& lpca, const std::vector<double>& data);

    static const std::vector<double>& window(int N);

private:
    static void makeWindow(int N);
    static std::vector<double> applyWindowAndPreemphasis(const std::vector<double>& data, double preemphFrequency, double sampleRate);
    static std::vector<double> calculatePredictionCoefficients(const std::vector<double>& data, int order);

    static std::vector<double> sWindow;
};

}

#endif // LPC_H
#ifndef ABERTHROOTS_H
#define ABERTHROOTS_H

#include <vector>
#include <complex>

#define PI 3.1415926535897932384626433832795028841971693993751058209749445923078164062

namespace vc::model {

std::vector<std::complex<double>> solveRoots(const std::vector<double>& P);

}

#endif // ABERTHROOTS_H
#pragma once
#include <cmath>
#include <algorithm>

// Coordinates and output in centimetres; time in seconds. The material uses
// these exact coefficients so physics samples the same moving surface.
namespace BenchySea {
struct Wave { double X, Y, Speed, Amplitude, Phase; };
inline constexpr Wave Waves[] = {
    {0.00073, 0.00031, 0.68, 18.0, 0.17},
    {-0.00041, 0.00117, -0.91, 13.0, 2.39},
    {0.00161, -0.00083, 1.13, 9.0, 4.71},
    {-0.00129, -0.00211, 1.42, 6.0, 1.13},
    {0.00317, 0.00091, -1.71, 4.0, 5.63},
    {-0.00243, 0.00419, 2.03, 3.0, 3.27},
    {0.00573, -0.00311, 2.31, 2.2, 0.79},
    {0.00191, 0.00737, -2.69, 1.5, 4.19},
    {-0.00821, -0.00543, 3.17, 1.1, 2.83},
    {0.01031, 0.00279, -3.53, 0.8, 5.17},
    {-0.00471, 0.01313, 3.91, 0.55, 1.91},
    {0.01493, -0.00917, 4.27, 0.4, 3.73}
};
inline double Height(double X, double Y, double T) {
    double H = 0.0;
    const double Warp=.55*std::sin(X*.00017+Y*.00023+T*.08)+.32*std::sin(-X*.00031+Y*.00011-T*.11);
    for (const auto& W : Waves) {
        double Envelope=.78+.22*std::sin(X*.00012-Y*.00009+T*.13+W.Phase);
        H+=std::sin(X*W.X+Y*W.Y+T*W.Speed+W.Phase+Warp)*W.Amplitude*Envelope;
    }
    return H;
}
inline void Gradient(double X, double Y, double T, double& DX, double& DY) {
    DX=DY=0;
    const double A=X*.00017+Y*.00023+T*.08,B=-X*.00031+Y*.00011-T*.11;
    const double Warp=.55*std::sin(A)+.32*std::sin(B);
    const double WX=.55*std::cos(A)*.00017-.32*std::cos(B)*.00031;
    const double WY=.55*std::cos(A)*.00023+.32*std::cos(B)*.00011;
    for (const auto& W : Waves) {
        double E=X*.00012-Y*.00009+T*.13+W.Phase;
        double Envelope=.78+.22*std::sin(E);
        double Phase=X*W.X+Y*W.Y+T*W.Speed+W.Phase+Warp;
        double C=std::cos(Phase)*W.Amplitude*Envelope;
        double S=std::sin(Phase)*W.Amplitude*.22*std::cos(E);
        DX+=C*(W.X+WX)+S*.00012;
        DY+=C*(W.Y+WY)-S*.00009;
    }
}
inline double BuoyancyFraction(double Bottom, double Surface, double Draft=34.0) {
    return std::clamp((Surface-Bottom)/Draft, 0.0, 3.0);
}
}

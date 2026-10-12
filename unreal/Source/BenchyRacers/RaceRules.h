#pragma once
#include <algorithm>
namespace BenchyRules {
constexpr int Checkpoints=16, Laps=3, MaxCoins=10;
inline float CoinSpeed(int coins) { return 1.f+.006f*std::clamp(coins,0,MaxCoins); }
inline int Lap(int passed) { return std::min(Laps,1+passed/Checkpoints); }
inline bool Finished(int passed) { return passed>=Checkpoints*Laps; }
inline int Next(int passed) { return (passed+1)%Checkpoints; }
inline int Last(int passed) { return passed%Checkpoints; }
}

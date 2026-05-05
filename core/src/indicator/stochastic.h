#pragma once

#include <vector>
#include <optional>

namespace tce {
class Series;

struct StochasticResult {
    std::vector<std::optional<double>> k;  // %K (smoothed if smooth>1)
    std::vector<std::optional<double>> d;  // %D = SMA(k, dPeriod)
};

// 일반: kPeriod=14, dPeriod=3, smooth=3 (slow stochastic)
// raw %K = 100 * (close - lowestLow) / (highestHigh - lowestLow)
// smooth = SMA(rawK, smooth)
StochasticResult stochastic(const Series& series, int kPeriod, int dPeriod, int smooth);

} // namespace tce

#pragma once
#include <vector>
#include <optional>

namespace tce {
class Series;

struct SuperTrendResult {
    std::vector<std::optional<double>> line;       // 상승/하락에 따른 SAR-like 라인
    std::vector<int>                   direction;  // 1=up, -1=down (size = candles)
};

// SuperTrend(period=10, multiplier=3.0)
SuperTrendResult superTrend(const Series& series, int period = 10, double multiplier = 3.0);

} // namespace tce

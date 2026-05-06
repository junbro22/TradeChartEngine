#pragma once

#include <vector>
#include <optional>

namespace tce {
class Series;

struct KeltnerResult {
    std::vector<std::optional<double>> middle; // EMA(emaPeriod)
    std::vector<std::optional<double>> upper;  // EMA + multiplier * ATR(atrPeriod)
    std::vector<std::optional<double>> lower;  // EMA - multiplier * ATR(atrPeriod)
};

// Keltner Channels. 표준값 emaPeriod=20, atrPeriod=10, multiplier=2.0.
// 첫 max(emaPeriod-1, atrPeriod)개 인덱스는 nullopt.
KeltnerResult keltner(const Series& series, int emaPeriod, int atrPeriod, double multiplier);

} // namespace tce

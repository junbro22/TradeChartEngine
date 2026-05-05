#pragma once

#include <vector>
#include <optional>

namespace tce {
class Series;

struct BollingerResult {
    std::vector<std::optional<double>> middle; // SMA
    std::vector<std::optional<double>> upper;  // SMA + stddev * sigma
    std::vector<std::optional<double>> lower;  // SMA - stddev * sigma
};

// Bollinger Bands. 일반적으로 period=20, stddev=2.0
BollingerResult bollinger(const Series& series, int period, double stddev);

} // namespace tce

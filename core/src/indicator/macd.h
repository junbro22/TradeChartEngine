#pragma once

#include <vector>
#include <optional>

namespace tce {
class Series;

struct MacdResult {
    std::vector<std::optional<double>> line;       // EMA(fast) - EMA(slow)
    std::vector<std::optional<double>> signal;     // EMA(line, signalPeriod)
    std::vector<std::optional<double>> histogram;  // line - signal
};

// MACD(fast=12, slow=26, signal=9)
MacdResult macd(const Series& series, int fast, int slow, int signalPeriod);

} // namespace tce

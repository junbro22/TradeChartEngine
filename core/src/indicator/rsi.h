#pragma once

#include <vector>
#include <optional>

namespace tce {
class Series;

// Wilder RSI(period). 0..100 범위. 첫 period개는 nullopt.
std::vector<std::optional<double>> rsi(const Series& series, int period);

} // namespace tce

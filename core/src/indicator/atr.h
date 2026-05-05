#pragma once

#include <vector>
#include <optional>

namespace tce {
class Series;

// Wilder ATR(period). 첫 period개는 nullopt.
std::vector<std::optional<double>> atr(const Series& series, int period);

} // namespace tce

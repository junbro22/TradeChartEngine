#pragma once

#include <vector>
#include <optional>

namespace tce {

class Series;

// Exponential Moving Average. period 미만 인덱스는 nullopt.
// alpha = 2 / (period + 1). 첫 유효 값은 SMA로 seed.
std::vector<std::optional<double>> ema(const Series& series, int period);

} // namespace tce

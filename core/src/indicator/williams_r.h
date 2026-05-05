#pragma once
#include <vector>
#include <optional>

namespace tce {
class Series;

// Williams %R(period=14). 범위 -100..0
std::vector<std::optional<double>> williamsR(const Series& series, int period = 14);

} // namespace tce

#pragma once
#include <vector>
#include <optional>

namespace tce {
class Series;

// Money Flow Index(period=14). 0..100
std::vector<std::optional<double>> mfi(const Series& series, int period = 14);

} // namespace tce

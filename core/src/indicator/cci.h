#pragma once
#include <vector>
#include <optional>

namespace tce {
class Series;

// CCI(period=20). typical price = (H+L+C)/3
std::vector<std::optional<double>> cci(const Series& series, int period = 20);

} // namespace tce

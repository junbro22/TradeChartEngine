#pragma once
#include <vector>
#include <optional>

namespace tce {
class Series;

// On-Balance Volume — 종가 비교 누적
std::vector<std::optional<double>> obv(const Series& series);

} // namespace tce

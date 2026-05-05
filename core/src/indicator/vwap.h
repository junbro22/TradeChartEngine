#pragma once
#include <vector>
#include <optional>

namespace tce {
class Series;

// Session VWAP — 일자(자정 기준)가 바뀌면 재시작.
// price = typical price = (H+L+C)/3
std::vector<std::optional<double>> vwap(const Series& series);

} // namespace tce

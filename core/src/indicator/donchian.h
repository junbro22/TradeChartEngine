#pragma once

#include <vector>
#include <optional>

namespace tce {
class Series;

struct DonchianResult {
    std::vector<std::optional<double>> upper;  // 최근 period 캔들 high.max
    std::vector<std::optional<double>> lower;  // 최근 period 캔들 low.min
    std::vector<std::optional<double>> middle; // (upper + lower) / 2
};

// Donchian Channels. 일반적으로 period=20.
// 첫 (period-1)개 인덱스는 nullopt.
DonchianResult donchian(const Series& series, int period);

} // namespace tce

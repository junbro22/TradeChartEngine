#pragma once

#include <vector>
#include <optional>

namespace tce {
class Series;

// Hull Moving Average — 라그 적은 MA. 표준 period=20.
// 산식: WMA(2*WMA(close, period/2) - WMA(close, period), sqrt(period))
// 첫 (period - 1)개 인덱스는 nullopt.
std::vector<std::optional<double>> hma(const Series& series, int period);

} // namespace tce

#pragma once
#include <vector>
#include <optional>

namespace tce {
class Series;

// Parabolic SAR. step=0.02, max=0.2 일반.
// 상승 트렌드에서는 SAR이 가격 아래, 하락에서는 위.
std::vector<std::optional<double>> psar(const Series& series, double step = 0.02, double maxStep = 0.2);

} // namespace tce

#pragma once

#include <vector>
#include <optional>

namespace tce {
class Series;

// 일목균형표(一目均衡表) — 5개 라인 + 구름 영역
// 일반: tenkan=9, kijun=26, senkouB=52, displacement=26
struct IchimokuResult {
    std::vector<std::optional<double>> tenkan;       // 전환선  (9 H+L)/2
    std::vector<std::optional<double>> kijun;        // 기준선  (26 H+L)/2
    std::vector<std::optional<double>> senkouA;      // 선행스팬1 = (전환+기준)/2, +displacement 앞으로
    std::vector<std::optional<double>> senkouB;      // 선행스팬2 = (52 H+L)/2,    +displacement 앞으로
    std::vector<std::optional<double>> chikou;       // 후행스팬 = close, -displacement 뒤로
    int displacement = 26;
};

IchimokuResult ichimoku(const Series& series,
                        int tenkanPeriod = 9,
                        int kijunPeriod = 26,
                        int senkouBPeriod = 52,
                        int displacement = 26);

} // namespace tce

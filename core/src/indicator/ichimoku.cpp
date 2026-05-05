#include "indicator/ichimoku.h"
#include "data/series.h"
#include <algorithm>
#include <limits>

namespace tce {

namespace {

// (highestHigh + lowestLow) / 2 over 'period' looking back
std::optional<double> donchianMid(const std::vector<TceCandle>& cs, size_t i, int period) {
    if ((int)i + 1 < period) return std::nullopt;
    double hi = -std::numeric_limits<double>::infinity();
    double lo =  std::numeric_limits<double>::infinity();
    for (int k = 0; k < period; ++k) {
        hi = std::max(hi, cs[i - k].high);
        lo = std::min(lo, cs[i - k].low);
    }
    return (hi + lo) / 2.0;
}

} // namespace

IchimokuResult ichimoku(const Series& series,
                        int tenkanPeriod, int kijunPeriod,
                        int senkouBPeriod, int displacement) {
    const auto& cs = series.candles();
    IchimokuResult r;
    r.displacement = displacement;
    const size_t N = cs.size();

    r.tenkan.assign(N, std::nullopt);
    r.kijun.assign(N, std::nullopt);
    r.chikou.assign(N, std::nullopt);
    // senkouA/B는 displacement 앞으로 — 시계열 길이를 N + displacement로 잡는 게 일반적
    r.senkouA.assign(N + displacement, std::nullopt);
    r.senkouB.assign(N + displacement, std::nullopt);

    for (size_t i = 0; i < N; ++i) {
        auto t = donchianMid(cs, i, tenkanPeriod);
        auto k = donchianMid(cs, i, kijunPeriod);
        auto b = donchianMid(cs, i, senkouBPeriod);

        r.tenkan[i] = t;
        r.kijun[i]  = k;

        // 선행스팬1 = (전환+기준)/2, 미래 displacement 위치에 표시
        if (t && k) {
            size_t fi = i + displacement;
            if (fi < r.senkouA.size()) r.senkouA[fi] = (*t + *k) / 2.0;
        }
        if (b) {
            size_t fi = i + displacement;
            if (fi < r.senkouB.size()) r.senkouB[fi] = *b;
        }

        // 후행스팬 = 종가, displacement만큼 과거에 표시
        if ((int)i - displacement >= 0) {
            r.chikou[i - displacement] = cs[i].close;
        }
    }
    return r;
}

} // namespace tce

#include "indicator/vwap.h"
#include "data/series.h"
#include <cmath>

namespace tce {

namespace {
// UTC + offset 기반 day key — 빌드 머신 timezone 의존성 없음.
int64_t dayKey(double ts, double offset) {
    return static_cast<int64_t>(std::floor((ts + offset) / 86400.0));
}
} // namespace

std::vector<std::optional<double>> vwap(const Series& series, double sessionOffsetSeconds) {
    const auto& cs = series.candles();
    std::vector<std::optional<double>> out(cs.size());
    if (cs.empty()) return out;

    int64_t currentDay = dayKey(cs[0].timestamp, sessionOffsetSeconds);
    double cumPV = 0.0, cumV = 0.0;
    for (size_t i = 0; i < cs.size(); ++i) {
        int64_t d = dayKey(cs[i].timestamp, sessionOffsetSeconds);
        if (d != currentDay) { cumPV = 0; cumV = 0; currentDay = d; }
        double tp = (cs[i].high + cs[i].low + cs[i].close) / 3.0;
        cumPV += tp * cs[i].volume;
        cumV  += cs[i].volume;
        if (cumV > 0) out[i] = cumPV / cumV;
    }
    return out;
}

} // namespace tce

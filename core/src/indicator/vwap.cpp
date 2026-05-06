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

VWAPBandsResult vwapBands(const Series& series, double sessionOffsetSeconds, double numStdev) {
    const auto& cs = series.candles();
    VWAPBandsResult r;
    r.middle.assign(cs.size(), std::nullopt);
    r.upper.assign(cs.size(), std::nullopt);
    r.lower.assign(cs.size(), std::nullopt);
    if (cs.empty()) return r;

    // weighted Welford — anchored VWAP의 표준 가중분산 산식.
    //   delta = tp - mean_prev
    //   mean  = mean_prev + (w / cumV) * delta
    //   M2   += w * delta * (tp - mean_new)
    //   sigma = sqrt(M2 / cumV)
    int64_t currentDay = dayKey(cs[0].timestamp, sessionOffsetSeconds);
    double cumV = 0.0, mean = 0.0, M2 = 0.0;
    for (size_t i = 0; i < cs.size(); ++i) {
        int64_t d = dayKey(cs[i].timestamp, sessionOffsetSeconds);
        if (d != currentDay) {
            cumV = 0; mean = 0; M2 = 0;
            currentDay = d;
        }
        double tp = (cs[i].high + cs[i].low + cs[i].close) / 3.0;
        double w  = cs[i].volume;
        if (w <= 0) {
            if (cumV > 0) {
                r.middle[i] = mean;
                if (numStdev > 0) {
                    double sigma = std::sqrt(M2 / cumV);
                    r.upper[i] = mean + numStdev * sigma;
                    r.lower[i] = mean - numStdev * sigma;
                }
            }
            continue;
        }
        cumV += w;
        double delta = tp - mean;
        mean += (w / cumV) * delta;
        M2   += w * delta * (tp - mean);
        double sigma = (cumV > 0) ? std::sqrt(M2 / cumV) : 0.0;
        r.middle[i] = mean;
        if (numStdev > 0) {
            r.upper[i] = mean + numStdev * sigma;
            r.lower[i] = mean - numStdev * sigma;
        }
    }
    return r;
}

} // namespace tce

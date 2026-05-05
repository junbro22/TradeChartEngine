#include "indicator/vwap.h"
#include "data/series.h"
#include <ctime>

namespace tce {

namespace {
int dayKey(double ts) {
    std::time_t t = static_cast<std::time_t>(ts);
    std::tm tm_v{};
    localtime_r(&t, &tm_v);
    return (tm_v.tm_year + 1900) * 10000 + (tm_v.tm_mon + 1) * 100 + tm_v.tm_mday;
}
} // namespace

std::vector<std::optional<double>> vwap(const Series& series) {
    const auto& cs = series.candles();
    std::vector<std::optional<double>> out(cs.size());
    if (cs.empty()) return out;

    int currentDay = dayKey(cs[0].timestamp);
    double cumPV = 0.0, cumV = 0.0;
    for (size_t i = 0; i < cs.size(); ++i) {
        int d = dayKey(cs[i].timestamp);
        if (d != currentDay) { cumPV = 0; cumV = 0; currentDay = d; }
        double tp = (cs[i].high + cs[i].low + cs[i].close) / 3.0;
        cumPV += tp * cs[i].volume;
        cumV  += cs[i].volume;
        if (cumV > 0) out[i] = cumPV / cumV;
    }
    return out;
}

} // namespace tce

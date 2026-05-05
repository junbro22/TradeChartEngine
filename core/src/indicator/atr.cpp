#include "indicator/atr.h"
#include "data/series.h"
#include <algorithm>
#include <cmath>

namespace tce {

std::vector<std::optional<double>> atr(const Series& series, int period) {
    const auto& cs = series.candles();
    std::vector<std::optional<double>> out(cs.size());
    if (period <= 0 || (int)cs.size() <= period) return out;

    // True Range
    std::vector<double> tr(cs.size(), 0.0);
    tr[0] = cs[0].high - cs[0].low;
    for (size_t i = 1; i < cs.size(); ++i) {
        double a = cs[i].high - cs[i].low;
        double b = std::fabs(cs[i].high - cs[i - 1].close);
        double c = std::fabs(cs[i].low  - cs[i - 1].close);
        tr[i] = std::max({a, b, c});
    }

    // Wilder's smoothing — seed = SMA(TR[1..period])
    double sum = 0.0;
    for (int i = 1; i <= period; ++i) sum += tr[i];
    double prev = sum / period;
    out[period] = prev;

    for (size_t i = (size_t)period + 1; i < cs.size(); ++i) {
        prev = (prev * (period - 1) + tr[i]) / period;
        out[i] = prev;
    }
    return out;
}

} // namespace tce

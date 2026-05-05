#include "indicator/stochastic.h"
#include "data/series.h"
#include <algorithm>
#include <limits>

namespace tce {

namespace {
std::vector<std::optional<double>> smaOptional(const std::vector<std::optional<double>>& src,
                                                int period) {
    std::vector<std::optional<double>> out(src.size());
    if (period <= 0) return out;
    int collected = 0;
    double sum = 0.0;
    int windowStart = 0;
    for (size_t i = 0; i < src.size(); ++i) {
        if (!src[i]) {
            collected = 0; sum = 0; windowStart = (int)i + 1;
            continue;
        }
        sum += *src[i];
        ++collected;
        if (collected > period) {
            sum -= *src[windowStart];
            ++windowStart;
            --collected;
        }
        if (collected == period) out[i] = sum / period;
    }
    return out;
}
} // namespace

StochasticResult stochastic(const Series& series, int kPeriod, int dPeriod, int smooth) {
    const auto& cs = series.candles();
    StochasticResult r;
    r.k.assign(cs.size(), std::nullopt);
    r.d.assign(cs.size(), std::nullopt);
    if (kPeriod <= 0 || (int)cs.size() < kPeriod) return r;

    // raw %K
    std::vector<std::optional<double>> rawK(cs.size());
    for (size_t i = (size_t)kPeriod - 1; i < cs.size(); ++i) {
        double hi = -std::numeric_limits<double>::infinity();
        double lo =  std::numeric_limits<double>::infinity();
        for (int k = 0; k < kPeriod; ++k) {
            hi = std::max(hi, cs[i - k].high);
            lo = std::min(lo, cs[i - k].low);
        }
        if (hi == lo) { rawK[i] = 50.0; continue; }
        rawK[i] = 100.0 * (cs[i].close - lo) / (hi - lo);
    }

    r.k = (smooth > 1) ? smaOptional(rawK, smooth) : rawK;
    r.d = smaOptional(r.k, dPeriod);
    return r;
}

} // namespace tce

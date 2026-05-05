#include "indicator/ema.h"
#include "data/series.h"

namespace tce {

std::vector<std::optional<double>> ema(const Series& series, int period) {
    const auto& cs = series.candles();
    std::vector<std::optional<double>> out(cs.size());
    if (period <= 0 || cs.empty() || (int)cs.size() < period) return out;

    const double alpha = 2.0 / (period + 1);

    // Seed: 첫 period개의 SMA
    double seed = 0.0;
    for (int i = 0; i < period; ++i) seed += cs[i].close;
    seed /= period;
    out[period - 1] = seed;

    double prev = seed;
    for (size_t i = period; i < cs.size(); ++i) {
        prev = alpha * cs[i].close + (1.0 - alpha) * prev;
        out[i] = prev;
    }
    return out;
}

} // namespace tce

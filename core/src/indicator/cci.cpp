#include "indicator/cci.h"
#include "data/series.h"
#include <cmath>

namespace tce {

std::vector<std::optional<double>> cci(const Series& series, int period) {
    const auto& cs = series.candles();
    std::vector<std::optional<double>> out(cs.size());
    if (period <= 1 || (int)cs.size() < period) return out;

    std::vector<double> tp(cs.size());
    for (size_t i = 0; i < cs.size(); ++i) {
        tp[i] = (cs[i].high + cs[i].low + cs[i].close) / 3.0;
    }

    for (size_t i = (size_t)period - 1; i < cs.size(); ++i) {
        double sum = 0;
        for (int k = 0; k < period; ++k) sum += tp[i - k];
        double ma = sum / period;
        double mad = 0;
        for (int k = 0; k < period; ++k) mad += std::fabs(tp[i - k] - ma);
        mad /= period;
        if (mad > 0) out[i] = (tp[i] - ma) / (0.015 * mad);
    }
    return out;
}

} // namespace tce

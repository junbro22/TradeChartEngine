#include "indicator/bollinger.h"
#include "data/series.h"
#include <cmath>

namespace tce {

BollingerResult bollinger(const Series& series, int period, double stddev) {
    const auto& cs = series.candles();
    BollingerResult r;
    r.middle.assign(cs.size(), std::nullopt);
    r.upper.assign(cs.size(), std::nullopt);
    r.lower.assign(cs.size(), std::nullopt);
    if (period <= 1 || (int)cs.size() < period) return r;

    for (size_t i = period - 1; i < cs.size(); ++i) {
        double sum = 0.0;
        for (int k = 0; k < period; ++k) sum += cs[i - k].close;
        double mean = sum / period;

        double sqsum = 0.0;
        for (int k = 0; k < period; ++k) {
            double d = cs[i - k].close - mean;
            sqsum += d * d;
        }
        double sigma = std::sqrt(sqsum / period);

        r.middle[i] = mean;
        r.upper[i]  = mean + stddev * sigma;
        r.lower[i]  = mean - stddev * sigma;
    }
    return r;
}

} // namespace tce

#include "indicator/rsi.h"
#include "data/series.h"
#include <cmath>

namespace tce {

std::vector<std::optional<double>> rsi(const Series& series, int period) {
    const auto& cs = series.candles();
    std::vector<std::optional<double>> out(cs.size());
    if (period <= 0 || (int)cs.size() <= period) return out;

    double avgGain = 0.0;
    double avgLoss = 0.0;
    for (int i = 1; i <= period; ++i) {
        double change = cs[i].close - cs[i - 1].close;
        if (change >= 0) avgGain += change;
        else             avgLoss += -change;
    }
    avgGain /= period;
    avgLoss /= period;

    auto computeRSI = [](double g, double l) -> double {
        if (l == 0.0) return 100.0;
        double rs = g / l;
        return 100.0 - (100.0 / (1.0 + rs));
    };

    out[period] = computeRSI(avgGain, avgLoss);

    for (size_t i = period + 1; i < cs.size(); ++i) {
        double change = cs[i].close - cs[i - 1].close;
        double gain = change > 0 ? change : 0.0;
        double loss = change < 0 ? -change : 0.0;
        avgGain = (avgGain * (period - 1) + gain) / period;
        avgLoss = (avgLoss * (period - 1) + loss) / period;
        out[i] = computeRSI(avgGain, avgLoss);
    }
    return out;
}

} // namespace tce

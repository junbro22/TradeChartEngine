#include "indicator/ma.h"
#include "data/series.h"

namespace tce {

std::vector<std::optional<double>> sma(const Series& series, int period) {
    const auto& cs = series.candles();
    std::vector<std::optional<double>> out(cs.size());
    if (period <= 0 || cs.empty()) return out;

    double window = 0.0;
    for (size_t i = 0; i < cs.size(); ++i) {
        window += cs[i].close;
        if ((int)i + 1 > period) {
            window -= cs[i - period].close;
        }
        if ((int)i + 1 >= period) {
            out[i] = window / period;
        }
    }
    return out;
}

} // namespace tce

#include "indicator/obv.h"
#include "data/series.h"

namespace tce {

std::vector<std::optional<double>> obv(const Series& series) {
    const auto& cs = series.candles();
    std::vector<std::optional<double>> out(cs.size());
    if (cs.empty()) return out;
    double v = 0.0;
    out[0] = 0.0;
    for (size_t i = 1; i < cs.size(); ++i) {
        if      (cs[i].close > cs[i-1].close) v += cs[i].volume;
        else if (cs[i].close < cs[i-1].close) v -= cs[i].volume;
        out[i] = v;
    }
    return out;
}

} // namespace tce

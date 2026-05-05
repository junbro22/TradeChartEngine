#include "indicator/williams_r.h"
#include "data/series.h"
#include <algorithm>
#include <limits>

namespace tce {

std::vector<std::optional<double>> williamsR(const Series& series, int period) {
    const auto& cs = series.candles();
    std::vector<std::optional<double>> out(cs.size());
    if (period <= 0 || (int)cs.size() < period) return out;

    for (size_t i = (size_t)period - 1; i < cs.size(); ++i) {
        double hi = -std::numeric_limits<double>::infinity();
        double lo =  std::numeric_limits<double>::infinity();
        for (int k = 0; k < period; ++k) {
            hi = std::max(hi, cs[i - k].high);
            lo = std::min(lo, cs[i - k].low);
        }
        if (hi != lo) out[i] = -100.0 * (hi - cs[i].close) / (hi - lo);
    }
    return out;
}

} // namespace tce

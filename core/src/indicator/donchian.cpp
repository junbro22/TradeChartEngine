#include "indicator/donchian.h"
#include "data/series.h"
#include <algorithm>

namespace tce {

DonchianResult donchian(const Series& series, int period) {
    const auto& cs = series.candles();
    DonchianResult r;
    r.upper.assign(cs.size(), std::nullopt);
    r.lower.assign(cs.size(), std::nullopt);
    r.middle.assign(cs.size(), std::nullopt);
    if (period <= 1 || static_cast<int>(cs.size()) < period) return r;

    for (size_t i = static_cast<size_t>(period - 1); i < cs.size(); ++i) {
        double hi = cs[i].high;
        double lo = cs[i].low;
        for (int k = 1; k < period; ++k) {
            const auto& c = cs[i - static_cast<size_t>(k)];
            if (c.high > hi) hi = c.high;
            if (c.low  < lo) lo = c.low;
        }
        r.upper[i]  = hi;
        r.lower[i]  = lo;
        r.middle[i] = (hi + lo) * 0.5;
    }
    return r;
}

} // namespace tce

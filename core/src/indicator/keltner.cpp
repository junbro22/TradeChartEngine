#include "indicator/keltner.h"
#include "indicator/ema.h"
#include "indicator/atr.h"
#include "data/series.h"

namespace tce {

KeltnerResult keltner(const Series& series, int emaPeriod, int atrPeriod, double multiplier) {
    const auto& cs = series.candles();
    KeltnerResult r;
    r.middle.assign(cs.size(), std::nullopt);
    r.upper.assign(cs.size(), std::nullopt);
    r.lower.assign(cs.size(), std::nullopt);
    if (emaPeriod <= 1 || atrPeriod <= 0 || cs.empty()) return r;

    auto e = ema(series, emaPeriod);
    auto a = atr(series, atrPeriod);
    const size_t n = std::min({e.size(), a.size(), cs.size()});
    for (size_t i = 0; i < n; ++i) {
        if (!e[i] || !a[i]) continue;
        r.middle[i] = *e[i];
        r.upper[i]  = *e[i] + multiplier * (*a[i]);
        r.lower[i]  = *e[i] - multiplier * (*a[i]);
    }
    return r;
}

} // namespace tce

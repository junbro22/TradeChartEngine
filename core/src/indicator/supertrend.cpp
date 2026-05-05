#include "indicator/supertrend.h"
#include "indicator/atr.h"
#include "data/series.h"

namespace tce {

SuperTrendResult superTrend(const Series& series, int period, double multiplier) {
    const auto& cs = series.candles();
    SuperTrendResult r;
    r.line.assign(cs.size(), std::nullopt);
    r.direction.assign(cs.size(), 1);
    if (cs.empty()) return r;

    auto a = atr(series, period);

    double finalUpper = 0, finalLower = 0;
    int dir = 1;

    for (size_t i = 0; i < cs.size(); ++i) {
        if (!a[i]) continue;
        double hl2 = (cs[i].high + cs[i].low) / 2.0;
        double basicUpper = hl2 + multiplier * (*a[i]);
        double basicLower = hl2 - multiplier * (*a[i]);

        if (i == 0 || !a[i-1]) {
            finalUpper = basicUpper;
            finalLower = basicLower;
            dir = (cs[i].close >= hl2) ? 1 : -1;
        } else {
            // final upper/lower bands (carry-forward)
            if (basicUpper < finalUpper || cs[i-1].close > finalUpper) finalUpper = basicUpper;
            if (basicLower > finalLower || cs[i-1].close < finalLower) finalLower = basicLower;

            // 방향 결정
            if (dir == 1 && cs[i].close < finalLower)      dir = -1;
            else if (dir == -1 && cs[i].close > finalUpper) dir = 1;
        }

        r.direction[i] = dir;
        r.line[i]      = (dir == 1) ? finalLower : finalUpper;
    }
    return r;
}

} // namespace tce

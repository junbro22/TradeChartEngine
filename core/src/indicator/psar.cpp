#include "indicator/psar.h"
#include "data/series.h"
#include <algorithm>

namespace tce {

std::vector<std::optional<double>> psar(const Series& series, double step, double maxStep) {
    const auto& cs = series.candles();
    std::vector<std::optional<double>> out(cs.size());
    if (cs.size() < 2) return out;

    bool uptrend = cs[1].close >= cs[0].close;
    double ep   = uptrend ? cs[0].high : cs[0].low;     // extreme point
    double sar  = uptrend ? cs[0].low  : cs[0].high;
    double af   = step;
    out[0] = sar;

    for (size_t i = 1; i < cs.size(); ++i) {
        double newSar = sar + af * (ep - sar);

        if (uptrend) {
            // SAR은 직전 2 봉의 low보다 위로 갈 수 없음
            if (i >= 2) newSar = std::min({newSar, cs[i-1].low, cs[i-2].low});
            else        newSar = std::min(newSar, cs[i-1].low);
            if (cs[i].low < newSar) {
                // reverse to downtrend
                uptrend = false;
                newSar = ep;
                ep = cs[i].low;
                af = step;
            } else {
                if (cs[i].high > ep) { ep = cs[i].high; af = std::min(af + step, maxStep); }
            }
        } else {
            if (i >= 2) newSar = std::max({newSar, cs[i-1].high, cs[i-2].high});
            else        newSar = std::max(newSar, cs[i-1].high);
            if (cs[i].high > newSar) {
                uptrend = true;
                newSar = ep;
                ep = cs[i].high;
                af = step;
            } else {
                if (cs[i].low < ep) { ep = cs[i].low; af = std::min(af + step, maxStep); }
            }
        }
        sar = newSar;
        out[i] = sar;
    }
    return out;
}

} // namespace tce

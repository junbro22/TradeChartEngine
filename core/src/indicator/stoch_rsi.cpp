#include "indicator/stoch_rsi.h"
#include "indicator/rsi.h"
#include "data/series.h"
#include <limits>

namespace tce {

namespace {

// optional 시리즈에 SMA 적용 — null은 건너뛰지 않고 윈도우의 모든 값이 valid한 경우만 emit.
std::vector<std::optional<double>> smaOpt(const std::vector<std::optional<double>>& src,
                                            int period) {
    std::vector<std::optional<double>> out(src.size());
    if (period <= 0) return out;
    for (size_t i = 0; i < src.size(); ++i) {
        if (i + 1 < static_cast<size_t>(period)) continue;
        double sum = 0;
        bool ok = true;
        for (int k = 0; k < period; ++k) {
            const auto& v = src[i - static_cast<size_t>(k)];
            if (!v) { ok = false; break; }
            sum += *v;
        }
        if (ok) out[i] = sum / period;
    }
    return out;
}

} // namespace

StochasticResult stochasticRSI(const Series& series,
                                int rsiPeriod, int kPeriod, int dPeriod, int smooth) {
    StochasticResult r;
    const auto& cs = series.candles();
    r.k.assign(cs.size(), std::nullopt);
    r.d.assign(cs.size(), std::nullopt);
    if (rsiPeriod <= 1 || kPeriod <= 0 || cs.empty()) return r;

    auto rsiSeries = rsi(series, rsiPeriod);

    // raw stochRSI = 100 * (rsi - rolling_min) / (rolling_max - rolling_min) over kPeriod
    std::vector<std::optional<double>> raw(cs.size());
    for (size_t i = static_cast<size_t>(kPeriod - 1); i < cs.size(); ++i) {
        double mn =  std::numeric_limits<double>::infinity();
        double mx = -std::numeric_limits<double>::infinity();
        bool ok = true;
        for (int k = 0; k < kPeriod; ++k) {
            const auto& v = rsiSeries[i - static_cast<size_t>(k)];
            if (!v) { ok = false; break; }
            if (*v < mn) mn = *v;
            if (*v > mx) mx = *v;
        }
        if (!ok) continue;
        if (mx == mn) raw[i] = 0.0;
        else          raw[i] = 100.0 * (*rsiSeries[i] - mn) / (mx - mn);
    }

    // %K = SMA(raw, smooth) — smooth<=1이면 raw 그대로
    if (smooth > 1) r.k = smaOpt(raw, smooth);
    else            r.k = raw;
    // %D = SMA(%K, dPeriod)
    if (dPeriod > 1) r.d = smaOpt(r.k, dPeriod);
    else             r.d = r.k;
    return r;
}

} // namespace tce

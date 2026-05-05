#include "indicator/dmi.h"
#include "data/series.h"
#include <algorithm>
#include <cmath>

namespace tce {

DmiResult dmi(const Series& series, int period) {
    const auto& cs = series.candles();
    DmiResult r;
    r.plusDI.assign(cs.size(), std::nullopt);
    r.minusDI.assign(cs.size(), std::nullopt);
    r.adx.assign(cs.size(), std::nullopt);
    if ((int)cs.size() <= period * 2) return r;

    // True Range, +DM, -DM
    std::vector<double> tr(cs.size(), 0.0), pdm(cs.size(), 0.0), mdm(cs.size(), 0.0);
    for (size_t i = 1; i < cs.size(); ++i) {
        double upMove   = cs[i].high - cs[i-1].high;
        double downMove = cs[i-1].low - cs[i].low;
        pdm[i] = (upMove > downMove && upMove > 0) ? upMove : 0;
        mdm[i] = (downMove > upMove && downMove > 0) ? downMove : 0;
        double a = cs[i].high - cs[i].low;
        double b = std::fabs(cs[i].high - cs[i-1].close);
        double c = std::fabs(cs[i].low  - cs[i-1].close);
        tr[i] = std::max({a, b, c});
    }

    // Wilder smoothed sums (initial = sum of first 'period')
    double sTR = 0, sPDM = 0, sMDM = 0;
    for (int i = 1; i <= period; ++i) {
        sTR += tr[i]; sPDM += pdm[i]; sMDM += mdm[i];
    }

    auto computeDI = [](double sm, double str) {
        return (str > 0) ? 100.0 * sm / str : 0.0;
    };

    std::vector<std::optional<double>> dx(cs.size(), std::nullopt);
    r.plusDI[period]  = computeDI(sPDM, sTR);
    r.minusDI[period] = computeDI(sMDM, sTR);
    {
        double pdi = *r.plusDI[period], mdi = *r.minusDI[period];
        if (pdi + mdi > 0) dx[period] = 100.0 * std::fabs(pdi - mdi) / (pdi + mdi);
    }

    for (size_t i = (size_t)period + 1; i < cs.size(); ++i) {
        sTR  = sTR  - sTR  / period + tr[i];
        sPDM = sPDM - sPDM / period + pdm[i];
        sMDM = sMDM - sMDM / period + mdm[i];
        double pdi = computeDI(sPDM, sTR);
        double mdi = computeDI(sMDM, sTR);
        r.plusDI[i]  = pdi;
        r.minusDI[i] = mdi;
        if (pdi + mdi > 0) dx[i] = 100.0 * std::fabs(pdi - mdi) / (pdi + mdi);
    }

    // ADX = Wilder MA of DX
    int adxStart = period * 2;
    if ((int)cs.size() > adxStart) {
        double sumDX = 0;
        for (int i = period + 1; i <= adxStart; ++i) if (dx[i]) sumDX += *dx[i];
        double adxPrev = sumDX / period;
        r.adx[adxStart] = adxPrev;
        for (size_t i = (size_t)adxStart + 1; i < cs.size(); ++i) {
            if (!dx[i]) continue;
            adxPrev = (adxPrev * (period - 1) + *dx[i]) / period;
            r.adx[i] = adxPrev;
        }
    }
    return r;
}

} // namespace tce

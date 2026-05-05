#include "indicator/mfi.h"
#include "data/series.h"

namespace tce {

std::vector<std::optional<double>> mfi(const Series& series, int period) {
    const auto& cs = series.candles();
    std::vector<std::optional<double>> out(cs.size());
    if (period <= 0 || (int)cs.size() <= period) return out;

    std::vector<double> tp(cs.size()), pos(cs.size(), 0), neg(cs.size(), 0);
    for (size_t i = 0; i < cs.size(); ++i) {
        tp[i] = (cs[i].high + cs[i].low + cs[i].close) / 3.0;
    }
    for (size_t i = 1; i < cs.size(); ++i) {
        double mf = tp[i] * cs[i].volume;
        if (tp[i] > tp[i-1])      pos[i] = mf;
        else if (tp[i] < tp[i-1]) neg[i] = mf;
    }

    for (size_t i = (size_t)period; i < cs.size(); ++i) {
        double sumPos = 0, sumNeg = 0;
        for (int k = 0; k < period; ++k) {
            sumPos += pos[i - k];
            sumNeg += neg[i - k];
        }
        if (sumNeg == 0)      out[i] = 100.0;
        else {
            double mr = sumPos / sumNeg;
            out[i] = 100.0 - (100.0 / (1.0 + mr));
        }
    }
    return out;
}

} // namespace tce

#include "indicator/hma.h"
#include "data/series.h"
#include <cmath>

namespace tce {

namespace {

// Weighted Moving Average — close 시리즈에서 period개 가중평균 (가중치 1..period).
std::vector<std::optional<double>> wma(const Series& s, int period) {
    const auto& cs = s.candles();
    std::vector<std::optional<double>> out(cs.size());
    if (period <= 0 || (int)cs.size() < period) return out;
    const double denom = static_cast<double>(period * (period + 1)) / 2.0;
    for (size_t i = static_cast<size_t>(period - 1); i < cs.size(); ++i) {
        double sum = 0;
        for (int k = 0; k < period; ++k) {
            // 가장 최근(=i)이 가중치 가장 큼
            sum += cs[i - static_cast<size_t>(k)].close * (period - k);
        }
        out[i] = sum / denom;
    }
    return out;
}

// optional 시리즈에 대한 WMA — HMA의 마지막 단계용.
std::vector<std::optional<double>> wmaOpt(const std::vector<std::optional<double>>& src,
                                            int period) {
    std::vector<std::optional<double>> out(src.size());
    if (period <= 0) return out;
    const double denom = static_cast<double>(period * (period + 1)) / 2.0;
    // 모든 (period개) 값이 valid한 윈도우만 계산
    for (size_t i = 0; i < src.size(); ++i) {
        if (i + 1 < static_cast<size_t>(period)) continue;
        double sum = 0;
        bool ok = true;
        for (int k = 0; k < period; ++k) {
            const auto& v = src[i - static_cast<size_t>(k)];
            if (!v) { ok = false; break; }
            sum += *v * (period - k);
        }
        if (ok) out[i] = sum / denom;
    }
    return out;
}

} // namespace

std::vector<std::optional<double>> hma(const Series& series, int period) {
    const auto& cs = series.candles();
    std::vector<std::optional<double>> out(cs.size());
    if (period <= 1 || (int)cs.size() < period) return out;

    auto w_half = wma(series, period / 2);
    auto w_full = wma(series, period);
    // raw[i] = 2 * wma(period/2)[i] - wma(period)[i] (둘 다 valid한 인덱스만)
    std::vector<std::optional<double>> raw(cs.size());
    for (size_t i = 0; i < cs.size(); ++i) {
        if (w_half[i] && w_full[i]) {
            raw[i] = 2.0 * (*w_half[i]) - (*w_full[i]);
        }
    }
    const int hp = std::max(1, static_cast<int>(std::lround(std::sqrt(static_cast<double>(period)))));
    return wmaOpt(raw, hp);
}

} // namespace tce

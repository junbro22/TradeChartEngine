#include "indicator/macd.h"
#include "indicator/ema.h"
#include "data/series.h"

namespace tce {

namespace {

// 시리즈처럼 동작하는 임시 어댑터 — EMA(EMA series) 계산용
class CloseSeriesAdapter {
public:
    CloseSeriesAdapter(const std::vector<std::optional<double>>& src) : src_(src) {}
    // EMA 함수가 Series를 기대하므로 별도 구현
    const std::vector<std::optional<double>>& src() const { return src_; }
private:
    const std::vector<std::optional<double>>& src_;
};

// signal line: EMA(macd line) — 단순 EMA를 옵셔널 시리즈에 적용
std::vector<std::optional<double>> emaOptional(const std::vector<std::optional<double>>& src,
                                                int period) {
    std::vector<std::optional<double>> out(src.size());
    if (period <= 0) return out;

    // 첫 유효 인덱스 찾기
    size_t firstIdx = 0;
    while (firstIdx < src.size() && !src[firstIdx]) ++firstIdx;
    if (firstIdx == src.size()) return out;

    // SMA seed (period개 모일 때까지)
    int collected = 0;
    double sum = 0.0;
    size_t seedAt = 0;
    for (size_t i = firstIdx; i < src.size(); ++i) {
        if (!src[i]) continue;
        sum += *src[i];
        ++collected;
        if (collected == period) { seedAt = i; break; }
    }
    if (collected < period) return out;

    out[seedAt] = sum / period;
    double prev = *out[seedAt];
    const double alpha = 2.0 / (period + 1);

    for (size_t i = seedAt + 1; i < src.size(); ++i) {
        if (!src[i]) continue;
        prev = alpha * (*src[i]) + (1.0 - alpha) * prev;
        out[i] = prev;
    }
    return out;
}

} // namespace

MacdResult macd(const Series& series, int fast, int slow, int signalPeriod) {
    auto fastEMA = ema(series, fast);
    auto slowEMA = ema(series, slow);

    MacdResult r;
    r.line.assign(fastEMA.size(), std::nullopt);
    for (size_t i = 0; i < fastEMA.size(); ++i) {
        if (fastEMA[i] && slowEMA[i]) {
            r.line[i] = *fastEMA[i] - *slowEMA[i];
        }
    }

    r.signal = emaOptional(r.line, signalPeriod);

    r.histogram.assign(r.line.size(), std::nullopt);
    for (size_t i = 0; i < r.line.size(); ++i) {
        if (r.line[i] && r.signal[i]) {
            r.histogram[i] = *r.line[i] - *r.signal[i];
        }
    }
    return r;
}

} // namespace tce

#include "data/series.h"
#include <algorithm>
#include <limits>

namespace tce {

void Series::setHistory(const TceCandle* data, size_t count) {
    candles_.assign(data, data + count);
}

void Series::append(const TceCandle& c) {
    if (!candles_.empty()) {
        const double last_ts = candles_.back().timestamp;
        if (c.timestamp == last_ts) {
            candles_.back() = c;
            return;
        }
        if (c.timestamp < last_ts) {
            // 정렬 가정 보호 — 더 작은 timestamp는 무시 (헤더 계약).
            return;
        }
    }
    candles_.push_back(c);
}

void Series::updateLast(double close, double volume) {
    if (candles_.empty()) return;
    auto& last = candles_.back();
    last.close = close;
    if (close > last.high) last.high = close;
    if (close < last.low)  last.low  = close;
    last.volume = volume;
}

double Series::minLowInRange(size_t from, size_t to) const {
    if (from >= to || from >= candles_.size()) return 0.0;
    to = std::min(to, candles_.size());
    double v = std::numeric_limits<double>::infinity();
    for (size_t i = from; i < to; ++i) v = std::min(v, candles_[i].low);
    return v == std::numeric_limits<double>::infinity() ? 0.0 : v;
}

double Series::maxHighInRange(size_t from, size_t to) const {
    if (from >= to || from >= candles_.size()) return 0.0;
    to = std::min(to, candles_.size());
    double v = -std::numeric_limits<double>::infinity();
    for (size_t i = from; i < to; ++i) v = std::max(v, candles_[i].high);
    return v == -std::numeric_limits<double>::infinity() ? 0.0 : v;
}

double Series::maxVolumeInRange(size_t from, size_t to) const {
    if (from >= to || from >= candles_.size()) return 0.0;
    to = std::min(to, candles_.size());
    double v = 0.0;
    for (size_t i = from; i < to; ++i) v = std::max(v, candles_[i].volume);
    return v;
}

} // namespace tce

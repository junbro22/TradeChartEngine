#include "chart.h"
#include <algorithm>

namespace tce {

Chart::Chart() = default;

void Chart::setHistory(const TceCandle* data, size_t count) {
    series_.setHistory(data, count);
}

void Chart::appendCandle(const TceCandle& c) {
    series_.append(c);
}

void Chart::updateLast(double close, double volume) {
    series_.updateLast(close, volume);
}

void Chart::addIndicator(TceIndicatorKind k, int period, TceColor c) {
    auto it = std::find_if(indicators_.begin(), indicators_.end(),
        [&](const IndicatorSpec& s) { return s.kind == k && s.period == period; });
    if (it != indicators_.end()) { it->color = c; return; }
    indicators_.push_back({k, period, c});
}

void Chart::removeIndicator(TceIndicatorKind k, int period) {
    indicators_.erase(std::remove_if(indicators_.begin(), indicators_.end(),
        [&](const IndicatorSpec& s) { return s.kind == k && s.period == period; }),
        indicators_.end());
}

void Chart::clearIndicators() { indicators_.clear(); }

void Chart::setCrosshair(float x, float y) {
    crosshair_.visible = true;
    crosshair_.x = x;
    crosshair_.y = y;
    int idx = viewport_.indexForX(series_.size(), x);
    crosshair_.candleIndex = idx;
    if (idx >= 0 && static_cast<size_t>(idx) < series_.size()) {
        crosshair_.timestamp = series_.candles()[idx].timestamp;
    }
    // price는 frame_builder의 YMap을 거쳐야 정확. 단순 추정으로:
    // 미리 계산해두지 않으면 정확한 매핑이 어려워 후속 패치 대상.
}

void Chart::clearCrosshair() {
    crosshair_ = CrosshairState{};
}

FrameOutput& Chart::buildFrame() {
    builder_.build(series_, viewport_, config_, indicators_, crosshair_, output_);
    return output_;
}

} // namespace tce

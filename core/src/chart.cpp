#include "chart.h"
#include <algorithm>

namespace tce {

Chart::Chart() = default;

void Chart::setHistory(const TceCandle* data, size_t count) {
    series_.setHistory(data, count);
}

void Chart::appendCandle(const TceCandle& c) {
    bool wasAtRightEdge = (viewport_.rightOffset() == 0);
    series_.append(c);
    if (!autoScroll_ && wasAtRightEdge) {
        // auto-scroll 꺼져있고 사용자가 오른쪽 끝에 있었으면 한 칸 뒤로 밀어
        // 보고 있던 캔들을 유지 (선택적 동작)
        // 기본 동작: 그대로 두면 viewport가 자동 우측 정렬됨
    }
    // autoScroll_ == true 면 rightOffset 그대로 (0이면 새 캔들이 자동 우측 끝)
}

void Chart::updateLast(double close, double volume) {
    series_.updateLast(close, volume);
}

void Chart::addOverlay(TceIndicatorKind k, int period, double param,
                       TceColor color, TceColor color2) {
    auto it = std::find_if(overlays_.begin(), overlays_.end(),
        [&](const OverlaySpec& s) { return s.kind == k && s.period == period; });
    if (it != overlays_.end()) {
        it->param = param; it->color = color; it->color2 = color2; return;
    }
    overlays_.push_back({k, period, param, color, color2});
}

void Chart::removeOverlay(TceIndicatorKind k, int period) {
    overlays_.erase(std::remove_if(overlays_.begin(), overlays_.end(),
        [&](const OverlaySpec& s) { return s.kind == k && s.period == period; }),
        overlays_.end());
}

void Chart::clearOverlays() { overlays_.clear(); }

void Chart::addSubpanel(TceIndicatorKind k, int p1, int p2, int p3,
                        TceColor color1, TceColor color2, TceColor color3) {
    auto it = std::find_if(subpanels_.begin(), subpanels_.end(),
        [&](const SubpanelSpec& s) { return s.kind == k; });
    if (it != subpanels_.end()) {
        it->p1 = p1; it->p2 = p2; it->p3 = p3;
        it->color1 = color1; it->color2 = color2; it->color3 = color3;
        return;
    }
    subpanels_.push_back({k, p1, p2, p3, color1, color2, color3});
}

void Chart::removeSubpanel(TceIndicatorKind k) {
    subpanels_.erase(std::remove_if(subpanels_.begin(), subpanels_.end(),
        [&](const SubpanelSpec& s) { return s.kind == k; }),
        subpanels_.end());
}

void Chart::clearSubpanels() { subpanels_.clear(); }

void Chart::setCrosshair(float x, float y) {
    crosshair_.visible = true;
    crosshair_.x = x;
    crosshair_.y = y;
    int idx = viewport_.indexForX(series_.size(), x);
    crosshair_.candleIndex = idx;
    if (idx >= 0 && static_cast<size_t>(idx) < series_.size()) {
        crosshair_.timestamp = series_.candles()[idx].timestamp;
    }
}

void Chart::clearCrosshair() {
    crosshair_ = CrosshairState{};
}

FrameOutput& Chart::buildFrame() {
    builder_.build(series_, viewport_, config_, overlays_, subpanels_, crosshair_, output_, lastLayout_);
    return output_;
}

LabelOutput& Chart::buildLabels() {
    labelBuilder_.build(series_, viewport_, config_,
                        lastLayout_.subpanelCount,
                        crosshair_,
                        lastLayout_.priceMin, lastLayout_.priceMax,
                        lastLayout_.mainTop, lastLayout_.mainBottom,
                        output_, labelOutput_);
    return labelOutput_;
}

} // namespace tce

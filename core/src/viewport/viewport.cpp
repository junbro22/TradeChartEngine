#include "viewport/viewport.h"
#include <algorithm>
#include <cmath>

namespace tce {

void Viewport::setSize(float w, float h) {
    width_  = std::max(1.0f, w);
    height_ = std::max(1.0f, h);
}

void Viewport::setVisibleCount(int c) {
    visibleCount_ = std::max(2, std::min(c, 5000));
}

void Viewport::setRightOffset(int o) {
    rightOffset_ = std::max(0, o);
}

float Viewport::slotWidth() const {
    return width_ / static_cast<float>(visibleCount_);
}

void Viewport::rangeFor(size_t total, size_t& outFrom, size_t& outTo) const {
    if (total == 0) { outFrom = outTo = 0; return; }
    int rightIdx = static_cast<int>(total) - 1 - rightOffset_;
    if (rightIdx < 0) rightIdx = 0;
    int leftIdx = rightIdx - (visibleCount_ - 1);
    if (leftIdx < 0) leftIdx = 0;
    outFrom = static_cast<size_t>(leftIdx);
    outTo   = static_cast<size_t>(rightIdx) + 1;
}

float Viewport::xForIndex(size_t total, size_t index) const {
    size_t from, to;
    rangeFor(total, from, to);
    if (to <= from) return 0.0f;
    const float w = slotWidth();
    const int relative = static_cast<int>(index) - static_cast<int>(from);
    return (static_cast<float>(relative) + 0.5f) * w;
}

int Viewport::indexForX(size_t total, float x) const {
    size_t from, to;
    rangeFor(total, from, to);
    if (to <= from) return -1;
    const float w = slotWidth();
    int rel = static_cast<int>(std::floor(x / w));
    int idx = static_cast<int>(from) + rel;
    if (idx < static_cast<int>(from) || idx >= static_cast<int>(to)) return -1;
    return idx;
}

void Viewport::pan(float dx_pixels) {
    const float w = slotWidth();
    if (w <= 0.0f) return;
    int delta = static_cast<int>(std::round(dx_pixels / w));
    rightOffset_ = std::max(0, rightOffset_ + delta);
}

void Viewport::zoom(float factor, float anchor_x) {
    if (factor <= 0.0f) return;
    // anchor_x 시간축 위치가 zoom 후에도 같은 캔들에 머물도록 rightOffset 보정.
    // anchor에 매핑된 슬롯 인덱스(가시 범위 내)는 visibleCount * (1 - x/W) 우측 거리에 있다.
    const float w = slotWidth();
    const int oldVisible = visibleCount_;
    int next = static_cast<int>(std::round(visibleCount_ / factor));
    setVisibleCount(next);
    const int newVisible = visibleCount_;
    if (oldVisible == newVisible || w <= 0.0f || width_ <= 0.0f) return;
    // anchor 슬롯의 우측 끝 기준 거리(슬롯 단위) — 시각적으로 anchor 위치 유지.
    float anchorFromRight = (width_ - anchor_x) / w; // old slot 단위
    int delta = static_cast<int>(std::round(
        anchorFromRight * (1.0f - static_cast<float>(newVisible) / static_cast<float>(oldVisible))
    ));
    rightOffset_ = std::max(0, rightOffset_ + delta);
}

} // namespace tce

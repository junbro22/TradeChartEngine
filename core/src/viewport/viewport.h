#pragma once

#include <cstddef>

namespace tce {

// 시계열 인덱스 + 화면 크기를 합쳐 가시 범위를 결정.
// rightOffset = 화면 우측 끝 다음에 비워둘 슬롯 수 (0 = 마지막 캔들이 우측 끝)
class Viewport {
public:
    void setSize(float width, float height);
    void setVisibleCount(int count);
    void setRightOffset(int offset);

    int  visibleCount() const { return visibleCount_; }
    int  rightOffset() const { return rightOffset_; }
    float width() const { return width_; }
    float height() const { return height_; }

    // 데이터 인덱스 범위 [from, to). totalCount는 series.size()
    void rangeFor(size_t totalCount, size_t& outFrom, size_t& outTo) const;

    // 캔들 인덱스 → 화면 X 중심 좌표
    float xForIndex(size_t totalCount, size_t index) const;

    // 화면 X → 가장 가까운 데이터 인덱스 (없으면 -1)
    int   indexForX(size_t totalCount, float x) const;

    // 인덱스 슬롯의 화면 폭 (캔들 폭)
    float slotWidth() const;

    // 핀치/드래그 헬퍼
    void pan(float dx_pixels);                   // 양수 = 왼쪽으로 데이터 이동(과거)
    void zoom(float factor, float anchor_x);     // factor > 1 = zoom-in (캔들 적게 보임)

private:
    float width_  = 1.0f;
    float height_ = 1.0f;
    int   visibleCount_ = 60;
    int   rightOffset_  = 0;       // 0 = 최신 캔들이 오른쪽 끝
};

} // namespace tce

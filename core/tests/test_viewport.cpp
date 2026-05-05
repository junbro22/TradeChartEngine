#include "viewport/viewport.h"
#include <cstdio>

#define EXPECT(cond) do { if (!(cond)) { std::printf("  FAIL %s:%d\n", __FILE__, __LINE__); ++failed; } } while (0)

int test_viewport() {
    int failed = 0;
    std::printf("test_viewport\n");

    tce::Viewport vp;
    vp.setSize(600.0f, 400.0f);
    vp.setVisibleCount(60);

    size_t from, to;
    vp.rangeFor(100, from, to);
    EXPECT(from == 40);
    EXPECT(to   == 100);

    vp.setRightOffset(10);
    vp.rangeFor(100, from, to);
    EXPECT(from == 30);
    EXPECT(to   == 90);

    vp.setRightOffset(0);
    int idx = vp.indexForX(100, 5.0f);          // slot ≈ 10px
    EXPECT(idx >= 40 && idx < 100);

    // pan: 데이터 절반 슬롯만큼
    int before = vp.rightOffset();
    vp.pan(50.0f);                               // 50/10 = 5 슬롯
    EXPECT(vp.rightOffset() > before);

    // zoom-in: visible 줄어듦
    vp.zoom(2.0f, 300.0f);
    EXPECT(vp.visibleCount() == 30);

    return failed;
}

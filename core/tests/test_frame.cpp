#include "tce/tce.h"
#include <cstdio>

#define EXPECT(cond) do { if (!(cond)) { std::printf("  FAIL %s:%d\n", __FILE__, __LINE__); ++failed; } } while (0)

int test_frame() {
    int failed = 0;
    std::printf("test_frame (C ABI)\n");

    TceContext* ctx = tce_create();
    EXPECT(ctx != nullptr);
    tce_set_size(ctx, 800.0f, 500.0f);

    TceCandle cs[10];
    for (int i = 0; i < 10; ++i) {
        cs[i] = TceCandle{(double)i, 100.0 + i, 110.0 + i, 95.0 + i, 105.0 + i, 1000.0 + i * 100};
    }
    tce_set_history(ctx, cs, 10);
    tce_set_visible_count(ctx, 10);

    TceColor red{0.93f, 0.27f, 0.27f, 1.0f};
    tce_add_indicator(ctx, TCE_IND_SMA, 3, red);

    TceFrame f = tce_build_frame(ctx);
    EXPECT(f.mesh_count >= 2);                  // 캔들 몸통 + 심지(+SMA)
    size_t totalVerts = 0;
    for (size_t i = 0; i < f.mesh_count; ++i) {
        totalVerts += f.meshes[i].vertex_count;
    }
    EXPECT(totalVerts > 0);
    tce_release_frame(f);

    EXPECT(tce_candle_count(ctx) == 10);

    // 라인 차트로 변경
    tce_set_series_type(ctx, TCE_SERIES_LINE);
    f = tce_build_frame(ctx);
    EXPECT(f.mesh_count >= 1);
    tce_release_frame(f);

    tce_destroy(ctx);
    return failed;
}

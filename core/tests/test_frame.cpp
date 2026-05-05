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

    // append_candle: 더 작은 timestamp는 거부, 동일 timestamp는 갱신
    {
        TceCandle smaller{5.0, 200, 200, 200, 200, 999};
        tce_append_candle(ctx, &smaller);
        EXPECT(tce_candle_count(ctx) == 10); // 거부
        TceCandle same{9.0, 999, 999, 999, 999, 999};
        tce_append_candle(ctx, &same);
        EXPECT(tce_candle_count(ctx) == 10); // 갱신
        TceCandle next{20.0, 200, 200, 200, 200, 999};
        tce_append_candle(ctx, &next);
        EXPECT(tce_candle_count(ctx) == 11); // 추가
    }

    // tce_get_candle 동작 — 범위 밖, 정상
    {
        TceCandle out{};
        EXPECT(tce_get_candle(ctx, 0, &out) == 1);
        EXPECT(out.timestamp == 0.0);
        EXPECT(tce_get_candle(ctx, 999, &out) == 0);
        EXPECT(tce_get_candle(ctx, 0, nullptr) == 0);
    }

    // HA 모드에서도 frame이 정상 빌드 (지표가 원본 OHLC로 계산되어 인덱스 충돌 없음)
    {
        tce_set_series_type(ctx, TCE_SERIES_HEIKIN_ASHI);
        TceColor c{0.5f, 0.5f, 1.0f, 1.0f};
        tce_add_rsi(ctx, 5, c);
        TceFrame ha = tce_build_frame(ctx);
        EXPECT(ha.mesh_count >= 1);
        tce_release_frame(ha);
    }

    // tce_fit_all
    {
        tce_fit_all(ctx);
        EXPECT(tce_right_offset(ctx) == 0);
        EXPECT(tce_visible_count(ctx) >= 1);
    }

    tce_destroy(ctx);
    return failed;
}

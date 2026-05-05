#include "tce/tce.h"
#include <cstdio>
#include <cmath>

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

    // 좌표 변환 — LINEAR 모드에서 round-trip
    {
        tce_set_series_type(ctx, TCE_SERIES_CANDLE);
        tce_set_price_axis_mode(ctx, TCE_PRICE_LINEAR);
        TceFrame f0 = tce_build_frame(ctx);
        (void)f0; tce_release_frame(f0);

        TceLayout L = tce_layout(ctx);
        // plot 영역 안의 임의 점에서 X→idx→X round-trip
        float midX = L.plot.x + L.plot.width * 0.5f;
        int idx = tce_screen_x_to_index(ctx, midX);
        EXPECT(idx >= 0);
        float xBack = 0;
        EXPECT(tce_index_to_screen_x(ctx, idx, &xBack) == 1);
        // 슬롯 폭 안쪽이라 정확히 같지는 않지만 같은 슬롯이어야
        EXPECT(std::fabs(xBack - midX) <= L.plot.width / tce_visible_count(ctx) * 0.5f + 1.0f);

        // Y → price → Y round-trip
        float midY = L.plot.y + L.plot.height * 0.5f;
        double p = tce_screen_y_to_price(ctx, midY);
        float yBack = tce_price_to_screen_y(ctx, p);
        EXPECT(std::fabs(yBack - midY) < 1e-3f);

        // plot 밖 X → -1
        EXPECT(tce_screen_x_to_index(ctx, L.plot.x - 5) == -1);
        EXPECT(tce_screen_x_to_index(ctx, L.plot.x + L.plot.width + 5) == -1);

        // index → X 가시 범위 밖이면 0
        float ignored;
        EXPECT(tce_index_to_screen_x(ctx, -1, &ignored) == 0);
        EXPECT(tce_index_to_screen_x(ctx, 9999, &ignored) == 0);
    }

    // 좌표 변환 — LOG 모드에서 round-trip (raw price 정확)
    {
        tce_set_price_axis_mode(ctx, TCE_PRICE_LOG);
        TceFrame f0 = tce_build_frame(ctx);
        (void)f0; tce_release_frame(f0);

        TceLayout L = tce_layout(ctx);
        float midY = L.plot.y + L.plot.height * 0.5f;
        double rawP = tce_screen_y_to_price(ctx, midY);
        EXPECT(rawP > 0);   // log 역변환이 양수여야
        float yBack = tce_price_to_screen_y(ctx, rawP);
        EXPECT(std::fabs(yBack - midY) < 1e-3f);

        // 모드 복원
        tce_set_price_axis_mode(ctx, TCE_PRICE_LINEAR);
    }

    // 알림 cross 콜백
    {
        struct Acc { int hits = 0; int last_id = 0; double last_price = 0.0; };
        Acc acc;
        tce_set_alert_callback(ctx,
            [](int id, double cp, void* user) {
                auto* a = static_cast<Acc*>(user);
                a->hits++;
                a->last_id = id;
                a->last_price = cp;
            }, &acc);

        // 현재 마지막 close 이상으로 alert 추가 — 위로 cross 시 fire
        TceCandle last{};
        tce_get_candle(ctx, tce_candle_count(ctx) - 1, &last);
        TceColor c{1, 0, 0, 1};
        int aid = tce_add_alert_line(ctx, last.close + 5.0, c);
        EXPECT(aid > 0);

        // 같거나 작은 close → fire 안 함
        tce_update_last(ctx, last.close + 4.99, last.volume);
        EXPECT(acc.hits == 0);

        // alert.price 위로 close → 1회 fire
        tce_update_last(ctx, last.close + 5.0, last.volume);
        EXPECT(acc.hits == 1);
        EXPECT(acc.last_id == aid);

        // 같은 가격 머무름 → 추가 fire 없음
        tce_update_last(ctx, last.close + 5.0, last.volume);
        EXPECT(acc.hits == 1);

        // 아래로 다시 cross → 1회 더 fire
        tce_update_last(ctx, last.close + 4.5, last.volume);
        EXPECT(acc.hits == 2);

        // 콜백 해제
        tce_set_alert_callback(ctx, nullptr, nullptr);
        tce_update_last(ctx, last.close + 6.0, last.volume);
        EXPECT(acc.hits == 2);

        tce_clear_alert_lines(ctx);
    }

    // Donchian frame build (회귀: 새 enum 분기)
    {
        TceColor mid{1, 1, 1, 1};
        TceColor edge{1, 1, 1, 0.5f};
        tce_add_donchian(ctx, 5, mid, edge);
        TceFrame fdc = tce_build_frame(ctx);
        EXPECT(fdc.mesh_count >= 1);
        tce_release_frame(fdc);
        tce_clear_indicators(ctx);
    }

    tce_destroy(ctx);
    return failed;
}

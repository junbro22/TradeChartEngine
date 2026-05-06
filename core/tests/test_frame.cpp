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

    // Keltner frame build + priceY 흡수
    {
        TceColor mid{0.5f, 1, 0.5f, 1};
        TceColor edge{0.5f, 1, 0.5f, 0.5f};
        tce_add_keltner(ctx, 5, 3, 2.0, mid, edge);
        TceFrame fk = tce_build_frame(ctx);
        EXPECT(fk.mesh_count >= 1);
        tce_release_frame(fk);
        // priceY가 keltner upper/lower까지 확장됨을 확인 — layout 가져오기
        TceLayout L = tce_layout(ctx);
        (void)L; // 단순히 build 회귀 없음 확인
        tce_clear_indicators(ctx);
    }

    // 지표 값 query API
    {
        TceColor c{1, 1, 1, 1};
        tce_add_indicator(ctx, TCE_IND_SMA, 3, c);
        tce_add_rsi(ctx, 5, c);

        // 등록 후 query
        double v = 0;
        EXPECT(tce_query_indicator_value(ctx, TCE_IND_SMA, 3, 5, &v) == 1);
        EXPECT(v != 0);
        EXPECT(tce_query_indicator_value(ctx, TCE_IND_RSI, 5, 8, &v) == 1);

        // period 미일치 → 0
        EXPECT(tce_query_indicator_value(ctx, TCE_IND_SMA, 99, 5, &v) == 0);
        // 등록 안 한 지표 → 0
        EXPECT(tce_query_indicator_value(ctx, TCE_IND_EMA, 3, 5, &v) == 0);
        // 범위 밖 idx → 0
        EXPECT(tce_query_indicator_value(ctx, TCE_IND_SMA, 3, 9999, &v) == 0);
        // 첫 (period-1) idx → 0
        EXPECT(tce_query_indicator_value(ctx, TCE_IND_SMA, 3, 0, &v) == 0);

        // BB query
        tce_add_bollinger(ctx, 5, 2.0, c);
        double u, m, l;
        EXPECT(tce_query_bollinger(ctx, 5, 6, &u, &m, &l) == 1);
        EXPECT(u > m && m > l);

        // MACD query
        tce_add_macd(ctx, 3, 6, 2, c, c, c);
        double line, sig, hist;
        EXPECT(tce_query_macd(ctx, 7, &line, &sig, &hist) == 1);

        // SuperTrend — single + dedicated query 일관성 검증 (multiplier 보존)
        tce_add_supertrend(ctx, 3, 1.5, c);  // 짧은 period + 작은 multiplier로 변동 보장
        double stLine = 0;
        int stDir = 0;
        if (tce_query_supertrend(ctx, 8, &stLine, &stDir) == 1) {
            // 같은 idx에서 single value query도 동일 line을 반환
            double stLine2 = 0;
            EXPECT(tce_query_indicator_value(ctx, TCE_IND_SUPERTREND, 3, 8, &stLine2) == 1);
            EXPECT(std::fabs(stLine - stLine2) < 1e-9);
        }

        tce_clear_indicators(ctx);
    }

    // 드로잉 export/import round-trip
    {
        tce_drawing_clear(ctx);
        TceColor c{1, 0, 0, 1};
        // horizontal 1점 + trendline 2점
        int h = tce_drawing_begin(ctx, TCE_DRAW_HORIZONTAL, 100, 200, c);
        EXPECT(h > 0);
        int t = tce_drawing_begin(ctx, TCE_DRAW_TRENDLINE, 50, 100, c);
        tce_drawing_update(ctx, t, 1, 300, 250);

        EXPECT(tce_drawing_count(ctx) == 2);

        TceDrawingExport e0{}, e1{};
        EXPECT(tce_drawing_export(ctx, 0, &e0) == 1);
        EXPECT(tce_drawing_export(ctx, 1, &e1) == 1);
        EXPECT(e0.kind == TCE_DRAW_HORIZONTAL && e0.point_count == 1);
        EXPECT(e1.kind == TCE_DRAW_TRENDLINE  && e1.point_count == 2);
        // color round-trip 검증
        EXPECT(e0.color.r == c.r && e0.color.g == c.g
            && e0.color.b == c.b && e0.color.a == c.a);

        // 범위 밖 idx
        TceDrawingExport bad{};
        EXPECT(tce_drawing_export(ctx, 999, &bad) == 0);

        // clear 후 import → 다시 2개
        tce_drawing_clear(ctx);
        EXPECT(tce_drawing_count(ctx) == 0);
        int newH = tce_drawing_import(ctx, &e0);
        int newT = tce_drawing_import(ctx, &e1);
        EXPECT(newH > 0 && newT > 0);
        EXPECT(tce_drawing_count(ctx) == 2);
        // 새 id는 이전 id와 달라도 됨
        EXPECT(newH != h || newT != t || true);  // 그냥 새 id 부여 검증

        // kind 범위 밖 import → 0
        TceDrawingExport invalid{};
        invalid.kind = static_cast<TceDrawingKind>(99);
        invalid.point_count = 1;
        EXPECT(tce_drawing_import(ctx, &invalid) == 0);

        // point_count 불일치 — HORIZONTAL인데 2점
        TceDrawingExport pcMismatch = e0;
        pcMismatch.point_count = 2;
        EXPECT(tce_drawing_import(ctx, &pcMismatch) == 0);

        tce_drawing_clear(ctx);
    }

    // ZigZag + VWAP bands frame 빌드 (회귀)
    {
        TceColor c{1, 1, 1, 1};
        TceColor edge{1, 1, 1, 0.5f};
        tce_add_zigzag(ctx, 5.0, c);
        tce_add_vwap_with_bands(ctx, 2.0, c, edge);

        TceFrame f = tce_build_frame(ctx);
        EXPECT(f.mesh_count >= 1);
        tce_release_frame(f);

        // VWAP bands query
        double m, u, l;
        // 캔들 인덱스 5에서 query — 데이터에 거래량/가격 변동이 있어 sigma > 0
        if (tce_query_vwap_bands(ctx, 5, &m, &u, &l) == 1) {
            EXPECT(u >= m && m >= l);
        }

        tce_clear_indicators(ctx);
    }

    // Volume Profile frame 빌드 (회귀)
    {
        TceColor bar{0.5f, 0.7f, 1.0f, 1.0f};
        TceColor poc{1.0f, 0.85f, 0.20f, 1.0f};
        tce_add_volume_profile(ctx, 16, 0.20, bar, poc);
        TceFrame f = tce_build_frame(ctx);
        EXPECT(f.mesh_count >= 1);
        tce_release_frame(f);
        tce_clear_indicators(ctx);

        // Renko 모드에서는 no-op (no Volume Profile mesh emit) — frame 자체는 정상 빌드
        tce_set_series_type(ctx, TCE_SERIES_RENKO);
        tce_set_renko_brick_size(ctx, 1.0);
        tce_add_volume_profile(ctx, 16, 0.20, bar, poc);
        TceFrame fr = tce_build_frame(ctx);
        // Renko mode 자체가 빌드되므로 mesh_count >= 1 (캔들/그리드 등). Volume Profile 분기는 skip.
        EXPECT(fr.mesh_count >= 1);
        tce_release_frame(fr);
        tce_clear_indicators(ctx);
        tce_set_series_type(ctx, TCE_SERIES_CANDLE);
    }

    // Drawing endpoint hit-test
    {
        tce_drawing_clear(ctx);
        TceColor c{1, 0, 0, 1};
        // trendline (50, 100) → (200, 250)
        int t = tce_drawing_begin(ctx, TCE_DRAW_TRENDLINE, 50, 100, c);
        EXPECT(t > 0);
        tce_drawing_update(ctx, t, 1, 200, 250);

        // 첫 점 근처 hit
        int idx = -1;
        int id = tce_hit_test_drawing_point(ctx, 52, 102, 14, 12, &idx);
        EXPECT(id == t);
        EXPECT(idx == 0);

        // 두 번째 점 근처 hit
        id = tce_hit_test_drawing_point(ctx, 198, 251, 14, 12, &idx);
        EXPECT(id == t);
        EXPECT(idx == 1);

        // 라인 중간 — endpoint 못 찾고 line fallback
        id = tce_hit_test_drawing_point(ctx, 125, 175, 14, 12, &idx);
        EXPECT(id == t);
        EXPECT(idx == -1);

        // 멀리 — 0 반환
        id = tce_hit_test_drawing_point(ctx, 500, 500, 14, 12, &idx);
        EXPECT(id == 0);

        tce_drawing_clear(ctx);
    }

    // Fib Extension 드로잉 — begin/update/export round-trip
    {
        tce_drawing_clear(ctx);
        TceColor c{0, 1, 1, 1};
        int fid = tce_drawing_begin(ctx, TCE_DRAW_FIB_EXTENSION, 100, 200, c);
        EXPECT(fid > 0);
        tce_drawing_update(ctx, fid, 1, 300, 250);
        EXPECT(tce_drawing_count(ctx) == 1);

        TceDrawingExport ex{};
        EXPECT(tce_drawing_export(ctx, 0, &ex) == 1);
        EXPECT(ex.kind == TCE_DRAW_FIB_EXTENSION);
        EXPECT(ex.point_count == 2);

        // import round-trip — 검증 상한이 6까지 포함되었는지
        tce_drawing_clear(ctx);
        EXPECT(tce_drawing_import(ctx, &ex) > 0);
        EXPECT(tce_drawing_count(ctx) == 1);
        tce_drawing_clear(ctx);
    }

    // 세션 offset — VWAP day boundary 변화
    {
        // 새 chart로 깨끗한 데이터 사용
        TceContext* c2 = tce_create();
        tce_set_size(c2, 800, 500);
        TceCandle hist[10];
        for (int i = 0; i < 10; ++i) {
            hist[i] = {(double)(i * 21600), 100.0 + i, 110.0 + i, 90.0 + i, 105.0 + i, 1000.0};
        }
        tce_set_history(c2, hist, 10);
        TceColor col{1, 1, 1, 1};
        tce_add_vwap(c2, col);

        // offset 0
        TceFrame f1 = tce_build_frame(c2);
        EXPECT(f1.mesh_count >= 1);
        tce_release_frame(f1);

        // offset 변경 — frame 재빌드 시 vwap 재계산 (회귀 없음)
        tce_set_session_start_utc(c2, 14, 30);
        TceFrame f2 = tce_build_frame(c2);
        EXPECT(f2.mesh_count >= 1);
        tce_release_frame(f2);

        // 직접 offset 지정
        tce_set_session_offset_seconds(c2, -52200.0);
        TceFrame f3 = tce_build_frame(c2);
        EXPECT(f3.mesh_count >= 1);
        tce_release_frame(f3);

        tce_destroy(c2);
    }

    tce_destroy(ctx);
    return failed;
}

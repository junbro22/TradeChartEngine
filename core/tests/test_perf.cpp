// 성능 측정 — build_frame / query 비용 baseline.
// 결과를 토대로 indicator 캐싱 도입 여부 결정.
#include "tce/tce.h"
#include <chrono>
#include <cstdio>
#include <cmath>
#include <vector>

int test_perf() {
    // 5000 candles 합성 (실측에 가까운 변동성)
    constexpr size_t N = 5000;
    TceContext* ctx = tce_create();
    tce_set_size(ctx, 800.0f, 500.0f);

    // heap 할당 — 다른 OS의 작은 스택(80~512KB) 안전
    std::vector<TceCandle> cs(N);
    double price = 100.0;
    for (size_t i = 0; i < N; ++i) {
        // 의사 난수 가격 변동 (sine + drift)
        double drift = std::sin(static_cast<double>(i) * 0.05) * 5.0;
        double noise = std::sin(static_cast<double>(i) * 0.31) * 0.8;
        price += drift * 0.01 + noise * 0.05;
        if (price < 10) price = 10;
        cs[i] = {(double)(i * 60),
                 price, price + 1.0, price - 1.0, price + 0.3,
                 1000.0 + std::fabs(noise) * 200.0};
    }
    tce_set_history(ctx, cs.data(), N);
    tce_set_visible_count(ctx, 200);

    // 8개 지표 등록 (typical 화면)
    TceColor c{1, 1, 1, 1};
    tce_add_indicator(ctx, TCE_IND_SMA, 20, c);
    tce_add_indicator(ctx, TCE_IND_EMA, 60, c);
    tce_add_bollinger(ctx, 20, 2.0, c);
    tce_add_rsi(ctx, 14, c);
    tce_add_macd(ctx, 12, 26, 9, c, c, c);
    tce_add_stochastic(ctx, 14, 3, 3, c, c);
    tce_add_atr(ctx, 14, c);
    tce_add_ichimoku(ctx, 9, 26, 52, 26, c, c);

    // build_frame × 1000회 timing
    constexpr int FRAMES = 1000;
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < FRAMES; ++i) {
        TceFrame f = tce_build_frame(ctx);
        (void)f;
        tce_release_frame(f);
    }
    auto t1 = std::chrono::steady_clock::now();
    double total_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double avg_ms = total_ms / FRAMES;

    std::printf("test_perf\n");
    std::printf("  build_frame: %.3f ms avg (%d frames, total %.1f ms, %zu candles, 8 indicators)\n",
                avg_ms, FRAMES, total_ms, N);

    // query × 10000회 timing
    constexpr int QUERIES = 10000;
    auto q0 = std::chrono::steady_clock::now();
    double v;
    for (int i = 0; i < QUERIES; ++i) {
        size_t idx = 2500 + (i % 100);
        tce_query_indicator_value(ctx, TCE_IND_RSI, 14, idx, &v);
    }
    auto q1 = std::chrono::steady_clock::now();
    double q_total = std::chrono::duration<double, std::milli>(q1 - q0).count();
    std::printf("  query RSI:   %.4f ms avg (%d queries, total %.1f ms)\n",
                q_total / QUERIES, QUERIES, q_total);

    // 임계 평가 — Release 빌드에서 평균 3ms 이상이면 캐싱 검토.
    // (Debug 빌드에선 측정 무의미 — README에 명시.)
    if (avg_ms > 3.0) {
        std::printf("  NOTE: build_frame > 3ms — indicator caching 도입 검토 권고.\n");
    } else {
        std::printf("  OK: build_frame within budget — 캐싱 yagni 유지.\n");
    }

    tce_destroy(ctx);
    return 0;  // perf 테스트는 항상 PASS (측정만)
}

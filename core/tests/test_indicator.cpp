#include "data/series.h"
#include "indicator/ma.h"
#include "indicator/ema.h"
#include "indicator/donchian.h"
#include "indicator/keltner.h"
#include "indicator/zigzag.h"
#include "indicator/hma.h"
#include "indicator/stoch_rsi.h"
#include "indicator/atr.h"
#include "indicator/vwap.h"
#include "indicator/pivot.h"
#include <cstdio>
#include <cmath>

#define EXPECT(cond) do { if (!(cond)) { std::printf("  FAIL %s:%d\n", __FILE__, __LINE__); ++failed; } } while (0)

int test_indicator() {
    int failed = 0;
    std::printf("test_indicator\n");

    tce::Series s;
    TceCandle hist[5];
    double closes[5] = {10, 11, 12, 13, 14};
    for (int i = 0; i < 5; ++i) {
        hist[i] = {(double)i, closes[i], closes[i], closes[i], closes[i], 1.0};
    }
    s.setHistory(hist, 5);

    auto sma3 = tce::sma(s, 3);
    EXPECT(sma3.size() == 5);
    EXPECT(!sma3[0].has_value());
    EXPECT(!sma3[1].has_value());
    EXPECT(sma3[2].has_value() && std::fabs(*sma3[2] - 11.0) < 1e-9);  // (10+11+12)/3
    EXPECT(sma3[3].has_value() && std::fabs(*sma3[3] - 12.0) < 1e-9);
    EXPECT(sma3[4].has_value() && std::fabs(*sma3[4] - 13.0) < 1e-9);

    auto ema3 = tce::ema(s, 3);
    EXPECT(ema3.size() == 5);
    EXPECT(!ema3[1].has_value());
    EXPECT(ema3[2].has_value() && std::fabs(*ema3[2] - 11.0) < 1e-9);
    EXPECT(ema3[3].has_value());
    // alpha=0.5 -> 0.5*13 + 0.5*11 = 12.0
    EXPECT(std::fabs(*ema3[3] - 12.0) < 1e-9);
    // 0.5*14 + 0.5*12 = 13.0
    EXPECT(std::fabs(*ema3[4] - 13.0) < 1e-9);

    // Donchian Channels — high/low가 다르도록 시리즈를 만들어 검증
    {
        tce::Series ds;
        TceCandle hi[5];
        // (open, high, low, close)
        double H[5] = {10, 12, 11, 13, 14};
        double L[5] = { 8,  9,  7, 10, 12};
        for (int i = 0; i < 5; ++i) {
            hi[i] = {(double)i, H[i], H[i], L[i], L[i], 1.0};
        }
        ds.setHistory(hi, 5);
        auto dc = tce::donchian(ds, 3);
        EXPECT(dc.upper.size() == 5);
        EXPECT(!dc.upper[0].has_value());
        EXPECT(!dc.upper[1].has_value());
        // i=2: max(12,11,10)=12, min(7,9,8)=7
        EXPECT(dc.upper[2].has_value()  && std::fabs(*dc.upper[2]  - 12.0) < 1e-9);
        EXPECT(dc.lower[2].has_value()  && std::fabs(*dc.lower[2]  -  7.0) < 1e-9);
        EXPECT(dc.middle[2].has_value() && std::fabs(*dc.middle[2] -  9.5) < 1e-9);
        // i=4: max(14,13,11)=14, min(12,10,7)=7
        EXPECT(dc.upper[4].has_value()  && std::fabs(*dc.upper[4]  - 14.0) < 1e-9);
        EXPECT(dc.lower[4].has_value()  && std::fabs(*dc.lower[4]  -  7.0) < 1e-9);
        EXPECT(dc.middle[4].has_value() && std::fabs(*dc.middle[4] - 10.5) < 1e-9);

        // period >= count → 모두 nullopt
        auto dc6 = tce::donchian(ds, 6);
        for (size_t i = 0; i < dc6.upper.size(); ++i) {
            EXPECT(!dc6.upper[i].has_value());
        }
    }

    // Keltner Channels — middle == EMA, upper/lower 일치 검증
    {
        tce::Series ks;
        TceCandle hi[10];
        for (int i = 0; i < 10; ++i) {
            double base = 100.0 + i;
            hi[i] = {(double)i, base, base + 2, base - 2, base, 1.0};
        }
        ks.setHistory(hi, 10);
        auto k = tce::keltner(ks, 5, 3, 2.0);
        auto e5 = tce::ema(ks, 5);
        auto a3 = tce::atr(ks, 3);
        for (size_t i = 0; i < 10; ++i) {
            if (e5[i] && a3[i]) {
                EXPECT(k.middle[i].has_value() && std::fabs(*k.middle[i] - *e5[i]) < 1e-9);
                EXPECT(k.upper[i].has_value()  && std::fabs(*k.upper[i]  - (*e5[i] + 2.0 * *a3[i])) < 1e-9);
                EXPECT(k.lower[i].has_value()  && std::fabs(*k.lower[i]  - (*e5[i] - 2.0 * *a3[i])) < 1e-9);
            }
        }
    }

    // 세션 offset — VWAP가 day boundary에서 reset되는지 검증
    {
        tce::Series vs;
        // ts = 0, 21600, 43200, 64800, 86400, 108000, ...  (6h 간격)
        // offset=0: floor(ts/86400)이 i=3에서 0, i=4에서 1 → i=4에서 reset.
        TceCandle vc[10];
        for (int i = 0; i < 10; ++i) {
            vc[i] = {(double)(i * 21600), 100.0 + i, 110.0 + i, 90.0 + i, 105.0 + i, 1000.0};
        }
        vs.setHistory(vc, 10);
        auto v = tce::vwap(vs, 0);
        EXPECT(v.size() == 10);
        // i=4에서 cumPV/cumV reset → VWAP[4] == typical[4]
        EXPECT(v[4].has_value());
        double tp4 = (vc[4].high + vc[4].low + vc[4].close) / 3.0;
        EXPECT(std::fabs(*v[4] - tp4) < 1e-9);

        // offset=-43200: ts+offset = -43200, -21600, 0, 21600, 43200, 64800, 86400, ...
        // floor: -1, -1, 0, 0, 0, 0, 1, ... → reset at i=2 and i=6.
        auto v2 = tce::vwap(vs, -43200);
        EXPECT(v2[2].has_value());
        double tp2 = (vc[2].high + vc[2].low + vc[2].close) / 3.0;
        EXPECT(std::fabs(*v2[2] - tp2) < 1e-9);
        EXPECT(v2[6].has_value());
        double tp6 = (vc[6].high + vc[6].low + vc[6].close) / 3.0;
        EXPECT(std::fabs(*v2[6] - tp6) < 1e-9);
    }

    // Pivot — offset 0 vs offset 변경 시 boundary 위치 변화
    {
        tce::Series ps;
        TceCandle pc[15];
        for (int i = 0; i < 15; ++i) {
            pc[i] = {(double)(i * 21600), 100.0, 110.0 + i, 90.0 - i, 100.0, 1000.0};
        }
        ps.setHistory(pc, 15);
        auto piv0 = tce::pivot(ps, tce::PivotKind::Standard, 0);
        // 첫 거래일은 모두 nullopt
        EXPECT(!piv0.p[0].has_value());
        // 5번째부터(day 1) 값 있어야
        EXPECT(piv0.p[4].has_value() || piv0.p[5].has_value());
    }

    // VWAP bands — middle == vwap, upper > middle > lower
    {
        tce::Series vbs;
        TceCandle bc[8];
        for (int i = 0; i < 8; ++i) {
            // 가격 변동 추가하여 sigma > 0 보장
            double tp = 100.0 + (i % 2 == 0 ? +5 : -3);
            bc[i] = {(double)(i * 3600), tp, tp + 1, tp - 1, tp, 1000.0};
        }
        vbs.setHistory(bc, 8);
        auto baseline = tce::vwap(vbs, 0);
        auto vb = tce::vwapBands(vbs, 0, 2.0);
        // middle은 plain vwap과 동일해야
        for (size_t i = 0; i < 8; ++i) {
            if (baseline[i] && vb.middle[i]) {
                EXPECT(std::fabs(*baseline[i] - *vb.middle[i]) < 1e-9);
            }
        }
        // 마지막 캔들 sigma > 0이라 upper > middle > lower
        EXPECT(vb.upper[7].has_value() && vb.middle[7].has_value() && vb.lower[7].has_value());
        EXPECT(*vb.upper[7] > *vb.middle[7]);
        EXPECT(*vb.middle[7] > *vb.lower[7]);

        // numStdev<=0 → upper/lower nullopt
        auto vb0 = tce::vwapBands(vbs, 0, 0);
        EXPECT(!vb0.upper[7].has_value());
        EXPECT(vb0.middle[7].has_value());
    }

    // ZigZag — 단조 증가 시리즈는 swing 1개만 (시작점) 있어야
    {
        tce::Series zs;
        TceCandle zc[10];
        for (int i = 0; i < 10; ++i) {
            double p = 100.0 + i;
            zc[i] = {(double)i, p, p, p, p, 1.0};
        }
        zs.setHistory(zc, 10);
        auto zz = tce::zigzag(zs, 5.0);  // 5% deviation 필요한데 단조 증가 (10% 까지)
        size_t count = 0;
        for (const auto& v : zz) if (v) ++count;
        EXPECT(count >= 1);  // 시작 swing 1개 + 마지막 잠정 swing 1개

        // M자 형태: 100 → 110 → 95 → 115 → 90 (deviation 충분)
        tce::Series ms;
        TceCandle mc[5] = {
            {0, 100, 100, 100, 100, 1.0},
            {1, 110, 110, 110, 110, 1.0},
            {2,  95,  95,  95,  95, 1.0},
            {3, 115, 115, 115, 115, 1.0},
            {4,  90,  90,  90,  90, 1.0},
        };
        ms.setHistory(mc, 5);
        auto zz2 = tce::zigzag(ms, 5.0);
        size_t swingCount = 0;
        for (const auto& v : zz2) if (v) ++swingCount;
        // 시작 100, swing 110, 95, 115, 잠정 90 — 5개 정도 기대
        EXPECT(swingCount >= 4);

        // 시작점 보정 검증 — 첫 캔들 close=100인데 low=80이면 시작 swing이 80이어야
        tce::Series ext;
        TceCandle ec[4];
        ec[0] = {0, 100, 105,  80, 100, 1.0};   // close 100, 단 low 80
        ec[1] = {1, 100, 105,  80, 100, 1.0};
        ec[2] = {2, 100, 105,  80, 100, 1.0};
        ec[3] = {3, 110, 120, 110, 120, 1.0};   // 80 → 120 = +50% 충분히 trigger
        ext.setHistory(ec, 4);
        auto zz3 = tce::zigzag(ext, 5.0);
        // 시작 swing은 idx 0/1/2 중 하나에서 80이어야 함 (close=100 아님)
        bool foundLow = false;
        for (size_t i = 0; i < 3; ++i) {
            if (zz3[i] && std::fabs(*zz3[i] - 80.0) < 1e-9) { foundLow = true; break; }
        }
        EXPECT(foundLow);

        // 하락 방향 — 첫 캔들 close=100인데 high=120이면 시작 swing이 120
        tce::Series dn;
        TceCandle dc[4];
        dc[0] = {0, 100, 120, 100, 100, 1.0};
        dc[1] = {1, 100, 120, 100, 100, 1.0};
        dc[2] = {2, 100, 120, 100, 100, 1.0};
        dc[3] = {3,  80,  85,  60,  60, 1.0};  // 120 → 60 = -50% trigger
        dn.setHistory(dc, 4);
        auto zz4 = tce::zigzag(dn, 5.0);
        bool foundHigh = false;
        for (size_t i = 0; i < 3; ++i) {
            if (zz4[i] && std::fabs(*zz4[i] - 120.0) < 1e-9) { foundHigh = true; break; }
        }
        EXPECT(foundHigh);
    }

    // Stochastic RSI — 단조 증가 close에서 RSI=100 수렴 → stochRSI도 100 근처 유지
    {
        tce::Series ss;
        TceCandle sc[50];
        for (int i = 0; i < 50; ++i) {
            double p = 100.0 + i;
            sc[i] = {(double)i, p, p, p, p, 1.0};
        }
        ss.setHistory(sc, 50);
        auto sr = tce::stochasticRSI(ss, 14, 14, 3, 3);
        EXPECT(sr.k.size() == 50);
        // RSI(14)=14..49, raw(kPeriod=14)=27..49, k=SMA(raw,3)=29..49, d=SMA(k,3)=31..49
        EXPECT(sr.k[40].has_value());
        EXPECT(sr.d[40].has_value());

        // smooth=1, dPeriod=1 fallback — raw 그대로 통과
        auto sr1 = tce::stochasticRSI(ss, 14, 14, 1, 1);
        EXPECT(sr1.k[40].has_value());
        EXPECT(sr1.d[40].has_value());
    }

    // HMA — period 4 단조증가 시리즈에서 close에 빠르게 따라감 검증
    {
        tce::Series hs;
        TceCandle hc[10];
        for (int i = 0; i < 10; ++i) {
            double p = 100.0 + i * 2.0;  // 100, 102, ..., 118
            hc[i] = {(double)i, p, p, p, p, 1.0};
        }
        hs.setHistory(hc, 10);
        auto h = tce::hma(hs, 4);
        // sqrt(4)=2 → wma 단계 모두 적용 후 인덱스 정상 채워짐
        EXPECT(h.size() == 10);
        // period < 4면 nullopt
        EXPECT(!h[0].has_value());
        // 충분히 큰 idx에서는 valid + close에 가까움 (단조 증가)
        EXPECT(h[8].has_value() && h[9].has_value());
        // HMA(8) < HMA(9) (상승 추세)
        EXPECT(*h[8] < *h[9]);
    }

    return failed;
}

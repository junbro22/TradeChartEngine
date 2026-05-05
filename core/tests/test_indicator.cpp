#include "data/series.h"
#include "indicator/ma.h"
#include "indicator/ema.h"
#include "indicator/donchian.h"
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

    return failed;
}

#include "data/series.h"
#include "indicator/ma.h"
#include "indicator/ema.h"
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

    return failed;
}

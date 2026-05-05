#include "data/series.h"
#include <cstdio>
#include <cmath>

#define EXPECT(cond) do { if (!(cond)) { std::printf("  FAIL %s:%d\n", __FILE__, __LINE__); ++failed; } } while (0)

int test_series() {
    int failed = 0;
    std::printf("test_series\n");

    tce::Series s;
    TceCandle hist[] = {
        {100.0, 10.0, 12.0,  9.0, 11.0, 100.0},
        {200.0, 11.0, 13.0, 10.0, 10.0,  80.0},
        {300.0, 10.0, 14.0,  9.5, 13.0, 150.0},
    };
    s.setHistory(hist, 3);
    EXPECT(s.size() == 3);
    EXPECT(std::fabs(s.minLowInRange(0, 3) - 9.0)  < 1e-9);
    EXPECT(std::fabs(s.maxHighInRange(0, 3) - 14.0) < 1e-9);
    EXPECT(std::fabs(s.maxVolumeInRange(0, 3) - 150.0) < 1e-9);

    // append 신규
    TceCandle next{400.0, 13.0, 15.0, 12.0, 14.5, 200.0};
    s.append(next);
    EXPECT(s.size() == 4);

    // append 동일 ts → 갱신
    TceCandle revise{400.0, 13.0, 16.0, 12.0, 15.5, 220.0};
    s.append(revise);
    EXPECT(s.size() == 4);
    EXPECT(std::fabs(s.candles().back().high - 16.0) < 1e-9);

    // updateLast: tick 갱신
    s.updateLast(15.7, 230.0);
    EXPECT(std::fabs(s.candles().back().close - 15.7) < 1e-9);
    EXPECT(std::fabs(s.candles().back().volume - 230.0) < 1e-9);

    return failed;
}

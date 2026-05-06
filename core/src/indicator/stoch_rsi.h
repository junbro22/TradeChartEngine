#pragma once

#include "indicator/stochastic.h"  // StochasticResult 재사용

namespace tce {
class Series;

// Stochastic RSI — RSI 시리즈에 stochastic 공식을 적용한 oscillator.
// 표준값: rsiPeriod=14, kPeriod=14, dPeriod=3, smooth=3.
// 0..100 범위, 20/80 가이드선.
StochasticResult stochasticRSI(const Series& series,
                                int rsiPeriod, int kPeriod, int dPeriod, int smooth);

} // namespace tce

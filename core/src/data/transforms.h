#pragma once

#include "tce/types.h"
#include <vector>

namespace tce {
class Series;

// Heikin-Ashi 변환 — 원본 캔들과 같은 길이의 변환 캔들 시리즈
std::vector<TceCandle> toHeikinAshi(const Series& series);

// Renko 변환 — 시간축 무시. brick size로 가격이 변할 때마다 새 캔들.
// 결과 캔들의 timestamp는 발생 시점(원본 캔들 ts)을 그대로 사용.
std::vector<TceCandle> toRenko(const Series& series, double brickSize);

} // namespace tce

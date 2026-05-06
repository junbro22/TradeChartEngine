#pragma once
#include <vector>
#include <optional>

namespace tce {
class Series;

// Session VWAP — 일자가 바뀌면 재시작. price = typical price = (H+L+C)/3.
// @param sessionOffsetSeconds  거래소 세션 시작이 UTC 자정과 어긋날 때 보정.
//   day boundary는 floor((timestamp + sessionOffsetSeconds) / 86400) 기준.
//   KR/JP(KST/JST=UTC+9, 09:00 시작) → 0.
//   NYSE 09:30 EST → -52200, EU CET 09:00 → -28800. (host에서 hour/minute로 입력 권장)
std::vector<std::optional<double>> vwap(const Series& series,
                                         double sessionOffsetSeconds = 0.0);

} // namespace tce

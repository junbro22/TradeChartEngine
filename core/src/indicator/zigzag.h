#pragma once

#include <vector>
#include <optional>

namespace tce {
class Series;

// ZigZag — 마지막 swing high/low로부터 deviationPct(%)만큼 반대 방향으로 움직이면 새 swing.
// 결과는 swing point에서만 값(high 또는 low)을 가진 sparse vector — emitPolyline이 자동으로
// gap을 끊고 두 점을 직선 연결한다.
//
// **repaint 주의**: 마지막 swing은 잠정값. 새 캔들이 들어와 더 큰 극값을 만들면
// 마지막 swing 위치가 변경된다. host가 ZigZag 기반 알림을 만들면 false signal 가능 — README 명시.
std::vector<std::optional<double>> zigzag(const Series& series, double deviationPct);

} // namespace tce

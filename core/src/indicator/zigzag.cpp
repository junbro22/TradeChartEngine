#include "indicator/zigzag.h"
#include "data/series.h"

namespace tce {

std::vector<std::optional<double>> zigzag(const Series& series, double deviationPct) {
    const auto& cs = series.candles();
    std::vector<std::optional<double>> out(cs.size());
    if (cs.empty() || deviationPct <= 0) return out;
    const double thresh = deviationPct / 100.0;

    // direction: +1 = 상승 추세(high를 추적), -1 = 하락 추세(low를 추적), 0 = 미정.
    // 시작 swing은 dir 결정 시점에 [0..i] 윈도우의 반대 극값으로 보정 (표준 ZigZag 일치).
    int dir = 0;
    size_t lastSwingIdx = 0;
    double lastSwingPrice = cs[0].close;
    // 미정 단계 동안 [0..i]의 max high / min low 추적 (인덱스 + 가격)
    size_t maxHighIdx = 0; double maxHigh = cs[0].high;
    size_t minLowIdx  = 0; double minLow  = cs[0].low;

    for (size_t i = 1; i < cs.size(); ++i) {
        const auto& c = cs[i];
        if (dir == 0) {
            // 미정 단계 — 누적 극값 추적
            if (c.high > maxHigh) { maxHigh = c.high; maxHighIdx = i; }
            if (c.low  < minLow)  { minLow  = c.low;  minLowIdx  = i; }
            // deviation 검사 — 0 가격 시리즈에서 division by zero 방지
            if (minLow <= 0 || maxHigh <= 0) continue;
            double upDev = (maxHigh - minLow) / minLow;
            double dnDev = (maxHigh - minLow) / maxHigh;
            if (upDev >= thresh && maxHighIdx > minLowIdx) {
                // minLow가 시작 swing(저점), maxHigh가 첫 swing high
                dir = +1;
                out[minLowIdx] = minLow;
                lastSwingIdx = maxHighIdx;
                lastSwingPrice = maxHigh;
            } else if (dnDev >= thresh && minLowIdx > maxHighIdx) {
                // maxHigh가 시작 swing(고점), minLow가 첫 swing low
                dir = -1;
                out[maxHighIdx] = maxHigh;
                lastSwingIdx = minLowIdx;
                lastSwingPrice = minLow;
            }
        } else if (dir == +1) {
            if (c.high > lastSwingPrice) {
                lastSwingIdx = i;
                lastSwingPrice = c.high;
            } else if ((lastSwingPrice - c.low) / lastSwingPrice >= thresh) {
                // 반전 — 이전 swing high 확정 후 추세 변경
                out[lastSwingIdx] = lastSwingPrice;
                dir = -1;
                lastSwingIdx = i;
                lastSwingPrice = c.low;
            }
        } else { // dir == -1
            if (c.low < lastSwingPrice) {
                lastSwingIdx = i;
                lastSwingPrice = c.low;
            } else if ((c.high - lastSwingPrice) / lastSwingPrice >= thresh) {
                out[lastSwingIdx] = lastSwingPrice;
                dir = +1;
                lastSwingIdx = i;
                lastSwingPrice = c.high;
            }
        }
    }
    // 마지막 잠정 swing point 기록 (다음 캔들에서 갱신될 수 있음)
    if (dir != 0) out[lastSwingIdx] = lastSwingPrice;
    return out;
}

} // namespace tce

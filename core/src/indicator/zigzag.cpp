#include "indicator/zigzag.h"
#include "data/series.h"

namespace tce {

std::vector<std::optional<double>> zigzag(const Series& series, double deviationPct) {
    const auto& cs = series.candles();
    std::vector<std::optional<double>> out(cs.size());
    if (cs.empty() || deviationPct <= 0) return out;
    const double thresh = deviationPct / 100.0;

    // direction: +1 = 상승 추세(high를 추적), -1 = 하락 추세(low를 추적), 0 = 미정.
    int dir = 0;
    size_t lastSwingIdx = 0;
    double lastSwingPrice = cs[0].close;

    for (size_t i = 1; i < cs.size(); ++i) {
        const auto& c = cs[i];
        if (dir == 0) {
            // 첫 deviation 발생 시 방향 결정
            double upDev = (c.high - lastSwingPrice) / lastSwingPrice;
            double dnDev = (lastSwingPrice - c.low)  / lastSwingPrice;
            if (upDev >= thresh) {
                dir = +1;
                out[lastSwingIdx] = lastSwingPrice;  // 시작점(첫 swing low) 기록
                lastSwingIdx = i;
                lastSwingPrice = c.high;
            } else if (dnDev >= thresh) {
                dir = -1;
                out[lastSwingIdx] = lastSwingPrice;
                lastSwingIdx = i;
                lastSwingPrice = c.low;
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

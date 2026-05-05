#include "data/transforms.h"
#include "data/series.h"
#include <algorithm>

namespace tce {

std::vector<TceCandle> toHeikinAshi(const Series& series) {
    const auto& cs = series.candles();
    std::vector<TceCandle> out;
    out.reserve(cs.size());
    if (cs.empty()) return out;

    // 첫 캔들: haOpen = (open + close) / 2
    TceCandle prev = cs[0];
    double haOpen  = (cs[0].open + cs[0].close) / 2.0;
    double haClose = (cs[0].open + cs[0].high + cs[0].low + cs[0].close) / 4.0;
    double haHigh  = std::max({cs[0].high, haOpen, haClose});
    double haLow   = std::min({cs[0].low,  haOpen, haClose});
    out.push_back({prev.timestamp, haOpen, haHigh, haLow, haClose, prev.volume});

    for (size_t i = 1; i < cs.size(); ++i) {
        const auto& c = cs[i];
        const auto& p = out.back();
        double oNew = (p.open + p.close) / 2.0;
        double cNew = (c.open + c.high + c.low + c.close) / 4.0;
        double hNew = std::max({c.high, oNew, cNew});
        double lNew = std::min({c.low,  oNew, cNew});
        out.push_back({c.timestamp, oNew, hNew, lNew, cNew, c.volume});
    }
    return out;
}

std::vector<TceCandle> toRenko(const Series& series, double brickSize) {
    const auto& cs = series.candles();
    std::vector<TceCandle> out;
    if (cs.empty() || brickSize <= 0) return out;

    double base = cs[0].close;
    int direction = 0; // 1=up, -1=down

    for (size_t i = 0; i < cs.size(); ++i) {
        double price = cs[i].close;
        while (true) {
            if (direction >= 0 && price >= base + brickSize) {
                double o = base, c = base + brickSize;
                out.push_back({cs[i].timestamp, o, c, o, c, 0});
                base += brickSize;
                direction = 1;
            } else if (direction <= 0 && price <= base - brickSize) {
                double o = base, c = base - brickSize;
                out.push_back({cs[i].timestamp, o, o, c, c, 0});
                base -= brickSize;
                direction = -1;
            } else if (direction == 1 && price <= base - 2 * brickSize) {
                double o = base - brickSize, c = base - 2 * brickSize;
                out.push_back({cs[i].timestamp, o, o, c, c, 0});
                base -= 2 * brickSize;
                direction = -1;
            } else if (direction == -1 && price >= base + 2 * brickSize) {
                double o = base + brickSize, c = base + 2 * brickSize;
                out.push_back({cs[i].timestamp, o, c, o, c, 0});
                base += 2 * brickSize;
                direction = 1;
            } else {
                break;
            }
        }
    }
    return out;
}

} // namespace tce

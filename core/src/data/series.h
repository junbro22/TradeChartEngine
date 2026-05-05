#pragma once

#include "tce/types.h"
#include <vector>

namespace tce {

class Series {
public:
    void setHistory(const TceCandle* data, size_t count);
    void append(const TceCandle& c);          // 같은 ts면 갱신
    void updateLast(double close, double volume);

    size_t size() const { return candles_.size(); }
    const std::vector<TceCandle>& candles() const { return candles_; }

    // [from, to) 닫힌 시작 / 열린 끝
    double minLowInRange(size_t from, size_t to) const;
    double maxHighInRange(size_t from, size_t to) const;
    double maxVolumeInRange(size_t from, size_t to) const;

private:
    std::vector<TceCandle> candles_;
};

} // namespace tce

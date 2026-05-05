#pragma once

#include "tce/types.h"
#include <vector>
#include <optional>

namespace tce {

class Series;

// Simple Moving Average — period 미만 인덱스는 nullopt
std::vector<std::optional<double>> sma(const Series& series, int period);

} // namespace tce

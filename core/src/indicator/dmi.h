#pragma once
#include <vector>
#include <optional>

namespace tce {
class Series;

struct DmiResult {
    std::vector<std::optional<double>> plusDI;   // +DI
    std::vector<std::optional<double>> minusDI;  // -DI
    std::vector<std::optional<double>> adx;      // ADX = SMA of DX
};

// Wilder DMI/ADX (period=14)
DmiResult dmi(const Series& series, int period = 14);

} // namespace tce

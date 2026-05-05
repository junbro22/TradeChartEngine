#pragma once
#include <vector>
#include <optional>

namespace tce {
class Series;

// Pivot Point 종류
enum class PivotKind {
    Standard,
    Fibonacci,
    Camarilla,
};

// 한 거래일의 피벗 7개 (R3/R2/R1/P/S1/S2/S3)
struct PivotResult {
    std::vector<std::optional<double>> p;
    std::vector<std::optional<double>> r1;
    std::vector<std::optional<double>> r2;
    std::vector<std::optional<double>> r3;
    std::vector<std::optional<double>> s1;
    std::vector<std::optional<double>> s2;
    std::vector<std::optional<double>> s3;
};

// 직전 거래일 H/L/C로부터 당일 모든 캔들 인덱스에 같은 7개 값 채움.
// 첫 거래일은 nullopt.
PivotResult pivot(const Series& series, PivotKind kind);

} // namespace tce

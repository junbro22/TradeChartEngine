#pragma once

#include "tce/types.h"
#include <vector>

namespace tce {

class Series;
class Viewport;

struct IndicatorSpec {
    TceIndicatorKind kind;
    int              period;
    TceColor         color;
};

struct CrosshairState {
    bool   visible = false;
    float  x = 0.0f;
    float  y = 0.0f;
    int    candleIndex = -1;
    double price = 0.0;
    double timestamp = 0.0;
};

struct ChartConfig {
    TceSeriesType  seriesType    = TCE_SERIES_CANDLE;
    TceColorScheme scheme        = TCE_SCHEME_KOREA;
    bool           volumeVisible = true;
};

// 1프레임 빌드 결과 — vertex/index 전용 메모리 보관.
// 호스트로 반환되는 TceFrame은 이 객체를 가리킨다.
class FrameOutput {
public:
    FrameOutput() = default;
    FrameOutput(const FrameOutput&) = delete;
    FrameOutput& operator=(const FrameOutput&) = delete;

    void clear();
    void addMesh(std::vector<TceVertex>&& verts,
                 std::vector<uint32_t>&&  idx,
                 int primitive);

    // C ABI 노출용 — 메시별 헤더 배열을 만들어 둠
    const std::vector<TceMesh>& meshHeaders() const { return meshHeaders_; }

private:
    void rebuildHeaders_();
    std::vector<std::vector<TceVertex>> vbufs_;
    std::vector<std::vector<uint32_t>>  ibufs_;
    std::vector<int>                    primitives_;
    std::vector<TceMesh>                meshHeaders_;
};

class FrameBuilder {
public:
    void build(const Series& series,
               const Viewport& viewport,
               const ChartConfig& config,
               const std::vector<IndicatorSpec>& indicators,
               const CrosshairState& crosshair,
               FrameOutput& out);
};

} // namespace tce

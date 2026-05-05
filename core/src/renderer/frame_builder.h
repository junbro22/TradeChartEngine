#pragma once

#include "tce/types.h"
#include <vector>

namespace tce {

class Series;
class Viewport;

// Overlay (메인 패널 위) 지표
struct OverlaySpec {
    TceIndicatorKind kind;       // SMA / EMA / BOLLINGER
    int              period = 0;
    double           param  = 2.0; // BB stddev (기본 2.0)
    TceColor         color  = {1, 1, 1, 1};
    TceColor         color2 = {1, 1, 1, 0.5}; // BB lower
};

// Subpanel (하단 별도 패널) 지표
struct SubpanelSpec {
    TceIndicatorKind kind;       // RSI / MACD / STOCHASTIC / ATR
    int              p1 = 14;    // primary period (RSI/ATR period, MACD fast, Stoch kPeriod)
    int              p2 = 0;     // MACD slow, Stoch dPeriod
    int              p3 = 0;     // MACD signal, Stoch smooth
    TceColor         color1 = {1, 1, 1, 1};
    TceColor         color2 = {1, 1, 1, 0.6};   // MACD signal, Stoch %D
    TceColor         color3 = {0.5, 0.5, 0.5, 0.4}; // MACD histogram
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

class FrameOutput {
public:
    FrameOutput() = default;
    FrameOutput(const FrameOutput&) = delete;
    FrameOutput& operator=(const FrameOutput&) = delete;

    void clear();
    void addMesh(std::vector<TceVertex>&& verts,
                 std::vector<uint32_t>&&  idx,
                 int primitive);

    const std::vector<TceMesh>& meshHeaders() const { return meshHeaders_; }

private:
    void rebuildHeaders_();
    std::vector<std::vector<TceVertex>> vbufs_;
    std::vector<std::vector<uint32_t>>  ibufs_;
    std::vector<int>                    primitives_;
    std::vector<TceMesh>                meshHeaders_;
};

// 픽셀 사각형
struct Rect {
    float x = 0;
    float y = 0;
    float w = 0;
    float h = 0;
};

// frame 빌드 후 레이아웃·가격 범위를 외부에 노출 (label_builder/wrapper가 사용)
struct PanelLayout {
    Rect   plot;              // 캔들이 그려지는 메인 영역 (axis 제외)
    Rect   priceAxis;         // 우측 가격축 라벨 영역
    Rect   timeAxis;          // 하단 시간축 라벨 영역
    Rect   volumePanel;       // 거래량 패널 (없으면 w=0)
    std::vector<Rect> subpanels; // 보조 패널들 (RSI/MACD/Stoch/ATR 등)
    double priceMin = 0;
    double priceMax = 0;
};

class FrameBuilder {
public:
    void build(const Series& series,
               const Viewport& viewport,
               const ChartConfig& config,
               const std::vector<OverlaySpec>& overlays,
               const std::vector<SubpanelSpec>& subpanels,
               const CrosshairState& crosshair,
               FrameOutput& out,
               PanelLayout& layout);
};

} // namespace tce

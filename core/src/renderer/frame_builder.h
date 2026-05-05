#pragma once

#include "tce/types.h"
#include <cmath>
#include <vector>

namespace tce {

class Series;
class Viewport;

// Overlay (메인 패널 위) 지표
struct OverlaySpec {
    TceIndicatorKind kind;       // SMA / EMA / BOLLINGER / ICHIMOKU / PSAR / SUPERTREND / VWAP / PIVOT_*
    int              period = 0; // SMA/EMA/BB period, Ichimoku tenkan, SuperTrend period
    int              p2     = 0; // Ichimoku kijun
    int              p3     = 0; // Ichimoku senkouB
    int              p4     = 0; // Ichimoku displacement
    double           param  = 2.0; // BB stddev / PSAR step / SuperTrend multiplier
    double           param2 = 0.2; // PSAR maxStep
    TceColor         color  = {1, 1, 1, 1};
    TceColor         color2 = {1, 1, 1, 0.5}; // BB lower / Ichimoku kijun / Pivot R-S
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
    TceSeriesType    seriesType    = TCE_SERIES_CANDLE;
    TceColorScheme   scheme        = TCE_SCHEME_KOREA;
    bool             volumeVisible = true;
    TcePriceAxisMode priceMode     = TCE_PRICE_LINEAR;
    double           renkoBrickSize = 0.0;   // 0이면 자동 (마지막 close × 0.5%)
    bool             showGrid      = true;
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
//
// 가격 좌표 정책:
//   priceMin/priceMax는 **정규화된(normalized)** 값이다.
//     LINEAR  → raw price 그대로
//     LOG     → log(raw price)
//     PERCENT → (raw / percentBase - 1) * 100
//   raw price ↔ screen Y 변환은 아래 free 함수(priceToY/yToPrice)를 통해서만 한다.
struct PanelLayout {
    Rect   plot;              // 캔들이 그려지는 메인 영역 (axis 제외)
    Rect   priceAxis;         // 우측 가격축 라벨 영역
    Rect   timeAxis;          // 하단 시간축 라벨 영역
    Rect   volumePanel;       // 거래량 패널 (없으면 w=0)
    std::vector<Rect> subpanels; // 보조 패널들 (RSI/MACD/Stoch/ATR 등)
    double priceMin = 0;        // normalized
    double priceMax = 0;        // normalized
    TcePriceAxisMode priceMode = TCE_PRICE_LINEAR;
    double percentBase = 0.0;   // PERCENT 모드의 기준가 (0이면 PERCENT 비활성)
};

inline double layoutNormalizePrice(const PanelLayout& l, double rawPrice) {
    switch (l.priceMode) {
    case TCE_PRICE_LOG:
        return rawPrice > 0 ? std::log(rawPrice) : 0.0;
    case TCE_PRICE_PERCENT:
        return (l.percentBase > 0) ? (rawPrice / l.percentBase - 1.0) * 100.0 : 0.0;
    default:
        return rawPrice;
    }
}

inline double layoutDenormalizePrice(const PanelLayout& l, double normalized) {
    switch (l.priceMode) {
    case TCE_PRICE_LOG:
        return std::exp(normalized);
    case TCE_PRICE_PERCENT:
        return (l.percentBase > 0) ? l.percentBase * (1.0 + normalized / 100.0)
                                   : normalized;
    default:
        return normalized;
    }
}

inline float layoutPriceToY(const PanelLayout& l, double rawPrice) {
    const float top = l.plot.y;
    const float bot = l.plot.y + l.plot.h;
    if (l.priceMax == l.priceMin) return (top + bot) * 0.5f;
    double nv = layoutNormalizePrice(l, rawPrice);
    double t = (nv - l.priceMin) / (l.priceMax - l.priceMin);
    return static_cast<float>(bot - t * (bot - top));
}

inline double layoutYToPrice(const PanelLayout& l, float screenY) {
    const float top = l.plot.y;
    const float bot = l.plot.y + l.plot.h;
    if (l.priceMax == l.priceMin) return layoutDenormalizePrice(l, l.priceMin);
    if (bot <= top) return layoutDenormalizePrice(l, l.priceMin);
    double t = ((double)bot - (double)screenY) / ((double)bot - (double)top);
    double nv = l.priceMin + t * (l.priceMax - l.priceMin);
    return layoutDenormalizePrice(l, nv);
}

class FrameBuilder {
public:
    /// @param drawSeries       메인 패널에 그릴 시리즈 (HA/Renko면 변환된 것)
    /// @param indicatorSeries  지표 계산에 사용할 시리즈. nullptr이면 drawSeries와 동일.
    ///                         HA 모드에선 원본 series를 넘겨 RSI/MACD 등이 원본 OHLC로 계산되도록 한다.
    void build(const Series& drawSeries,
               const Viewport& viewport,
               const ChartConfig& config,
               const std::vector<OverlaySpec>& overlays,
               const std::vector<SubpanelSpec>& subpanels,
               const CrosshairState& crosshair,
               FrameOutput& out,
               PanelLayout& layout,
               const Series* indicatorSeries = nullptr);
};

} // namespace tce

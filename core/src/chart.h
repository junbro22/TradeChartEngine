#pragma once

#include "tce/types.h"
#include "data/series.h"
#include "viewport/viewport.h"
#include "renderer/frame_builder.h"
#include <vector>

namespace tce {

class Chart {
public:
    Chart();

    // data
    void setHistory(const TceCandle* data, size_t count);
    void appendCandle(const TceCandle& c);
    void updateLast(double close, double volume);
    size_t candleCount() const { return series_.size(); }

    // config
    void setSeriesType(TceSeriesType t)       { config_.seriesType = t; }
    void setColorScheme(TceColorScheme s)     { config_.scheme = s; }
    void setVolumeVisible(bool v)             { config_.volumeVisible = v; }
    void setSize(float w, float h)            { viewport_.setSize(w, h); }

    // overlay 지표 (메인 패널)
    void addOverlay(TceIndicatorKind k, int period, double param,
                    TceColor color, TceColor color2 = {1, 1, 1, 0.5});
    void removeOverlay(TceIndicatorKind k, int period);
    void clearOverlays();

    // subpanel 지표 (보조 패널)
    void addSubpanel(TceIndicatorKind k, int p1, int p2, int p3,
                     TceColor color1,
                     TceColor color2 = {1, 1, 1, 0.6},
                     TceColor color3 = {0.5f, 0.5f, 0.5f, 0.4f});
    void removeSubpanel(TceIndicatorKind k);
    void clearSubpanels();

    void clearAllIndicators() { clearOverlays(); clearSubpanels(); }

    // viewport
    Viewport& viewport()             { return viewport_; }
    const Viewport& viewport() const { return viewport_; }

    // crosshair
    void setCrosshair(float x, float y);
    void clearCrosshair();
    const CrosshairState& crosshair() const { return crosshair_; }

    // render
    FrameOutput& buildFrame();

private:
    Series                       series_;
    Viewport                     viewport_;
    ChartConfig                  config_;
    std::vector<OverlaySpec>     overlays_;
    std::vector<SubpanelSpec>    subpanels_;
    CrosshairState               crosshair_;
    FrameOutput                  output_;
    FrameBuilder                 builder_;
};

} // namespace tce

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

    // indicators
    void addIndicator(TceIndicatorKind k, int period, TceColor c);
    void removeIndicator(TceIndicatorKind k, int period);
    void clearIndicators();

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
    std::vector<IndicatorSpec>   indicators_;
    CrosshairState               crosshair_;
    FrameOutput                  output_;
    FrameBuilder                 builder_;
};

} // namespace tce

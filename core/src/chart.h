#pragma once

#include "tce/types.h"
#include "data/series.h"
#include "viewport/viewport.h"
#include "renderer/frame_builder.h"
#include "renderer/label_builder.h"
#include "drawing/drawing.h"
#include "drawing/drawing_renderer.h"
#include "overlay/trade_markers.h"
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
    bool   getCandle(size_t index, TceCandle& out) const {
        const auto& cs = series_.candles();
        if (index >= cs.size()) return false;
        out = cs[index]; return true;
    }

    // viewport 헬퍼
    void resetViewport()  { viewport_.setRightOffset(0); }
    void fitAll() {
        const size_t n = series_.size();
        if (n > 0) viewport_.setVisibleCount(static_cast<int>(n));
        viewport_.setRightOffset(0);
    }

    // config
    void setSeriesType(TceSeriesType t)       { config_.seriesType = t; }
    void setColorScheme(TceColorScheme s)     { config_.scheme = s; }
    void setVolumeVisible(bool v)             { config_.volumeVisible = v; }
    void setSize(float w, float h)            { viewport_.setSize(w, h); }
    void setPriceAxisMode(TcePriceAxisMode m) { config_.priceMode = m; }
    void setRenkoBrickSize(double v)          { config_.renkoBrickSize = v; }
    void setShowGrid(bool v)                  { config_.showGrid = v; }

    // overlay 지표 (메인 패널)
    void addOverlay(TceIndicatorKind k, int period, double param,
                    TceColor color, TceColor color2 = {1, 1, 1, 0.5});
    // 다중 파라미터 overlay (Ichimoku 4 ints, PSAR step+maxStep, SuperTrend period+multiplier)
    void addOverlayEx(TceIndicatorKind k,
                      int period, int p2, int p3, int p4,
                      double param, double param2,
                      TceColor color, TceColor color2 = {1, 1, 1, 0.5});
    void removeOverlay(TceIndicatorKind k, int period);
    void clearOverlays();

    // subpanel 지표 (보조 패널)
    void addSubpanel(TceIndicatorKind k, int p1, int p2, int p3,
                     TceColor color1,
                     TceColor color2 = {1, 1, 1, 0.6},
                     TceColor color3 = {0.5f, 0.5f, 0.5f, 0.4f});
    /// kind 일치 + (period<=0 무관 OR p1==period 일치)인 첫 spec 1개 제거.
    /// period<=0이면 같은 kind의 모든 spec 제거.
    void removeSubpanel(TceIndicatorKind k, int period = 0);
    void clearSubpanels();

    void clearAllIndicators() { clearOverlays(); clearSubpanels(); }

    // viewport
    Viewport& viewport()             { return viewport_; }
    const Viewport& viewport() const { return viewport_; }

    // crosshair
    void setCrosshair(float x, float y);
    void clearCrosshair();
    const CrosshairState& crosshair() const { return crosshair_; }

    // auto-scroll
    void setAutoScroll(bool v) { autoScroll_ = v; }
    bool autoScroll() const    { return autoScroll_; }

    // render
    FrameOutput& buildFrame();
    LabelOutput& buildLabels();   // buildFrame 직후 호출 (같은 layout 사용)
    const PanelLayout& layout() const;

    // 인터랙션 — wrapper는 raw px만 dispatch
    void applyPinch(float scale, float anchorPx);
    void applyPan(float dxPx);

    // 화면 ↔ 도메인 좌표 변환 (lastLayout 기반 — buildFrame 호출 후만 의미)
    double screenToTimestamp(float screenX) const;
    double screenToPrice(float screenY) const;
    /// 화면 X → 캔들 인덱스. plot 영역 밖이면 -1.
    int    screenXToIndex(float screenX) const;
    /// 캔들 인덱스 → 화면 X 중심. 가시 viewport 밖이면 false 반환 (out_x 비변경).
    bool   indexToScreenX(int index, float& out_x) const;
    /// 가격 → 화면 Y. plot 높이 0이면 (top+bot)/2.
    float  priceToScreenY(double price) const;

    // 드로잉 — 사용자가 raw px로 입력. 엔진이 도메인 좌표로 저장.
    int  beginDrawing(TceDrawingKind kind, float screenX, float screenY, TceColor color);
    void updateDrawing(int id, size_t pointIdx, float screenX, float screenY);
    void removeDrawing(int id);
    void clearDrawings();
    const std::vector<Drawing>& drawings() const { return drawings_.all(); }
    // 화면 px 위치에서 가장 가까운 드로잉 id (없으면 0)
    int  hitTestDrawing(float screenX, float screenY, float toleranceePx = 12.0f) const;
    // 드로잉 통째 이동 (모든 점이 동일 dx/dy만큼)
    void translateDrawing(int id, float dxPx, float dyPx);

    // 매수/매도 마커
    int  addTradeMarker(double timestamp, double price, bool isBuy, double quantity);
    void removeTradeMarker(int id);
    void clearTradeMarkers();

    // 가격 알림선
    int  addAlertLine(double price, TceColor color);
    void updateAlertLineByScreen(int id, float screenY);
    void removeAlertLine(int id);
    void clearAlertLines();
    int  hitTestAlertLine(float screenY, float toleranceePx = 14.0f) const;

    /// 알림선 cross 콜백 — append/updateLast 시 prev_close ↔ new_close 사이에
    /// alert.price가 있으면 fire. host가 푸시 노티/사운드 트리거.
    using AlertCrossFn = void(*)(int alert_id, double cross_price, void* user);
    void setAlertCallback(AlertCrossFn cb, void* user) { alertCb_ = cb; alertUser_ = user; }

private:
    Series                       series_;
    Viewport                     viewport_;
    ChartConfig                  config_;
    std::vector<OverlaySpec>     overlays_;
    std::vector<SubpanelSpec>    subpanels_;
    CrosshairState               crosshair_;
    FrameOutput                  output_;
    FrameBuilder                 builder_;
    LabelOutput                  labelOutput_;
    LabelBuilder                 labelBuilder_;
    PanelLayout                  lastLayout_;
    DrawingStore                 drawings_;
    DrawingRenderer              drawingRenderer_;
    TradeMarkerStore             markers_;
    AlertLineStore               alerts_;
    MarkerRenderer               markerRenderer_;
    bool                         autoScroll_ = true;

    AlertCrossFn                 alertCb_ = nullptr;
    void*                        alertUser_ = nullptr;
    void                         fireAlertCrossings_(double prevClose, double newClose);
};

} // namespace tce

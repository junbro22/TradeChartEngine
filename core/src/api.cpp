// extern "C" 어댑터 — C++ Chart 클래스를 C ABI로 노출
#include "tce/tce.h"
#include "chart.h"
#include <algorithm>
#include <new>

struct TceContext {
    tce::Chart chart;
};

extern "C" {

TceContext* tce_create(void) {
    return new (std::nothrow) TceContext{};
}

void tce_destroy(TceContext* ctx) {
    delete ctx;
}

const char* tce_version(void) {
    return "0.1.0";
}

void tce_set_history(TceContext* ctx, const TceCandle* candles, size_t count) {
    if (!ctx || !candles) return;
    ctx->chart.setHistory(candles, count);
}

void tce_append_candle(TceContext* ctx, const TceCandle* candle) {
    if (!ctx || !candle) return;
    ctx->chart.appendCandle(*candle);
}

void tce_update_last(TceContext* ctx, double close, double volume) {
    if (!ctx) return;
    ctx->chart.updateLast(close, volume);
}

size_t tce_candle_count(const TceContext* ctx) {
    return ctx ? ctx->chart.candleCount() : 0;
}

void tce_set_series_type(TceContext* ctx, TceSeriesType type) {
    if (ctx) ctx->chart.setSeriesType(type);
}

void tce_set_color_scheme(TceContext* ctx, TceColorScheme scheme) {
    if (ctx) ctx->chart.setColorScheme(scheme);
}

void tce_set_size(TceContext* ctx, float w, float h) {
    if (ctx) ctx->chart.setSize(w, h);
}

void tce_set_volume_panel_visible(TceContext* ctx, int visible) {
    if (ctx) ctx->chart.setVolumeVisible(visible != 0);
}

void tce_add_indicator(TceContext* ctx, TceIndicatorKind kind, int period, TceColor color) {
    if (!ctx) return;
    if (kind == TCE_IND_SMA || kind == TCE_IND_EMA) {
        ctx->chart.addOverlay(kind, period, 0.0, color);
    }
}

void tce_remove_indicator(TceContext* ctx, TceIndicatorKind kind, int period) {
    if (!ctx) return;
    ctx->chart.removeOverlay(kind, period);
    ctx->chart.removeSubpanel(kind);
}

void tce_clear_indicators(TceContext* ctx) {
    if (ctx) ctx->chart.clearAllIndicators();
}

void tce_add_bollinger(TceContext* ctx, int period, double stddev, TceColor color) {
    if (!ctx) return;
    TceColor edge{color.r, color.g, color.b, 0.5f};
    ctx->chart.addOverlay(TCE_IND_BOLLINGER, period, stddev, color, edge);
}

void tce_add_rsi(TceContext* ctx, int period, TceColor color) {
    if (!ctx) return;
    ctx->chart.addSubpanel(TCE_IND_RSI, period, 0, 0, color);
}

void tce_add_macd(TceContext* ctx, int fast, int slow, int signal,
                  TceColor lineColor, TceColor signalColor, TceColor histColor) {
    if (!ctx) return;
    ctx->chart.addSubpanel(TCE_IND_MACD, fast, slow, signal, lineColor, signalColor, histColor);
}

void tce_add_stochastic(TceContext* ctx, int kPeriod, int dPeriod, int smooth,
                        TceColor kColor, TceColor dColor) {
    if (!ctx) return;
    ctx->chart.addSubpanel(TCE_IND_STOCHASTIC, kPeriod, dPeriod, smooth, kColor, dColor);
}

void tce_add_atr(TceContext* ctx, int period, TceColor color) {
    if (!ctx) return;
    ctx->chart.addSubpanel(TCE_IND_ATR, period, 0, 0, color);
}

void tce_add_ichimoku(TceContext* ctx, int tenkan, int kijun, int /*senkouB*/, int /*displacement*/,
                      TceColor tenkanColor, TceColor kijunColor) {
    if (!ctx) return;
    // 현재 frame_builder는 (9, 26, 52, 26) 고정. 향후 OverlaySpec 확장 시 사용.
    ctx->chart.addOverlay(TCE_IND_ICHIMOKU, tenkan > 0 ? tenkan : 9, kijun > 0 ? kijun : 26,
                          tenkanColor, kijunColor);
}

void tce_add_psar(TceContext* ctx, double step, double maxStep, TceColor color) {
    if (!ctx) return;
    (void)maxStep; // OverlaySpec 한계 — 향후 확장
    ctx->chart.addOverlay(TCE_IND_PSAR, 0, step > 0 ? step : 0.02, color);
}

void tce_add_supertrend(TceContext* ctx, int period, double multiplier, TceColor color) {
    if (!ctx) return;
    ctx->chart.addOverlay(TCE_IND_SUPERTREND, period > 0 ? period : 10,
                          multiplier > 0 ? multiplier : 3.0, color);
}

void tce_add_vwap(TceContext* ctx, TceColor color) {
    if (!ctx) return;
    ctx->chart.addOverlay(TCE_IND_VWAP, 0, 0.0, color);
}

void tce_add_dmi(TceContext* ctx, int period,
                 TceColor plusDIColor, TceColor minusDIColor, TceColor adxColor) {
    if (!ctx) return;
    ctx->chart.addSubpanel(TCE_IND_DMI_ADX, period, 0, 0,
                           plusDIColor, minusDIColor, adxColor);
}

void tce_add_cci(TceContext* ctx, int period, TceColor color) {
    if (!ctx) return;
    ctx->chart.addSubpanel(TCE_IND_CCI, period, 0, 0, color);
}

void tce_add_williams_r(TceContext* ctx, int period, TceColor color) {
    if (!ctx) return;
    ctx->chart.addSubpanel(TCE_IND_WILLIAMS_R, period, 0, 0, color);
}

void tce_add_obv(TceContext* ctx, TceColor color) {
    if (!ctx) return;
    ctx->chart.addSubpanel(TCE_IND_OBV, 0, 0, 0, color);
}

void tce_add_mfi(TceContext* ctx, int period, TceColor color) {
    if (!ctx) return;
    ctx->chart.addSubpanel(TCE_IND_MFI, period, 0, 0, color);
}

void tce_add_pivot_standard(TceContext* ctx, TceColor pColor, TceColor rsColor) {
    if (!ctx) return;
    ctx->chart.addOverlay(TCE_IND_PIVOT_STANDARD, 0, 0.0, pColor, rsColor);
}

void tce_add_pivot_fibonacci(TceContext* ctx, TceColor pColor, TceColor rsColor) {
    if (!ctx) return;
    ctx->chart.addOverlay(TCE_IND_PIVOT_FIBONACCI, 0, 0.0, pColor, rsColor);
}

void tce_add_pivot_camarilla(TceContext* ctx, TceColor pColor, TceColor rsColor) {
    if (!ctx) return;
    ctx->chart.addOverlay(TCE_IND_PIVOT_CAMARILLA, 0, 0.0, pColor, rsColor);
}

void tce_set_visible_count(TceContext* ctx, int count) {
    if (ctx) ctx->chart.viewport().setVisibleCount(count);
}

int tce_visible_count(const TceContext* ctx) {
    return ctx ? ctx->chart.viewport().visibleCount() : 0;
}

void tce_set_right_offset(TceContext* ctx, int offset) {
    if (ctx) ctx->chart.viewport().setRightOffset(offset);
}

int tce_right_offset(const TceContext* ctx) {
    return ctx ? ctx->chart.viewport().rightOffset() : 0;
}

void tce_pan(TceContext* ctx, float dx) {
    if (ctx) ctx->chart.viewport().pan(dx);
}

void tce_zoom(TceContext* ctx, float factor, float anchor_x) {
    if (ctx) ctx->chart.viewport().zoom(factor, anchor_x);
}

void tce_set_crosshair(TceContext* ctx, float x, float y) {
    if (ctx) ctx->chart.setCrosshair(x, y);
}

void tce_clear_crosshair(TceContext* ctx) {
    if (ctx) ctx->chart.clearCrosshair();
}

TceCrosshairInfo tce_crosshair_info(const TceContext* ctx) {
    TceCrosshairInfo info{};
    if (!ctx) return info;
    const auto& c = ctx->chart.crosshair();
    info.visible = c.visible ? 1 : 0;
    info.candle_index = c.candleIndex;
    info.price = c.price;
    info.timestamp = c.timestamp;
    info.screen_x = c.x;
    info.screen_y = c.y;
    return info;
}

void tce_set_auto_scroll(TceContext* ctx, int enabled) {
    if (ctx) ctx->chart.setAutoScroll(enabled != 0);
}

int tce_auto_scroll(const TceContext* ctx) {
    return (ctx && ctx->chart.autoScroll()) ? 1 : 0;
}

void tce_apply_pinch(TceContext* ctx, float scale, float anchor_px) {
    if (ctx) ctx->chart.applyPinch(scale, anchor_px);
}

void tce_apply_pan(TceContext* ctx, float dx_px) {
    if (ctx) ctx->chart.applyPan(dx_px);
}

TceLayout tce_layout(const TceContext* ctx) {
    TceLayout out{};
    if (!ctx) return out;
    const auto& l = ctx->chart.layout();
    auto toC = [](const tce::Rect& r) {
        return TceRect{r.x, r.y, r.w, r.h};
    };
    out.plot         = toC(l.plot);
    out.priceAxis    = toC(l.priceAxis);
    out.timeAxis     = toC(l.timeAxis);
    out.volumePanel  = toC(l.volumePanel);
    out.subpanelCount = static_cast<int>(std::min<size_t>(l.subpanels.size(), 8));
    for (int i = 0; i < out.subpanelCount; ++i) {
        out.subpanels[i] = toC(l.subpanels[i]);
    }
    return out;
}

TceLabels tce_build_labels(TceContext* ctx) {
    TceLabels out{};
    if (!ctx) return out;
    auto& labels = ctx->chart.buildLabels();
    const auto& v = labels.items();
    out.items = v.data();
    out.count = v.size();
    return out;
}

TceFrame tce_build_frame(TceContext* ctx) {
    TceFrame f{};
    if (!ctx) return f;
    auto& out = ctx->chart.buildFrame();
    const auto& meshes = out.meshHeaders();
    f.meshes      = meshes.data();
    f.mesh_count  = meshes.size();
    f.internal    = nullptr;     // 메모리는 ctx 내부에서 소유, release는 no-op
    return f;
}

void tce_release_frame(TceFrame /*frame*/) {
    // 현재 구현은 ctx 내부 FrameOutput이 메모리 소유 → 별도 release 불필요.
    // 향후 thread-safe / async 빌드 시 internal 포인터로 관리.
}

} // extern "C"

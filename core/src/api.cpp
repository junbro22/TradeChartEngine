// extern "C" 어댑터 — C++ Chart 클래스를 C ABI로 노출
#include "tce/tce.h"
#include "chart.h"
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
    if (ctx) ctx->chart.addIndicator(kind, period, color);
}

void tce_remove_indicator(TceContext* ctx, TceIndicatorKind kind, int period) {
    if (ctx) ctx->chart.removeIndicator(kind, period);
}

void tce_clear_indicators(TceContext* ctx) {
    if (ctx) ctx->chart.clearIndicators();
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

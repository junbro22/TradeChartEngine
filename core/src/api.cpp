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
    if (!ctx) return;
    // 콜백 user 포인터가 dangling되지 않도록 명시 nullify (방어 코드).
    // host가 이미 set_alert_callback(NULL)을 호출했어도 안전 — 이중 nullify는 무해.
    ctx->chart.setAlertCallback(nullptr, nullptr);
    delete ctx;
}

const char* tce_version(void) {
    return "0.11.0";
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

int tce_get_candle(const TceContext* ctx, size_t index, TceCandle* out) {
    if (!ctx || !out) return 0;
    return ctx->chart.getCandle(index, *out) ? 1 : 0;
}

void tce_reset_viewport(TceContext* ctx) {
    if (ctx) ctx->chart.resetViewport();
}

void tce_fit_all(TceContext* ctx) {
    if (ctx) ctx->chart.fitAll();
}

int tce_screen_x_to_index(const TceContext* ctx, float screen_x) {
    if (!ctx) return -1;
    return ctx->chart.screenXToIndex(screen_x);
}

int tce_index_to_screen_x(const TceContext* ctx, int index, float* out_x) {
    if (!ctx || !out_x) return 0;
    float x = 0;
    if (!ctx->chart.indexToScreenX(index, x)) return 0;
    *out_x = x;
    return 1;
}

double tce_screen_y_to_price(const TceContext* ctx, float screen_y) {
    if (!ctx) return 0.0;
    return ctx->chart.screenToPrice(screen_y);
}

float tce_price_to_screen_y(const TceContext* ctx, double price) {
    if (!ctx) return 0.0f;
    return ctx->chart.priceToScreenY(price);
}

int tce_query_indicator_value(const TceContext* ctx, TceIndicatorKind kind, int period,
                              size_t idx, double* out) {
    if (!ctx || !out) return 0;
    double v = 0;
    if (!ctx->chart.queryIndicatorValue(kind, period, idx, v)) return 0;
    *out = v;
    return 1;
}

int tce_query_bollinger(const TceContext* ctx, int period, size_t idx,
                        double* upper, double* middle, double* lower) {
    if (!ctx || !upper || !middle || !lower) return 0;
    double u, m, l;
    if (!ctx->chart.queryBollinger(period, idx, u, m, l)) return 0;
    *upper = u; *middle = m; *lower = l;
    return 1;
}

int tce_query_donchian(const TceContext* ctx, int period, size_t idx,
                       double* upper, double* middle, double* lower) {
    if (!ctx || !upper || !middle || !lower) return 0;
    double u, m, l;
    if (!ctx->chart.queryDonchian(period, idx, u, m, l)) return 0;
    *upper = u; *middle = m; *lower = l;
    return 1;
}

int tce_query_keltner(const TceContext* ctx, int emaPeriod, size_t idx,
                      double* upper, double* middle, double* lower) {
    if (!ctx || !upper || !middle || !lower) return 0;
    double u, m, l;
    if (!ctx->chart.queryKeltner(emaPeriod, idx, u, m, l)) return 0;
    *upper = u; *middle = m; *lower = l;
    return 1;
}

int tce_query_macd(const TceContext* ctx, size_t idx,
                   double* line, double* signal, double* hist) {
    if (!ctx || !line || !signal || !hist) return 0;
    double a, b, c;
    if (!ctx->chart.queryMACD(idx, a, b, c)) return 0;
    *line = a; *signal = b; *hist = c;
    return 1;
}

int tce_query_stochastic(const TceContext* ctx, size_t idx, double* k, double* d) {
    if (!ctx || !k || !d) return 0;
    double a, b;
    if (!ctx->chart.queryStochastic(idx, a, b)) return 0;
    *k = a; *d = b;
    return 1;
}

int tce_query_dmi(const TceContext* ctx, int period, size_t idx,
                  double* plusDI, double* minusDI, double* adx) {
    if (!ctx || !plusDI || !minusDI || !adx) return 0;
    double a, b, c;
    if (!ctx->chart.queryDMI(period, idx, a, b, c)) return 0;
    *plusDI = a; *minusDI = b; *adx = c;
    return 1;
}

int tce_query_pivot(const TceContext* ctx, TceIndicatorKind kind, size_t idx,
                    double* p, double* r1, double* r2, double* r3,
                    double* s1, double* s2, double* s3) {
    if (!ctx || !p || !r1 || !r2 || !r3 || !s1 || !s2 || !s3) return 0;
    double pp, rr1, rr2, rr3, ss1, ss2, ss3;
    if (!ctx->chart.queryPivot(kind, idx, pp, rr1, rr2, rr3, ss1, ss2, ss3)) return 0;
    *p = pp; *r1 = rr1; *r2 = rr2; *r3 = rr3;
    *s1 = ss1; *s2 = ss2; *s3 = ss3;
    return 1;
}

int tce_query_ichimoku(const TceContext* ctx, size_t idx,
                       double* tenkan, double* kijun,
                       double* senkouA, double* senkouB, double* chikou) {
    if (!ctx || !tenkan || !kijun || !senkouA || !senkouB || !chikou) return 0;
    double t, k, sA, sB, ch;
    if (!ctx->chart.queryIchimoku(idx, t, k, sA, sB, ch)) return 0;
    *tenkan = t; *kijun = k; *senkouA = sA; *senkouB = sB; *chikou = ch;
    return 1;
}

int tce_query_supertrend(const TceContext* ctx, size_t idx,
                         double* line, int* direction) {
    if (!ctx || !line || !direction) return 0;
    double l; int d;
    if (!ctx->chart.querySuperTrend(idx, l, d)) return 0;
    *line = l; *direction = d;
    return 1;
}

int tce_query_vwap_bands(const TceContext* ctx, size_t idx,
                         double* middle, double* upper, double* lower) {
    if (!ctx || !middle || !upper || !lower) return 0;
    double m, u, l;
    if (!ctx->chart.queryVWAPBands(idx, m, u, l)) return 0;
    *middle = m; *upper = u; *lower = l;
    return 1;
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

void tce_set_price_axis_mode(TceContext* ctx, TcePriceAxisMode mode) {
    if (ctx) ctx->chart.setPriceAxisMode(mode);
}

void tce_set_renko_brick_size(TceContext* ctx, double size) {
    if (ctx) ctx->chart.setRenkoBrickSize(size);
}

void tce_set_show_grid(TceContext* ctx, int show) {
    if (ctx) ctx->chart.setShowGrid(show != 0);
}

void tce_set_session_start_utc(TceContext* ctx, int hour, int minute) {
    if (ctx) ctx->chart.setSessionStartUTC(hour, minute);
}

void tce_set_session_offset_seconds(TceContext* ctx, double offsetSeconds) {
    if (ctx) ctx->chart.setSessionOffsetSeconds(offsetSeconds);
}

void tce_add_indicator(TceContext* ctx, TceIndicatorKind kind, int period, TceColor color) {
    if (!ctx) return;
    // 단일-period 지표를 일반 진입점에서 dispatch.
    // 다중-파라미터 지표(BB/MACD/Stoch/Ichimoku/PSAR/SuperTrend/DMI)는 전용 함수 필요.
    switch (kind) {
    case TCE_IND_SMA:
    case TCE_IND_EMA:
    case TCE_IND_HMA:
        ctx->chart.addOverlay(kind, period, 0.0, color);
        return;
    case TCE_IND_VWAP:
        ctx->chart.addOverlay(TCE_IND_VWAP, 0, 0.0, color);
        return;
    case TCE_IND_RSI:
        ctx->chart.addSubpanel(TCE_IND_RSI, period, 0, 0, color);
        return;
    case TCE_IND_ATR:
        ctx->chart.addSubpanel(TCE_IND_ATR, period, 0, 0, color);
        return;
    case TCE_IND_CCI:
        ctx->chart.addSubpanel(TCE_IND_CCI, period, 0, 0, color);
        return;
    case TCE_IND_WILLIAMS_R:
        ctx->chart.addSubpanel(TCE_IND_WILLIAMS_R, period, 0, 0, color);
        return;
    case TCE_IND_OBV:
        ctx->chart.addSubpanel(TCE_IND_OBV, 0, 0, 0, color);
        return;
    case TCE_IND_MFI:
        ctx->chart.addSubpanel(TCE_IND_MFI, period, 0, 0, color);
        return;
    default:
        // 다중-파라미터 지표는 전용 함수에서만 등록 가능.
        return;
    }
}

void tce_remove_indicator(TceContext* ctx, TceIndicatorKind kind, int period) {
    if (!ctx) return;
    ctx->chart.removeOverlay(kind, period);
    ctx->chart.removeSubpanel(kind, period);
}

void tce_clear_indicators(TceContext* ctx) {
    if (ctx) ctx->chart.clearAllIndicators();
}

void tce_add_bollinger(TceContext* ctx, int period, double stddev, TceColor color) {
    if (!ctx) return;
    TceColor edge{color.r, color.g, color.b, 0.5f};
    ctx->chart.addOverlay(TCE_IND_BOLLINGER, period, stddev, color, edge);
}

void tce_add_hma(TceContext* ctx, int period, TceColor color) {
    if (!ctx) return;
    ctx->chart.addOverlay(TCE_IND_HMA, period > 1 ? period : 20, 0.0, color);
}

void tce_add_donchian(TceContext* ctx, int period, TceColor color, TceColor edgeColor) {
    if (!ctx) return;
    ctx->chart.addOverlay(TCE_IND_DONCHIAN, period > 0 ? period : 20, 0.0,
                          color, edgeColor);
}

void tce_add_keltner(TceContext* ctx, int emaPeriod, int atrPeriod, double multiplier,
                     TceColor color, TceColor edgeColor) {
    if (!ctx) return;
    ctx->chart.addOverlayEx(TCE_IND_KELTNER,
                            emaPeriod > 0 ? emaPeriod : 20,
                            atrPeriod > 0 ? atrPeriod : 10,
                            0, 0,
                            multiplier > 0 ? multiplier : 2.0, 0.0,
                            color, edgeColor);
}

void tce_add_zigzag(TceContext* ctx, double deviationPct, TceColor color) {
    if (!ctx) return;
    ctx->chart.addOverlay(TCE_IND_ZIGZAG, 0,
                          deviationPct > 0 ? deviationPct : 5.0, color);
}

void tce_add_volume_profile(TceContext* ctx, int bins, double widthRatio,
                             TceColor barColor, TceColor pocColor) {
    if (!ctx) return;
    // bins clamp: [2, 256] — 비합리적 큰 값으로 매 프레임 거대 vector 할당 방지.
    if (bins < 2)   bins = 24;
    if (bins > 256) bins = 256;
    // OverlaySpec 매핑: period=bins, param=widthRatio. color/color2 = bar/poc.
    ctx->chart.addOverlay(TCE_IND_VOLUME_PROFILE,
                          bins,
                          (widthRatio > 0 && widthRatio < 1) ? widthRatio : 0.20,
                          barColor, pocColor);
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

void tce_add_ichimoku(TceContext* ctx, int tenkan, int kijun, int senkouB, int displacement,
                      TceColor tenkanColor, TceColor kijunColor) {
    if (!ctx) return;
    ctx->chart.addOverlayEx(TCE_IND_ICHIMOKU,
                            tenkan > 0 ? tenkan : 9,
                            kijun  > 0 ? kijun  : 26,
                            senkouB > 0 ? senkouB : 52,
                            displacement > 0 ? displacement : 26,
                            0.0, 0.0,
                            tenkanColor, kijunColor);
}

void tce_add_psar(TceContext* ctx, double step, double maxStep, TceColor color) {
    if (!ctx) return;
    ctx->chart.addOverlayEx(TCE_IND_PSAR, 0, 0, 0, 0,
                            step > 0 ? step : 0.02,
                            maxStep > 0 ? maxStep : 0.20,
                            color, color);
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

void tce_add_vwap_with_bands(TceContext* ctx, double numStdev, TceColor color, TceColor bandColor) {
    if (!ctx) return;
    // VWAP overlay spec의 param에 numStdev 보관. 0이면 plain VWAP과 동일.
    ctx->chart.addOverlay(TCE_IND_VWAP, 0, numStdev > 0 ? numStdev : 0.0, color, bandColor);
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

int tce_drawing_begin(TceContext* ctx, TceDrawingKind kind,
                      float x, float y, TceColor color) {
    if (!ctx) return 0;
    return ctx->chart.beginDrawing(kind, x, y, color);
}

void tce_drawing_update(TceContext* ctx, int id, int point_idx, float x, float y) {
    if (!ctx) return;
    ctx->chart.updateDrawing(id, static_cast<size_t>(std::max(0, point_idx)), x, y);
}

void tce_drawing_remove(TceContext* ctx, int id) {
    if (ctx) ctx->chart.removeDrawing(id);
}

void tce_drawing_clear(TceContext* ctx) {
    if (ctx) ctx->chart.clearDrawings();
}

int tce_drawing_hit_test(const TceContext* ctx, float x, float y) {
    return ctx ? ctx->chart.hitTestDrawing(x, y) : 0;
}

void tce_drawing_translate(TceContext* ctx, int id, float dx, float dy) {
    if (ctx) ctx->chart.translateDrawing(id, dx, dy);
}

size_t tce_drawing_count(const TceContext* ctx) {
    return ctx ? ctx->chart.drawingCount() : 0;
}

int tce_drawing_export(const TceContext* ctx, size_t idx, TceDrawingExport* out) {
    if (!ctx || !out) return 0;
    TceDrawingKind kind;
    TceColor color;
    std::vector<tce::DrawingPoint> pts;
    int id = 0;
    if (!ctx->chart.exportDrawing(idx, kind, color, pts, id)) return 0;
    out->id = id;
    out->kind = kind;
    out->color = color;
    out->point_count = static_cast<int>(std::min<size_t>(pts.size(), 2));
    for (int i = 0; i < 2; ++i) {
        out->ts[i]    = (i < out->point_count) ? pts[i].timestamp : 0.0;
        out->price[i] = (i < out->point_count) ? pts[i].price     : 0.0;
    }
    return 1;
}

int tce_drawing_import(TceContext* ctx, const TceDrawingExport* in) {
    if (!ctx || !in) return 0;
    if (in->point_count < 1 || in->point_count > 2) return 0;
    tce::DrawingPoint pts[2];
    for (int i = 0; i < in->point_count; ++i) {
        pts[i].timestamp = in->ts[i];
        pts[i].price     = in->price[i];
    }
    return ctx->chart.importDrawing(in->kind, in->color, pts,
                                     static_cast<size_t>(in->point_count));
}

int tce_add_trade_marker(TceContext* ctx, double ts, double price, int isBuy, double qty) {
    return ctx ? ctx->chart.addTradeMarker(ts, price, isBuy != 0, qty) : 0;
}

void tce_remove_trade_marker(TceContext* ctx, int id) {
    if (ctx) ctx->chart.removeTradeMarker(id);
}

void tce_clear_trade_markers(TceContext* ctx) {
    if (ctx) ctx->chart.clearTradeMarkers();
}

int tce_add_alert_line(TceContext* ctx, double price, TceColor color) {
    return ctx ? ctx->chart.addAlertLine(price, color) : 0;
}

void tce_update_alert_line_by_screen(TceContext* ctx, int id, float y) {
    if (ctx) ctx->chart.updateAlertLineByScreen(id, y);
}

void tce_remove_alert_line(TceContext* ctx, int id) {
    if (ctx) ctx->chart.removeAlertLine(id);
}

void tce_clear_alert_lines(TceContext* ctx) {
    if (ctx) ctx->chart.clearAlertLines();
}

int tce_hit_test_alert_line(const TceContext* ctx, float y) {
    return ctx ? ctx->chart.hitTestAlertLine(y) : 0;
}

void tce_set_alert_callback(TceContext* ctx, TceAlertCrossCallback cb, void* user) {
    if (!ctx) return;
    ctx->chart.setAlertCallback(reinterpret_cast<tce::Chart::AlertCrossFn>(cb), user);
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

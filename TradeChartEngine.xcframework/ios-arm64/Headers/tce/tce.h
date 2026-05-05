#ifndef TCE_H
#define TCE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

// =====================
// 컨텍스트 라이프사이클
// =====================
typedef struct TceContext TceContext;

TceContext* tce_create(void);
void        tce_destroy(TceContext* ctx);
const char* tce_version(void);

// =====================
// 데이터 push
// =====================
// 히스토리 일괄 설정 (덮어쓰기)
void tce_set_history(TceContext* ctx, const TceCandle* candles, size_t count);

// 한 캔들 추가 또는 갱신 (timestamp 동일하면 갱신)
void tce_append_candle(TceContext* ctx, const TceCandle* candle);

// 마지막 캔들의 close/volume만 업데이트 (tick 갱신용)
void tce_update_last(TceContext* ctx, double close, double volume);

size_t tce_candle_count(const TceContext* ctx);

// =====================
// 차트 설정
// =====================
void tce_set_series_type(TceContext* ctx, TceSeriesType type);
void tce_set_color_scheme(TceContext* ctx, TceColorScheme scheme);
void tce_set_size(TceContext* ctx, float width, float height);
void tce_set_volume_panel_visible(TceContext* ctx, int visible);   // 0/1
void tce_set_price_axis_mode(TceContext* ctx, TcePriceAxisMode mode);
void tce_set_renko_brick_size(TceContext* ctx, double size);
void tce_set_show_grid(TceContext* ctx, int show);

// =====================
// 지표 — Overlay (메인 패널 위에)
// =====================
// 같은 (kind, period) 조합은 한 번만 추가됨
void tce_add_indicator(TceContext* ctx, TceIndicatorKind kind, int period, TceColor color);
void tce_remove_indicator(TceContext* ctx, TceIndicatorKind kind, int period);
void tce_clear_indicators(TceContext* ctx);

// Bollinger Bands (overlay) — period 기준선 SMA, ±stddev * sigma
void tce_add_bollinger(TceContext* ctx, int period, double stddev, TceColor color);

// =====================
// 지표 — Subpanel
// =====================
// RSI(period) — 보통 14
void tce_add_rsi(TceContext* ctx, int period, TceColor color);

// MACD(fast=12, slow=26, signal=9) — line / signal / histogram 3개 시리즈
void tce_add_macd(TceContext* ctx, int fast, int slow, int signal,
                  TceColor lineColor, TceColor signalColor, TceColor histColor);

// Stochastic(k_period=14, d_period=3, smooth=3) — %K / %D
void tce_add_stochastic(TceContext* ctx, int kPeriod, int dPeriod, int smooth,
                        TceColor kColor, TceColor dColor);

// ATR(period=14)
void tce_add_atr(TceContext* ctx, int period, TceColor color);

// =====================
// 추가 Overlay (Phase 3)
// =====================
// 일목균형표 — 전환선 색 + 기준선 색 (선행스팬/후행스팬 색은 엔진 기본값)
void tce_add_ichimoku(TceContext* ctx, int tenkan, int kijun, int senkouB, int displacement,
                      TceColor tenkanColor, TceColor kijunColor);

// Parabolic SAR (step, max)
void tce_add_psar(TceContext* ctx, double step, double maxStep, TceColor color);

// SuperTrend (period, multiplier)
void tce_add_supertrend(TceContext* ctx, int period, double multiplier, TceColor color);

// VWAP — session 자동
void tce_add_vwap(TceContext* ctx, TceColor color);

// =====================
// 추가 Subpanel (Phase 3)
// =====================
void tce_add_dmi(TceContext* ctx, int period,
                 TceColor plusDIColor, TceColor minusDIColor, TceColor adxColor);
void tce_add_cci(TceContext* ctx, int period, TceColor color);
void tce_add_williams_r(TceContext* ctx, int period, TceColor color);
void tce_add_obv(TceContext* ctx, TceColor color);
void tce_add_mfi(TceContext* ctx, int period, TceColor color);

// Pivot Points — 일봉 자동. P는 main color, R/S는 secondary color.
void tce_add_pivot_standard(TceContext* ctx, TceColor pColor, TceColor rsColor);
void tce_add_pivot_fibonacci(TceContext* ctx, TceColor pColor, TceColor rsColor);
void tce_add_pivot_camarilla(TceContext* ctx, TceColor pColor, TceColor rsColor);

// =====================
// 뷰포트 (zoom/pan)
// =====================
// 보이는 캔들 수 (정수)
void tce_set_visible_count(TceContext* ctx, int count);
int  tce_visible_count(const TceContext* ctx);

// 가장 오른쪽 캔들 인덱스 (auto-scroll 시 -1 사용)
void tce_set_right_offset(TceContext* ctx, int offset);
int  tce_right_offset(const TceContext* ctx);

// 핀치/드래그 헬퍼 — screen 좌표 입력으로 zoom/pan
void tce_pan(TceContext* ctx, float dx_pixels);
void tce_zoom(TceContext* ctx, float factor, float anchor_x);

// =====================
// 크로스헤어
// =====================
void              tce_set_crosshair(TceContext* ctx, float screen_x, float screen_y);
void              tce_clear_crosshair(TceContext* ctx);
TceCrosshairInfo  tce_crosshair_info(const TceContext* ctx);

// =====================
// Auto-scroll (rightOffset == 0이면 새 캔들 추가 시 자동 우측 정렬)
// =====================
void tce_set_auto_scroll(TceContext* ctx, int enabled);
int  tce_auto_scroll(const TceContext* ctx);

// =====================
// 렌더링
// =====================
// 현재 상태로부터 1 프레임의 vertex/index 빌드.
// 반환된 TceFrame은 tce_release_frame()으로 반환해야 함.
TceFrame tce_build_frame(TceContext* ctx);
void     tce_release_frame(TceFrame frame);

// 같은 프레임의 텍스트 라벨 묶음 (build_frame 직후 호출).
// 메모리는 ctx 내부 소유.
TceLabels tce_build_labels(TceContext* ctx);

// 마지막 build_frame의 레이아웃 정보 — wrapper가 텍스트 영역/패널 영역을 그대로 사용
TceLayout tce_layout(const TceContext* ctx);

// =====================
// 인터랙션 — wrapper는 raw pixel 좌표만 dispatch
// =====================
// pinch: scale은 누적 아닌 마지막 변화 (≈1.0 기준), anchor는 화면 X 픽셀
void tce_apply_pinch(TceContext* ctx, float scale, float anchor_px);
// pan: 양수 dx = 손가락이 우측으로 이동(과거 캔들 보임)
void tce_apply_pan(TceContext* ctx, float dx_px);

// =====================
// 드로잉 도구
// =====================
// 새 드로잉 시작 — screenX/Y는 첫 점. 2점 도구는 두 번째 점도 같은 위치에 자동 추가.
// 반환: drawing id (>0 정상). wrapper는 drag 중 update_drawing(id, 1, x, y)로 두 번째 점 갱신.
int  tce_drawing_begin(TceContext* ctx, TceDrawingKind kind,
                       float screen_x, float screen_y, TceColor color);

// 특정 점 갱신 (point_idx: 0=첫 점, 1=두 번째 점)
void tce_drawing_update(TceContext* ctx, int id, int point_idx,
                        float screen_x, float screen_y);

void tce_drawing_remove(TceContext* ctx, int id);
void tce_drawing_clear(TceContext* ctx);

// 화면 px에서 가장 가까운 드로잉 id (없으면 0). 사용자 tap에서 활용.
int  tce_drawing_hit_test(const TceContext* ctx, float screen_x, float screen_y);
// 드로잉 통째 이동
void tce_drawing_translate(TceContext* ctx, int id, float dx_px, float dy_px);

// =====================
// 매수/매도 마커
// =====================
int  tce_add_trade_marker(TceContext* ctx, double timestamp, double price,
                          int is_buy, double quantity);
void tce_remove_trade_marker(TceContext* ctx, int id);
void tce_clear_trade_markers(TceContext* ctx);

// =====================
// 가격 알림선 (드래그 가능)
// =====================
int  tce_add_alert_line(TceContext* ctx, double price, TceColor color);
void tce_update_alert_line_by_screen(TceContext* ctx, int id, float screen_y);
void tce_remove_alert_line(TceContext* ctx, int id);
void tce_clear_alert_lines(TceContext* ctx);
// 화면 px 위치에서 가장 가까운 알림선 id (없으면 0)
int  tce_hit_test_alert_line(const TceContext* ctx, float screen_y);

#ifdef __cplusplus
}
#endif

#endif

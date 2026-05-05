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
// 렌더링
// =====================
// 현재 상태로부터 1 프레임의 vertex/index 빌드.
// 반환된 TceFrame은 tce_release_frame()으로 반환해야 함.
TceFrame tce_build_frame(TceContext* ctx);
void     tce_release_frame(TceFrame frame);

#ifdef __cplusplus
}
#endif

#endif

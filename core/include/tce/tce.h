/**
 * TradeChartEngine — public C ABI (함수 선언)
 *
 * 공개 API의 모든 진입점. wrapper(iOS Swift / Android JNI / Web Wasm)는 이 헤더의
 * 함수만 호출하면 같은 차트를 같은 동작으로 렌더할 수 있다.
 *
 * 표준 사용 흐름 (한 차트 인스턴스 기준)
 *   1) ctx = tce_create()
 *   2) tce_set_size(ctx, w, h)               — 화면 픽셀 크기. 회전/리사이즈 시 매번 호출.
 *   3) tce_set_history(ctx, candles, n)      — 초기 시계열 push.
 *   4) tce_set_series_type / set_color_scheme / set_volume_panel_visible / ... — 옵션.
 *   5) tce_add_indicator / tce_add_rsi / ... — 지표 등록 (원하는 만큼).
 *   6) (매 프레임)
 *        frame  = tce_build_frame(ctx);
 *        labels = tce_build_labels(ctx);
 *        layout = tce_layout(ctx);
 *        // wrapper가 frame.meshes를 GPU에 업로드 + labels/layout으로 텍스트 overlay 그림
 *        tce_release_frame(frame);
 *   7) (제스처) tce_apply_pinch / tce_apply_pan / tce_set_crosshair / ...
 *   8) (실시간) tce_append_candle 또는 tce_update_last
 *   9) tce_destroy(ctx)
 *
 * 모든 함수는 같은 ctx에 대해 단일 스레드(main)에서만 호출. NULL ctx에 대한 호출은 안전(no-op).
 */
#ifndef TCE_H
#define TCE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * 컨텍스트 라이프사이클
 * ============================================================ */

/// 엔진 컨텍스트 — opaque 핸들. host는 포인터로만 다룬다.
typedef struct TceContext TceContext;

/// 새 차트 컨텍스트 생성.
/// @return 성공 시 비-NULL. 실패 시 NULL (현재는 메모리 부족 외 실패 케이스 없음).
TceContext* tce_create(void);

/// 컨텍스트 파괴 — 내부 리소스(시계열, 지표, 드로잉, 마지막 프레임 메시) 모두 해제.
/// 호출 후 ctx 사용 금지. NULL 안전.
void        tce_destroy(TceContext* ctx);

/// 엔진 버전 문자열 ("0.7.0" 등). 정적 메모리 — 해제 금지.
const char* tce_version(void);

/* ============================================================
 * 데이터 push
 * ============================================================ */

/// 히스토리 일괄 설정 — 기존 시계열을 완전히 덮어쓴다.
/// @param candles  timestamp 오름차순 정렬 필요. 비정렬이면 동작 미정.
/// @param count    candles 배열 길이.
/// @note 입력 배열은 호출 동안만 유효 — 엔진이 즉시 내부로 복사한다.
void tce_set_history(TceContext* ctx, const TceCandle* candles, size_t count);

/// 한 캔들 추가 — 마지막 timestamp보다 크면 append, 같으면 in-place 갱신.
/// 그보다 작으면 무시(또는 정렬된 위치 삽입은 미지원).
void tce_append_candle(TceContext* ctx, const TceCandle* candle);

/// 마지막 캔들의 close와 volume만 빠르게 갱신 — tick 단위 실시간 업데이트용.
/// high/low는 close가 기존 high/low를 벗어나면 자동 확장.
void tce_update_last(TceContext* ctx, double close, double volume);

/// 현재 보관 중인 캔들 개수.
size_t tce_candle_count(const TceContext* ctx);

/// index 위치의 캔들을 복사해서 *out에 채움. 성공 시 1, 실패(범위 밖/NULL) 시 0.
/// crosshair_info의 candle_index를 host가 OHLC로 역참조할 때 사용.
int    tce_get_candle(const TceContext* ctx, size_t index, TceCandle* out);

/* ============================================================
 * 차트 설정
 * ============================================================ */

/// 메인 패널 그리기 모드 변경 (캔들/라인/Area/OHLC bar/Heikin-Ashi/Renko).
/// Heikin-Ashi/Renko는 엔진 내부에서 시계열을 변환해 그린다 — 원본은 보존.
///
/// 지표 계산 정책 (host가 알아야 하는 결정):
///   - Heikin-Ashi: 메인 패널은 HA 캔들로 그리지만, 모든 지표(SMA/RSI/MACD/...)는
///     원본 OHLC 기준으로 계산된다 (시리즈 길이 동일하므로 인덱스 일치).
///   - Renko: 메인 패널은 brick 시리즈(원본보다 짧을 수 있음)로 그리고,
///     지표도 brick 시리즈 기준으로 계산된다 (원본 인덱스와 매핑할 수 없으므로).
void tce_set_series_type(TceContext* ctx, TceSeriesType type);

/// 양봉/음봉 색 스킴 변경 — 한국식(빨강/파랑) 또는 미국식(초록/빨강).
void tce_set_color_scheme(TceContext* ctx, TceColorScheme scheme);

/// 화면 크기(픽셀) 설정 — 회전/리사이즈/스플릿 시 호출.
/// width/height에 0 또는 음수 전달 금지.
void tce_set_size(TceContext* ctx, float width, float height);

/// 거래량 패널 표시 여부 (1=표시, 0=숨김). 숨기면 메인 패널이 그만큼 확장.
void tce_set_volume_panel_visible(TceContext* ctx, int visible);

/// 가격 축 매핑 모드 — Linear / Log / Percent.
void tce_set_price_axis_mode(TceContext* ctx, TcePriceAxisMode mode);

/// Renko 모드의 brick 가격 크기 (price 단위).
/// 0 또는 음수면 자동값(마지막 close × 0.5%) 사용.
void tce_set_renko_brick_size(TceContext* ctx, double size);

/// 그리드(plot 영역의 가로/세로 보조선) 표시 여부 (1=표시, 0=숨김).
void tce_set_show_grid(TceContext* ctx, int show);

/// 거래소 세션 시작(UTC 기준) 설정 — VWAP/Pivot의 일별 boundary를 그 시각에 맞춤.
/// 사용 예:
///   - KR/JP (KST/JST 09:00 시작 → UTC 00:00):  tce_set_session_start_utc(ctx, 0, 0);  // 또는 호출 생략
///   - NYSE (09:30 EST → UTC 14:30):            tce_set_session_start_utc(ctx, 14, 30);
///   - EU CET (09:00 CET → UTC 08:00):          tce_set_session_start_utc(ctx, 8, 0);
///   - 24시간 (CRYPTO):                          tce_set_session_start_utc(ctx, 0, 0);
/// DST 전환은 host가 처리(EDT 09:30 = UTC 13:30 → 시간만 다시 set).
void tce_set_session_start_utc(TceContext* ctx, int hour, int minute);

/// 직접 offset(초)을 지정 — `tce_set_session_start_utc`와 둘 중 하나만 쓰면 됨.
/// day boundary = floor((timestamp + offset) / 86400).
void tce_set_session_offset_seconds(TceContext* ctx, double offsetSeconds);

/* ============================================================
 * 지표 — 일반 (Overlay/Subpanel 공용 진입점)
 * ============================================================ */

/// 지표 추가/갱신.
/// 같은 (kind, period) 조합이 이미 있으면 색만 갱신, 새로 만들지 않는다.
/// SMA/EMA/RSI/ATR/CCI/Williams %R/MFI 등 단일-period 지표에 사용.
/// 다중 파라미터 지표(BB, MACD, Stoch, Ichimoku, PSAR, SuperTrend, DMI)는
/// 전용 함수(`tce_add_bollinger` 등)를 사용해야 한다.
void tce_add_indicator(TceContext* ctx, TceIndicatorKind kind, int period, TceColor color);

/// 지표 제거 — (kind, period) 일치하는 것 1개. 없으면 no-op.
void tce_remove_indicator(TceContext* ctx, TceIndicatorKind kind, int period);

/// 모든 지표 제거 (overlay + subpanel + pivot 포함).
void tce_clear_indicators(TceContext* ctx);

/* ============================================================
 * 지표 — Overlay (메인 패널)
 * ============================================================ */

/// Bollinger Bands — period의 SMA를 기준선, ±stddev*sigma를 상하단으로 그린다.
/// 일반적인 값: period=20, stddev=2.0.
void tce_add_bollinger(TceContext* ctx, int period, double stddev, TceColor color);

/// Hull Moving Average — WMA 기반의 라그 적은 MA. 표준 period=20.
/// 산식: WMA(2*WMA(close, period/2) - WMA(close, period), sqrt(period))
void tce_add_hma(TceContext* ctx, int period, TceColor color);

/// Donchian Channels — period 캔들의 high.max(상단)/low.min(하단)/중앙선.
/// 표준값 period=20. 터틀 트레이딩에서 자주 쓰임.
/// @param color      중앙선(중간) 색
/// @param edgeColor  상/하단 채널 색 (보통 alpha 낮춰 옅게)
void tce_add_donchian(TceContext* ctx, int period, TceColor color, TceColor edgeColor);

/// Keltner Channels — EMA(emaPeriod) ± multiplier * ATR(atrPeriod).
/// 표준값 emaPeriod=20, atrPeriod=10, multiplier=2.0. BB와 비교/대체로 자주 쓰임.
/// @param color      중앙선(EMA) 색
/// @param edgeColor  상/하단 채널 색 (보통 alpha 낮춰 옅게)
void tce_add_keltner(TceContext* ctx, int emaPeriod, int atrPeriod, double multiplier,
                     TceColor color, TceColor edgeColor);

/// ZigZag — deviationPct(%) 이상의 swing high/low 직선 연결.
/// 표준값 deviationPct=5.0 (5%). repaint 주의: 마지막 swing은 잠정값이며
/// 새 캔들이 들어와 더 큰 극값이 발견되면 마지막 swing 위치가 갱신된다 — host는
/// ZigZag 기반 알림 신뢰성을 검증한 후 사용할 것.
void tce_add_zigzag(TceContext* ctx, double deviationPct, TceColor color);

/// Volume Profile — 가시 가격 범위를 bins등분 후 bin별 거래량 합산.
/// plot 우측 widthRatio 영역에 반투명 막대 + POC(거래량 최대 bin) / VAH/VAL(70% 누적 양 끝)
/// 가로선. **alpha 0.30으로 강제** — 캔들/지표가 막대 뒤로 보이게.
/// **Renko 모드**: 미지원 — brick 시리즈가 시간 무관이라 volume 매핑이 의미 없음. no-op.
/// **viewport 변경 시 재계산** — indicator 캐시 대상 아님.
/// @param bins        가격 등분 수 (default 24)
/// @param widthRatio  plot 폭 대비 막대 영역 비율 (0..1, default 0.20)
/// @param barColor    bin 막대 색 (alpha는 0.30으로 자동 보정)
/// @param pocColor    POC/VAH/VAL 가로선 색
void tce_add_volume_profile(TceContext* ctx, int bins, double widthRatio,
                             TceColor barColor, TceColor pocColor);

/// 일목균형표 — 전환선/기준선/선행스팬A·B/후행스팬 + 구름.
/// 표준값: tenkan=9, kijun=26, senkouB=52, displacement=26.
/// 선행스팬/후행스팬 색은 엔진 기본값(반투명)을 사용 — wrapper에서 주관 색 설정 불필요.
void tce_add_ichimoku(TceContext* ctx, int tenkan, int kijun, int senkouB, int displacement,
                      TceColor tenkanColor, TceColor kijunColor);

/// Parabolic SAR — 추세 반전점을 점으로 표시.
/// 표준값: step=0.02, maxStep=0.20.
void tce_add_psar(TceContext* ctx, double step, double maxStep, TceColor color);

/// SuperTrend — ATR 기반 추세 추종 선.
/// 표준값: period=10, multiplier=3.0.
void tce_add_supertrend(TceContext* ctx, int period, double multiplier, TceColor color);

/// VWAP — 거래량 가중 평균가. session(일별)으로 자동 reset.
/// 일별 경계는 `tce_set_session_start_utc` / `tce_set_session_offset_seconds` 적용 후
/// floor((ts + offset) / 86400) 기준 (default offset=0 → UTC 자정).
void tce_add_vwap(TceContext* ctx, TceColor color);

/// VWAP + ±numStdev × sigma 밴드. numStdev<=0이면 그냥 VWAP과 동일.
/// 같은 day session 내 가중분산으로 sigma 계산. 밴드 흡수 priceY 자동.
/// @param numStdev   밴드 폭 (1.0 / 2.0 / 1.5 등 자유). 표준은 2.0.
/// @param color      VWAP 본선 색
/// @param bandColor  ±sigma 라인 색 (보통 alpha 낮춰 옅게)
void tce_add_vwap_with_bands(TceContext* ctx, double numStdev, TceColor color, TceColor bandColor);

/// Pivot Points — 일봉 자동 계산. P / R1·R2·R3 / S1·S2·S3 = 7개 가로선.
/// 일별 경계는 host local timezone 기준. 한국장(09:00 KST 시작)은 host가 timestamp 정렬 후 push.
/// @param pColor   P(중심)선 색
/// @param rsColor  R*/S* 6개 선의 공통 색
void tce_add_pivot_standard(TceContext* ctx, TceColor pColor, TceColor rsColor);
void tce_add_pivot_fibonacci(TceContext* ctx, TceColor pColor, TceColor rsColor);
void tce_add_pivot_camarilla(TceContext* ctx, TceColor pColor, TceColor rsColor);

/* ============================================================
 * 지표 — Subpanel (별도 패널)
 * ============================================================ */

/// RSI — 상대 강도 지수. period 표준값 14. 0..100 + 30/70 가이드선 자동.
void tce_add_rsi(TceContext* ctx, int period, TceColor color);

/// MACD — line(EMA-fast - EMA-slow) / signal(EMA(line, signal)) / histogram.
/// 표준값: fast=12, slow=26, signal=9.
void tce_add_macd(TceContext* ctx, int fast, int slow, int signal,
                  TceColor lineColor, TceColor signalColor, TceColor histColor);

/// Stochastic — %K(원시) → smooth → %D.
/// 표준값: kPeriod=14, dPeriod=3, smooth=3. 20/80 가이드선 자동.
void tce_add_stochastic(TceContext* ctx, int kPeriod, int dPeriod, int smooth,
                        TceColor kColor, TceColor dColor);

/// Stochastic RSI — RSI 시리즈에 stochastic 공식 적용.
/// 표준값 rsiPeriod=14, kPeriod=14, dPeriod=3, smooth=3. 0..100, 20/80 가이드선 자동.
void tce_add_stochastic_rsi(TceContext* ctx,
                             int rsiPeriod, int kPeriod, int dPeriod, int smooth,
                             TceColor kColor, TceColor dColor);

/// ATR — Wilder의 평균 진폭. period 표준값 14.
void tce_add_atr(TceContext* ctx, int period, TceColor color);

/// DMI/ADX — +DI / -DI / ADX 3개 라인. period 표준값 14.
void tce_add_dmi(TceContext* ctx, int period,
                 TceColor plusDIColor, TceColor minusDIColor, TceColor adxColor);

/// CCI — Commodity Channel Index. period 표준값 20. ±100 가이드선 자동.
void tce_add_cci(TceContext* ctx, int period, TceColor color);

/// Williams %R — period 표준값 14. -100..0 범위. -20/-80 가이드선 자동.
void tce_add_williams_r(TceContext* ctx, int period, TceColor color);

/// OBV — On-Balance Volume. 누적이라 period 없음.
void tce_add_obv(TceContext* ctx, TceColor color);

/// MFI — Money Flow Index. period 표준값 14. 0..100 + 20/80 가이드선 자동.
void tce_add_mfi(TceContext* ctx, int period, TceColor color);

/* ============================================================
 * 뷰포트 (zoom / pan)
 * ============================================================ */

/// 보이는 캔들 수 — 작을수록 zoom-in.
/// 엔진이 [2, 5000] 범위로 자동 clamp.
void tce_set_visible_count(TceContext* ctx, int count);
int  tce_visible_count(const TceContext* ctx);

/// 가장 우측에 보이는 캔들의 인덱스 오프셋 (마지막 캔들 = 0). 양수면 미래 공백.
/// auto-scroll(rightOffset==0) 상태에서 새 캔들이 들어오면 자동 우측 정렬 유지.
void tce_set_right_offset(TceContext* ctx, int offset);
int  tce_right_offset(const TceContext* ctx);

/// (deprecated) 픽셀 단위 pan. `tce_apply_pan()` 사용 권장.
void tce_pan(TceContext* ctx, float dx_pixels)
    __attribute__((deprecated("use tce_apply_pan")));
/// (deprecated) 픽셀 단위 zoom. `tce_apply_pinch()` 사용 권장.
void tce_zoom(TceContext* ctx, float factor, float anchor_x)
    __attribute__((deprecated("use tce_apply_pinch")));

/// rightOffset = 0으로 되돌려 가장 최근 캔들이 우측 끝에 보이도록 한다.
void tce_reset_viewport(TceContext* ctx);
/// 모든 캔들이 한 화면에 들어가도록 visibleCount = candle_count, rightOffset = 0.
void tce_fit_all(TceContext* ctx);

/* ============================================================
 * 좌표 변환 (host 커스텀 overlay/annotation)
 *
 * 모두 마지막 build_frame 결과(lastLayout) 기준 — build_frame 호출 후에만 의미 있음.
 * pan/zoom으로 viewport 변경 시 다음 build_frame 후 결과가 갱신된다.
 * ============================================================ */

/// 화면 X(px) → 캔들 인덱스. plot 영역 밖이거나 캔들 없음이면 -1.
int    tce_screen_x_to_index(const TceContext* ctx, float screen_x);
/// 캔들 인덱스 → 화면 X(px) 중심. 가시 범위 밖이면 0 반환(out_x 비변경), 성공 시 1.
int    tce_index_to_screen_x(const TceContext* ctx, int index, float* out_x);
/// 화면 Y(px) → raw price. PRICE_LOG/PERCENT 모드에서 자동 역변환.
double tce_screen_y_to_price(const TceContext* ctx, float screen_y);
/// raw price → 화면 Y(px). PRICE_LOG/PERCENT 모드에서 자동 변환.
float  tce_price_to_screen_y(const TceContext* ctx, double price);

/* ============================================================
 * 지표 값 query — host의 crosshair hover 라벨용
 *
 * 정책
 *   - 단일 출력 지표(SMA/EMA/RSI/CCI/Williams %R/ATR/MFI/OBV/VWAP/SuperTrend)는
 *     `tce_query_indicator_value`로 통일.
 *   - 다출력 지표(BB/MACD/Stochastic/DMI/Ichimoku/Donchian/Keltner/Pivot)는 전용 함수.
 *   - 매칭 정책: (kind, period) 정확 일치한 spec이 등록되어 있어야 1 반환.
 *     period가 없는 지표(VWAP/OBV/Pivot)는 period 인자 무시.
 *   - candle_index 범위 밖 / 첫 (period-1)개 등 미정의 위치 → 0 반환, out 미변경.
 *   - 캐시 없음 — host가 hover 시점에만 호출. build_frame과 같은 데이터로 재계산.
 * ============================================================ */

/// 단일 출력 지표 값 query. 1=valid, 0=invalid(미등록/index 범위 밖/period 미일치).
int tce_query_indicator_value(const TceContext* ctx, TceIndicatorKind kind, int period,
                              size_t candle_index, double* out);

/// Bollinger Bands query — upper/middle/lower 3채널.
int tce_query_bollinger(const TceContext* ctx, int period, size_t candle_index,
                        double* upper, double* middle, double* lower);

/// Donchian Channels query — upper/middle/lower 3채널.
int tce_query_donchian(const TceContext* ctx, int period, size_t candle_index,
                       double* upper, double* middle, double* lower);

/// Keltner Channels query — upper/middle/lower 3채널.
int tce_query_keltner(const TceContext* ctx, int emaPeriod, size_t candle_index,
                      double* upper, double* middle, double* lower);

/// MACD query — line/signal/histogram 3채널.
int tce_query_macd(const TceContext* ctx, size_t candle_index,
                   double* line, double* signal, double* histogram);

/// Stochastic query — %K / %D 2채널.
int tce_query_stochastic(const TceContext* ctx, size_t candle_index,
                         double* k, double* d);

/// Stochastic RSI query — %K / %D 2채널 (Stochastic과 동일 시그니처).
int tce_query_stochastic_rsi(const TceContext* ctx, size_t candle_index,
                              double* k, double* d);

/// DMI/ADX query — +DI / -DI / ADX 3채널.
int tce_query_dmi(const TceContext* ctx, int period, size_t candle_index,
                  double* plusDI, double* minusDI, double* adx);

/// Pivot Points query — kind는 등록된 piv 종류와 같아야 함. 7채널.
int tce_query_pivot(const TceContext* ctx, TceIndicatorKind kind, size_t candle_index,
                    double* p, double* r1, double* r2, double* r3,
                    double* s1, double* s2, double* s3);

/// Ichimoku query — 5채널 (tenkan/kijun/senkouA/senkouB/chikou).
int tce_query_ichimoku(const TceContext* ctx, size_t candle_index,
                       double* tenkan, double* kijun,
                       double* senkouA, double* senkouB, double* chikou);

/// SuperTrend query — line + direction(+1=상승, -1=하락).
int tce_query_supertrend(const TceContext* ctx, size_t candle_index,
                         double* line, int* direction);

/// VWAP bands query — middle/upper/lower 3채널. VWAP이 numStdev=0(밴드 없음)으로 등록되었으면
/// upper/lower는 0으로 채워지고 1을 반환하지 않음(밴드 미등록). middle은 항상 valid.
int tce_query_vwap_bands(const TceContext* ctx, size_t candle_index,
                         double* middle, double* upper, double* lower);

/* ============================================================
 * 크로스헤어 (마우스 hover / 터치 & 홀드)
 * ============================================================ */

/// 크로스헤어 위치 설정. plot 영역 밖이어도 horizontal/vertical은 그려지고 candle_index는 -1.
void              tce_set_crosshair(TceContext* ctx, float screen_x, float screen_y);
void              tce_clear_crosshair(TceContext* ctx);
/// 현재 크로스헤어 정보 (candle_index, price, timestamp 포함). visible=0이면 의미 없음.
TceCrosshairInfo  tce_crosshair_info(const TceContext* ctx);

/* ============================================================
 * Auto-scroll (실시간 데이터 따라 우측 정렬 유지)
 * ============================================================ */

/// auto-scroll on/off. on이면 right_offset이 0일 때 새 캔들 도착 시 자동으로 0 유지.
/// 사용자가 pan하여 right_offset != 0 상태가 되면 일시적으로 비활성, pan으로 다시 0이 되면 복원.
void tce_set_auto_scroll(TceContext* ctx, int enabled);
int  tce_auto_scroll(const TceContext* ctx);

/* ============================================================
 * 렌더링 (매 프레임)
 * ============================================================ */

/// 현재 상태(시계열/뷰포트/지표/드로잉/마커/...)로부터 1 프레임의 vertex/index 빌드.
/// @return TceFrame.meshes는 ctx 소유. 다음 build_frame 호출 시 자동으로 재사용됨.
/// @note 현재 구현에선 release_frame은 no-op(호출 누락 안전). 향후 async 빌드 도입 시 의미 부여.
TceFrame tce_build_frame(TceContext* ctx);

/// 프레임 해제 — 현재는 no-op. 향후 thread-safe 빌드를 위해 페어 호출 권장.
void     tce_release_frame(TceFrame frame);

/// 같은 프레임의 텍스트 라벨 묶음 (가격축/시간축/마지막 가격/크로스헤어 라벨).
/// `build_frame` 직후 호출 — 그 시점 viewport/layout 기준으로 계산.
/// 문자열 메모리는 ctx 소유. 다음 build_labels 호출까지 유효.
TceLabels tce_build_labels(TceContext* ctx);

/// 마지막 build_frame의 레이아웃 정보 — wrapper가 텍스트 영역/패널 영역을 그대로 사용.
/// build_frame 호출 전에는 zero-rect 반환.
TceLayout tce_layout(const TceContext* ctx);

/* ============================================================
 * 인터랙션 — wrapper는 raw pixel 좌표만 dispatch
 * ============================================================ */

/// 핀치 제스처 적용. recognizer.scale의 마지막 변화량 (≈1.0 기준)을 그대로 전달.
/// @param scale       1.0보다 크면 zoom-in, 작으면 zoom-out.
/// @param anchor_px   화면 X 픽셀 — 핀치 중심점이 시간축에서 같은 위치를 유지하도록 사용.
/// @note recognizer.scale = 1로 reset하는 책임은 wrapper.
void tce_apply_pinch(TceContext* ctx, float scale, float anchor_px);

/// 팬(드래그) 제스처 적용.
/// @param dx_px  양수면 손가락이 우측으로 이동 — 차트는 과거(좌측)를 보여줌.
/// @note translation을 0으로 reset하는 책임은 wrapper.
void tce_apply_pan(TceContext* ctx, float dx_px);

/* ============================================================
 * 드로잉 도구 (사용자가 그리는 추세선/수평선/...)
 * ============================================================ */

/// 새 드로잉 시작. 두-점 도구(trendline/fib/measure/rectangle)는 시작 시
/// 두 번째 점도 같은 위치에 자동 추가 — drag 중 `tce_drawing_update(id, 1, ...)`로 갱신.
/// @return drawing id (>0 정상, 0 실패).
int  tce_drawing_begin(TceContext* ctx, TceDrawingKind kind,
                       float screen_x, float screen_y, TceColor color);

/// 특정 점 좌표 갱신.
/// @param point_idx  0 = 첫 점, 1 = 두 번째 점 (1점 도구는 1 무시).
void tce_drawing_update(TceContext* ctx, int id, int point_idx,
                        float screen_x, float screen_y);

/// 드로잉 1개 제거. 없으면 no-op.
void tce_drawing_remove(TceContext* ctx, int id);

/// 모든 드로잉 제거.
void tce_drawing_clear(TceContext* ctx);

/// 화면 px 위치에서 가장 가까운 드로잉의 id (없거나 거리 임계 초과면 0).
/// 사용자 tap에서 활용 — 선택 → 삭제 등.
int  tce_drawing_hit_test(const TceContext* ctx, float screen_x, float screen_y);

/// 드로잉 통째 평행이동 (dx/dy 픽셀). 이미 도메인 좌표(timestamp/price)로 보관되어 있어
/// 화면 px 입력은 도메인 단위로 자동 변환 후 누적.
void tce_drawing_translate(TceContext* ctx, int id, float dx_px, float dy_px);

/* ── 직렬화 export/import ─────────────────────────────────────
 * host가 사용자 그림을 영속 저장(JSON/binary)하고 다음 세션에 복원.
 * 도메인 좌표 기반이라 series 변경/리로드 시에도 안전.
 * ──────────────────────────────────────────────────────────── */

/// 등록된 드로잉 개수.
size_t tce_drawing_count(const TceContext* ctx);

/// idx 위치의 드로잉을 *out에 export. 1=ok, 0=invalid(idx 범위 밖/NULL).
/// 등록 순서 보존 — host가 같은 순서로 import하면 hit-test 우선순위 동일.
int    tce_drawing_export(const TceContext* ctx, size_t idx, TceDrawingExport* out);

/// import — 새 id 부여 후 반환. 0=invalid(kind 범위 밖/point_count 불일치/NULL).
/// HORIZONTAL/VERTICAL은 point_count=1, 나머지는 2 필수.
int    tce_drawing_import(TceContext* ctx, const TceDrawingExport* in);

/* ============================================================
 * 매수/매도 마커
 * ============================================================ */

/// 거래 마커 추가 (▲ 매수 / ▼ 매도).
/// @param timestamp  거래 시각 — 가까운 캔들에 연결되거나 보간.
/// @param price      체결가
/// @param is_buy     1 = 매수, 0 = 매도
/// @param quantity   수량 (0이면 라벨 미표시)
/// @return 마커 id (>0). remove에 사용.
int  tce_add_trade_marker(TceContext* ctx, double timestamp, double price,
                          int is_buy, double quantity);

void tce_remove_trade_marker(TceContext* ctx, int id);
void tce_clear_trade_markers(TceContext* ctx);

/* ============================================================
 * 가격 알림선 (사용자가 드래그로 가격 변경)
 * ============================================================ */

/// 가격 알림선 추가 — 가로선 + 우측 가격 라벨.
/// @return 알림선 id (>0). update/remove에 사용.
int  tce_add_alert_line(TceContext* ctx, double price, TceColor color);

/// 화면 Y 픽셀로 알림선 가격 갱신 — 사용자가 드래그할 때 매 프레임 호출.
/// 엔진이 Y픽셀 → 가격으로 역변환.
void tce_update_alert_line_by_screen(TceContext* ctx, int id, float screen_y);

void tce_remove_alert_line(TceContext* ctx, int id);
void tce_clear_alert_lines(TceContext* ctx);

/// 화면 Y 픽셀에 가장 가까운 알림선 id (거리 임계 초과면 0). drag 시작점 검출용.
int  tce_hit_test_alert_line(const TceContext* ctx, float screen_y);

/// 알림선 cross 콜백 시그니처. append_candle/update_last 호출 시 prev_close 와 new_close
/// 사이를 가로지르는 알림선이 있으면 한 번 호출된다.
/// @param alert_id    이벤트가 발생한 알림선의 id (>0)
/// @param cross_price 알림선의 가격 (alert_id가 등록될 때의 price)
/// @param user        등록 시 넘긴 user 포인터 (host 컨텍스트 전달용)
typedef void (*TceAlertCrossCallback)(int alert_id, double cross_price, void* user);

/// 알림선 cross 콜백 등록. 같은 ctx에 다시 호출하면 이전 콜백을 대체.
/// cb=NULL을 넘기면 콜백 해제.
/// @note 콜백은 append_candle/update_last를 호출한 스레드(=main)에서 호출됨.
void tce_set_alert_callback(TceContext* ctx, TceAlertCrossCallback cb, void* user);

#ifdef __cplusplus
}
#endif

#endif

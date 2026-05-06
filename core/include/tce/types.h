/**
 * TradeChartEngine — public C ABI 타입 정의
 *
 * 이 헤더는 host(iOS Swift / Android JNI / Web Wasm)에서 그대로 사용 가능한
 * `extern "C"` 타입 묶음이다. 모든 ABI는 단순 POD struct + enum으로만 구성.
 *
 * 좌표 단위
 *   - timestamp      : Unix epoch seconds (소수 허용)
 *   - price          : 거래소 표기 그대로 (예: 원, USD)
 *   - screen x/y     : pixel (top-left origin)
 *   - color (r/g/b/a): 0..1 normalized
 *
 * 메모리 소유권
 *   - host에서 엔진으로 전달하는 포인터(예: TceCandle*)는 호출 동안만 유효.
 *     엔진이 내부로 복사한 뒤 즉시 반환.
 *   - 엔진에서 host로 반환하는 포인터(예: TceFrame.meshes)는 ctx가 소유.
 *     `tce_release_frame()`을 호출한 뒤 다시 접근 금지.
 *
 * 스레드 안전성
 *   - 모든 함수는 같은 ctx에 대해 main thread에서만 호출 (현재 정책).
 *     Wrapper가 동기화 책임.
 */
#ifndef TCE_TYPES_H
#define TCE_TYPES_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * 기본 프리미티브 (다른 struct가 멤버로 사용)
 * ============================================================ */

/// RGBA 색 — 0..1 normalized.
typedef struct TceColor { float r, g, b, a; } TceColor;

/// 화면 픽셀 단위 사각형. (x,y)는 top-left.
typedef struct TceRect {
    float x;
    float y;
    float width;
    float height;
} TceRect;

/* ============================================================
 * 데이터 입력 (host → 엔진)
 * ============================================================ */

/// 단일 캔들/봉 데이터 — host가 시계열을 push할 때 사용.
/// timestamp는 정렬되어 있어야 한다 (오름차순). 같은 timestamp면 갱신으로 처리.
typedef struct TceCandle {
    double timestamp;   ///< Unix epoch seconds. ms 단위면 1000.0으로 나눠서 전달.
    double open;        ///< 시가
    double high;        ///< 고가
    double low;         ///< 저가
    double close;       ///< 종가
    double volume;      ///< 거래량 (단위 무관, 보조 패널 정규화에만 사용)
} TceCandle;

/// 매수/매도 표시 — host(증권 앱)가 자기 거래 기록을 push.
typedef struct TceTradeMarker {
    double timestamp;       ///< 거래 시각 (해당 캔들의 timestamp 또는 보간 가능)
    double price;           ///< 체결가
    int    is_buy;          ///< 1 = 매수, 0 = 매도
    double quantity;        ///< 수량 (0이면 라벨 표시 생략)
} TceTradeMarker;

/// 가격 알림선 — 사용자가 드래그로 위치 조정 가능. host에서 add/remove.
/// 주: 보통은 host가 직접 이 struct를 만들어 넘기는 일은 없고
/// `tce_add_alert_line()` 결과로 엔진이 부여한 id를 host가 보관한다.
typedef struct TceAlertLine {
    int      id;            ///< 엔진이 부여 (>0)
    double   price;
    TceColor color;         ///< 라인/라벨 배경 색
} TceAlertLine;

/* ============================================================
 * 차트 모드/스타일 enum
 * ============================================================ */

/// 차트 종류. 메인 가격 패널의 그리기 방식.
typedef enum TceSeriesType {
    TCE_SERIES_CANDLE       = 0,  ///< 캔들스틱 (기본)
    TCE_SERIES_LINE         = 1,  ///< 종가 라인
    TCE_SERIES_AREA         = 2,  ///< 종가 라인 + 하단 면적 채움
    TCE_SERIES_OHLC_BAR     = 3,  ///< OHLC bar (서양식 — H/L 세로선 + open/close 좌우 tick)
    TCE_SERIES_HEIKIN_ASHI  = 4,  ///< Heikin-Ashi (변환된 OHLC를 캔들 모양으로)
    TCE_SERIES_RENKO        = 5,  ///< Renko (시간축 무시, brick size 기반)
} TceSeriesType;

/// 가격 축 매핑 모드.
typedef enum TcePriceAxisMode {
    TCE_PRICE_LINEAR  = 0,  ///< 선형 (기본). 1원 = 1픽셀 비율 동일.
    TCE_PRICE_LOG     = 1,  ///< 로그 스케일. log(price)로 매핑.
    TCE_PRICE_PERCENT = 2,  ///< 가시 범위 첫 캔들 close 기준 % 변화.
} TcePriceAxisMode;

/// 색 스킴 — 양봉/음봉 색.
typedef enum TceColorScheme {
    TCE_SCHEME_KOREA = 0,   ///< 한국식: 양봉 빨강, 음봉 파랑 (기본)
    TCE_SCHEME_US    = 1,   ///< 미국식: 양봉 초록, 음봉 빨강
} TceColorScheme;

/// 지표 종류. Overlay는 메인 패널 위에, Subpanel은 별도 보조 패널에 그려짐.
/// 같은 (kind, period) 조합은 한 번만 등록되며 두 번째 호출은 색만 갱신.
typedef enum TceIndicatorKind {
    /* ── Overlay (메인 가격 패널) ── */
    TCE_IND_SMA               = 0,    ///< 단순 이동평균 (period)
    TCE_IND_EMA               = 1,    ///< 지수 이동평균 (period)
    TCE_IND_BOLLINGER         = 2,    ///< 볼린저 밴드 (period + stddev)
    TCE_IND_ICHIMOKU          = 3,    ///< 일목균형표 5선 + 구름
    TCE_IND_PSAR              = 4,    ///< Parabolic SAR (step + maxStep)
    TCE_IND_SUPERTREND        = 5,    ///< SuperTrend (period + multiplier)
    TCE_IND_VWAP              = 6,    ///< Session VWAP (자동 일별 reset)
    TCE_IND_PIVOT_STANDARD    = 7,    ///< Pivot Points (Standard) — 7개 가로선
    TCE_IND_PIVOT_FIBONACCI   = 8,    ///< Pivot Points (Fibonacci)
    TCE_IND_PIVOT_CAMARILLA   = 9,    ///< Pivot Points (Camarilla)
    TCE_IND_DONCHIAN          = 10,   ///< Donchian Channels — period 캔들 high.max/low.min/center
    TCE_IND_KELTNER           = 11,   ///< Keltner Channels — EMA(emaP) ± multiplier * ATR(atrP)
    TCE_IND_ZIGZAG            = 12,   ///< ZigZag — deviationPct% 이상의 swing high/low 직선 연결
    TCE_IND_VOLUME_PROFILE    = 13,   ///< Volume Profile — 가시 가격 범위 bin별 거래량 + POC/VAH/VAL

    /* ── Subpanel (별도 패널) ── */
    TCE_IND_RSI               = 100,  ///< RSI (period). 0..100 + 30/70 가이드선
    TCE_IND_MACD              = 101,  ///< MACD line/signal/histogram
    TCE_IND_STOCHASTIC        = 102,  ///< Stochastic %K/%D + 20/80 가이드선
    TCE_IND_ATR               = 103,  ///< ATR (Wilder period)
    TCE_IND_DMI_ADX           = 104,  ///< +DI/-DI/ADX (period)
    TCE_IND_CCI               = 105,  ///< CCI (period) + ±100 가이드선
    TCE_IND_WILLIAMS_R        = 106,  ///< Williams %R (period). -100..0
    TCE_IND_OBV               = 107,  ///< On-Balance Volume (누적)
    TCE_IND_MFI               = 108,  ///< Money Flow Index (period). 0..100
} TceIndicatorKind;

/// 드로잉 도구 종류. wrapper는 `tce_drawing_begin(kind, ...)`로 새 객체 시작.
typedef enum TceDrawingKind {
    TCE_DRAW_TRENDLINE          = 0,  ///< 두 점을 잇는 직선
    TCE_DRAW_HORIZONTAL         = 1,  ///< 한 가격 수평선 (price만 사용)
    TCE_DRAW_VERTICAL           = 2,  ///< 한 시간 수직선 (timestamp만 사용)
    TCE_DRAW_FIB_RETRACEMENT    = 3,  ///< 두 점 사이 0/0.236/0.382/0.5/0.618/0.786/1.0 가로선
    TCE_DRAW_MEASURE            = 4,  ///< 두 점 사이 가격 차이 + % 라벨 박스
    TCE_DRAW_RECTANGLE          = 5,  ///< 두 점 대각선 사각형 (외곽선 + 반투명 채움)
    TCE_DRAW_FIB_EXTENSION      = 6,  ///< 두 점 너머 1.0/1.272/1.382/1.618/2.0/2.618 가로선 (추세 연장)
} TceDrawingKind;

/// 텍스트 라벨의 anchor (정렬 기준). wrapper는 픽셀 좌표 + anchor를 보고 그대로 .position()으로 그림.
typedef enum TceTextAnchor {
    TCE_ANCHOR_LEFT_CENTER   = 0,
    TCE_ANCHOR_RIGHT_CENTER  = 1,
    TCE_ANCHOR_CENTER_TOP    = 2,
    TCE_ANCHOR_CENTER_BOTTOM = 3,
    TCE_ANCHOR_CENTER_CENTER = 4,    ///< 일반적으로 모든 라벨이 이 모드. 엔진이 padding을 좌표에 반영.
} TceTextAnchor;

/// 라벨 종류 — wrapper가 분류해서 다른 폰트/색을 적용 가능.
typedef enum TceLabelKind {
    TCE_LABEL_PRICE_AXIS  = 0,           ///< Y축 가격 라벨 (priceAxis 영역)
    TCE_LABEL_TIME_AXIS   = 1,           ///< X축 시간 라벨 (timeAxis 영역)
    TCE_LABEL_LAST_PRICE  = 2,           ///< 마지막 가격 우측 강조 라벨
    TCE_LABEL_CROSSHAIR_PRICE = 3,       ///< 크로스헤어 가격 (어두운 배경 박스)
    TCE_LABEL_CROSSHAIR_TIME  = 4,       ///< 크로스헤어 시간
} TceLabelKind;

/* ============================================================
 * 상호작용 결과 / 쿼리 응답
 * ============================================================ */

/// 크로스헤어 hit-test 결과 — `tce_crosshair_info()` 반환.
typedef struct TceCrosshairInfo {
    int    visible;         ///< 0 = 숨김, 1 = 표시
    int    candle_index;    ///< -1 = 캔들 영역 밖
    double price;           ///< 크로스헤어 위치의 가격 (Y 매핑 역함수)
    double timestamp;       ///< candle_index의 timestamp
    float  screen_x;        ///< 입력된 screen X
    float  screen_y;        ///< 입력된 screen Y
} TceCrosshairInfo;

/* ============================================================
 * 렌더링 출력 (엔진 → host)
 * ============================================================ */

/// 정점 — pos(x,y) + color(rgba). wrapper가 GPU vertex buffer로 그대로 업로드.
typedef struct TceVertex {
    float x;
    float y;
    float r;
    float g;
    float b;
    float a;
} TceVertex;

/// 한 묶음의 메시 — 같은 primitive 타입의 정점/인덱스.
typedef struct TceMesh {
    const TceVertex* vertices;      ///< 엔진 소유. 다음 build_frame까지 유효.
    size_t           vertex_count;
    const uint32_t*  indices;
    size_t           index_count;
    int              primitive;     ///< 0 = triangles, 1 = lines
} TceMesh;

/// 한 프레임의 메시 묶음 (캔들/지표/그리드/마커/알림선/드로잉/크로스헤어 등 통합).
/// `tce_build_frame()` 반환 → `tce_release_frame()`으로 반환.
typedef struct TceFrame {
    const TceMesh* meshes;
    size_t         mesh_count;
    void*          internal;        ///< 엔진 내부 핸들 (release용 — wrapper는 무시)
} TceFrame;

/// 한 텍스트 라벨 — 좌표/anchor/문자열만 담음. 폰트는 wrapper(native CoreText/Canvas)가 결정.
/// text는 raw 숫자/시간 문자열 ("12345.0", "14:25"). 통화 단위/콤마/현지화는 host가 후처리.
typedef struct TceLabel {
    const char*    text;            ///< null-terminated UTF-8. 엔진 소유.
    float          x;               ///< 화면 px (anchor 적용된 좌표)
    float          y;
    TceTextAnchor  anchor;
    TceLabelKind   kind;
    TceColor       color;           ///< 텍스트 색
    TceColor       background;      ///< 배경 박스 색. alpha 0이면 배경 없음.
} TceLabel;

/// 한 프레임의 라벨 묶음 — `tce_build_labels()` 반환.
typedef struct TceLabels {
    const TceLabel* items;
    size_t          count;
} TceLabels;

/// 드로잉 직렬화 — host가 사용자 그림을 영속 저장/복원할 때 사용.
/// 도메인 좌표(timestamp/price)만 보관하므로 차트 series가 달라도 안전.
typedef struct TceDrawingExport {
    int            id;             ///< export 시: 엔진 id. import 시: 무시(새 id 부여).
    TceDrawingKind kind;
    TceColor       color;
    int            point_count;    ///< 1(horizontal/vertical) 또는 2(나머지).
    double         ts[2];          ///< 도메인 timestamp. 미사용 슬롯은 의미 없음.
    double         price[2];       ///< 도메인 raw price. HORIZONTAL은 price[0]만, VERTICAL은 ts[0]만.
} TceDrawingExport;

/// 한 프레임의 레이아웃 정보 — wrapper가 텍스트/오버레이 영역 결정에 사용.
/// `tce_layout()`으로 build_frame 직후 query.
typedef struct TceLayout {
    TceRect plot;            ///< 메인 가격 패널 (캔들 영역. axis 제외)
    TceRect priceAxis;       ///< 우측 가격축 (라벨 표시 영역)
    TceRect timeAxis;        ///< 하단 시간축
    TceRect volumePanel;     ///< 거래량 패널 (volumeVisible=false면 width=0)
    int     subpanelCount;
    TceRect subpanels[8];    ///< 보조 패널들 (RSI/MACD 등 — 최대 8개)
} TceLayout;

#ifdef __cplusplus
}
#endif

#endif

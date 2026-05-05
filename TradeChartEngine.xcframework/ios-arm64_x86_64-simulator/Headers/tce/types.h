#ifndef TCE_TYPES_H
#define TCE_TYPES_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 단일 캔들 데이터 — 호스트에서 push할 때 사용
typedef struct TceCandle {
    double timestamp;   // Unix epoch seconds (소수 가능)
    double open;
    double high;
    double low;
    double close;
    double volume;
} TceCandle;

// 차트 종류
typedef enum TceSeriesType {
    TCE_SERIES_CANDLE       = 0,
    TCE_SERIES_LINE         = 1,
    TCE_SERIES_AREA         = 2,
    TCE_SERIES_OHLC_BAR     = 3,
    TCE_SERIES_HEIKIN_ASHI  = 4,
    TCE_SERIES_RENKO        = 5,
} TceSeriesType;

// 가격축 모드
typedef enum TcePriceAxisMode {
    TCE_PRICE_LINEAR  = 0,
    TCE_PRICE_LOG     = 1,
    TCE_PRICE_PERCENT = 2,   // 가시 범위 첫 캔들 기준 % 변화
} TcePriceAxisMode;

// 색 스킴
typedef enum TceColorScheme {
    TCE_SCHEME_KOREA = 0,    // 양봉 빨강 / 음봉 파랑
    TCE_SCHEME_US    = 1,    // 양봉 초록 / 음봉 빨강
} TceColorScheme;

// 지표 종류
typedef enum TceIndicatorKind {
    // Overlay (메인 패널)
    TCE_IND_SMA               = 0,
    TCE_IND_EMA               = 1,
    TCE_IND_BOLLINGER         = 2,   // period + stddev
    TCE_IND_ICHIMOKU          = 3,   // tenkan/kijun/senkouB periods
    TCE_IND_PSAR              = 4,   // step + max
    TCE_IND_SUPERTREND        = 5,   // period + multiplier
    TCE_IND_VWAP              = 6,   // session 자동
    TCE_IND_PIVOT_STANDARD    = 7,   // 일봉 기반 자동 — 별도 파라미터 없음
    TCE_IND_PIVOT_FIBONACCI   = 8,
    TCE_IND_PIVOT_CAMARILLA   = 9,

    // Subpanel
    TCE_IND_RSI               = 100,
    TCE_IND_MACD              = 101,
    TCE_IND_STOCHASTIC        = 102,
    TCE_IND_ATR               = 103,
    TCE_IND_DMI_ADX           = 104, // period
    TCE_IND_CCI               = 105, // period
    TCE_IND_WILLIAMS_R        = 106, // period
    TCE_IND_OBV               = 107,
    TCE_IND_MFI               = 108, // period
} TceIndicatorKind;

// RGBA 0..1
typedef struct TceColor { float r, g, b, a; } TceColor;

// 정점: pos(x,y) + color(rgba)
typedef struct TceVertex {
    float x;
    float y;
    float r;
    float g;
    float b;
    float a;
} TceVertex;

// 한 프레임의 메시 묶음
// vertices/indices는 엔진이 소유. host는 chart_release_frame()으로 반환
typedef struct TceMesh {
    const TceVertex* vertices;
    size_t           vertex_count;
    const uint32_t*  indices;
    size_t           index_count;
    int              primitive;     // 0=triangles, 1=lines
} TceMesh;

// 한 프레임 = 여러 mesh
typedef struct TceFrame {
    const TceMesh* meshes;
    size_t         mesh_count;
    void*          internal;        // 엔진 내부 핸들 (release용)
} TceFrame;

// 매수/매도 표시 (호스트 앱이 push)
typedef struct TceTradeMarker {
    double timestamp;
    double price;
    int    is_buy;       // 1=매수, 0=매도
    double quantity;     // 표기용 (옵션, 0이면 라벨 없음)
} TceTradeMarker;

// 가격 알림선 (사용자가 드래그 가능)
typedef struct TceAlertLine {
    int    id;
    double price;
    TceColor color;
} TceAlertLine;

// 크로스헤어 hit-test 결과
typedef struct TceCrosshairInfo {
    int    visible;          // 0/1
    int    candle_index;     // -1 if 없음
    double price;
    double timestamp;
    float  screen_x;
    float  screen_y;
} TceCrosshairInfo;

// 텍스트 정렬 (anchor)
typedef enum TceTextAnchor {
    TCE_ANCHOR_LEFT_CENTER   = 0,
    TCE_ANCHOR_RIGHT_CENTER  = 1,
    TCE_ANCHOR_CENTER_TOP    = 2,
    TCE_ANCHOR_CENTER_BOTTOM = 3,
    TCE_ANCHOR_CENTER_CENTER = 4,
} TceTextAnchor;

// 라벨 종류
typedef enum TceLabelKind {
    TCE_LABEL_PRICE_AXIS  = 0,   // Y축 가격
    TCE_LABEL_TIME_AXIS   = 1,   // X축 시간
    TCE_LABEL_LAST_PRICE  = 2,   // 마지막 가격 우측 박스
    TCE_LABEL_CROSSHAIR_PRICE = 3,
    TCE_LABEL_CROSSHAIR_TIME  = 4,
} TceLabelKind;

// 텍스트 라벨 1개 (코어가 채움, wrapper가 폰트로 그림)
typedef struct TceLabel {
    const char*    text;        // null-terminated, 엔진이 소유
    float          x;           // 화면 px
    float          y;
    TceTextAnchor  anchor;
    TceLabelKind   kind;
    TceColor       color;
    TceColor       background;  // alpha 0이면 배경 없음
} TceLabel;

// 라벨 묶음 (한 프레임)
typedef struct TceLabels {
    const TceLabel* items;
    size_t          count;
} TceLabels;

// 화면 픽셀 단위 사각형
typedef struct TceRect {
    float x;
    float y;
    float width;
    float height;
} TceRect;

// 드로잉 객체 종류
typedef enum TceDrawingKind {
    TCE_DRAW_TRENDLINE          = 0,  // 2점 직선
    TCE_DRAW_HORIZONTAL         = 1,  // 1점 (price만 사용)
    TCE_DRAW_VERTICAL           = 2,  // 1점 (timestamp만 사용)
    TCE_DRAW_FIB_RETRACEMENT    = 3,  // 2점 + 7개 가로 라인
    TCE_DRAW_MEASURE            = 4,  // 2점 + 가격/시간/% 라벨
    TCE_DRAW_RECTANGLE          = 5,  // 2점 대각선 사각형
} TceDrawingKind;

// 한 프레임의 레이아웃 정보 — 텍스트/오버레이 영역 결정에 사용
typedef struct TceLayout {
    TceRect plot;            // 메인 가격 패널 (캔들이 그려지는 영역)
    TceRect priceAxis;       // 우측 가격 축 (라벨 영역)
    TceRect timeAxis;        // 하단 시간 축
    TceRect volumePanel;     // 거래량 패널 (없으면 width=0)
    int     subpanelCount;
    TceRect subpanels[8];    // 보조 패널들 (최대 8개)
} TceLayout;

#ifdef __cplusplus
}
#endif

#endif

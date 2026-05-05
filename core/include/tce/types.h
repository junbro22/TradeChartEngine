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
    TCE_SERIES_CANDLE = 0,
    TCE_SERIES_LINE   = 1,   // 종가 라인
    TCE_SERIES_AREA   = 2,
} TceSeriesType;

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
    TCE_IND_BOLLINGER         = 2,   // period + stddev (default 2.0)

    // Subpanel
    TCE_IND_RSI               = 100, // period
    TCE_IND_MACD              = 101, // fast, slow, signal
    TCE_IND_STOCHASTIC        = 102, // k_period, d_period, smooth
    TCE_IND_ATR               = 103, // period
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

#ifdef __cplusplus
}
#endif

#endif

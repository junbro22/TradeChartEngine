#include "renderer/frame_builder.h"
#include "data/series.h"
#include "viewport/viewport.h"
#include "indicator/ma.h"
#include "indicator/ema.h"
#include "indicator/rsi.h"
#include "indicator/macd.h"
#include "indicator/bollinger.h"
#include "indicator/donchian.h"
#include "indicator/keltner.h"
#include "indicator/zigzag.h"
#include "indicator/hma.h"
#include "indicator/stochastic.h"
#include "indicator/stoch_rsi.h"
#include "indicator/atr.h"
#include "indicator/ichimoku.h"
#include "indicator/psar.h"
#include "indicator/supertrend.h"
#include "indicator/vwap.h"
#include "indicator/dmi.h"
#include "indicator/cci.h"
#include "indicator/williams_r.h"
#include "indicator/obv.h"
#include "indicator/mfi.h"
#include "indicator/pivot.h"
#include "data/transforms.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace tce {

void FrameOutput::clear() {
    vbufs_.clear();
    ibufs_.clear();
    primitives_.clear();
    meshHeaders_.clear();
}

void FrameOutput::addMesh(std::vector<TceVertex>&& v,
                          std::vector<uint32_t>&&  i,
                          int primitive) {
    vbufs_.push_back(std::move(v));
    ibufs_.push_back(std::move(i));
    primitives_.push_back(primitive);
    rebuildHeaders_();
}

void FrameOutput::rebuildHeaders_() {
    meshHeaders_.clear();
    meshHeaders_.reserve(vbufs_.size());
    for (size_t i = 0; i < vbufs_.size(); ++i) {
        TceMesh m{
            vbufs_[i].data(),
            vbufs_[i].size(),
            ibufs_[i].data(),
            ibufs_[i].size(),
            primitives_[i]
        };
        meshHeaders_.push_back(m);
    }
}

namespace {

constexpr int PRIM_TRIANGLES = 0;
constexpr int PRIM_LINES     = 1;

constexpr float PANEL_GAP = 4.0f;

struct YMap {
    float top, bottom;
    double minP, maxP;
    TcePriceAxisMode mode = TCE_PRICE_LINEAR;
    double percentBase = 0.0;       // percent 모드의 기준 가격

    double normalize(double v) const {
        switch (mode) {
        case TCE_PRICE_LOG:
            return (v > 0) ? std::log(v) : 0.0;
        case TCE_PRICE_PERCENT:
            return (percentBase > 0) ? (v / percentBase - 1.0) * 100.0 : 0.0;
        default:
            return v;
        }
    }
    float yFor(double v) const {
        double nv = normalize(v);
        if (maxP == minP) return (top + bottom) * 0.5f;
        double t = (nv - minP) / (maxP - minP);
        return static_cast<float>(bottom - t * (bottom - top));
    }
};

void emitLine(std::vector<TceVertex>& v, std::vector<uint32_t>& idx,
              float x1, float y1, float x2, float y2, TceColor c) {
    uint32_t base = static_cast<uint32_t>(v.size());
    v.push_back({x1, y1, c.r, c.g, c.b, c.a});
    v.push_back({x2, y2, c.r, c.g, c.b, c.a});
    idx.push_back(base);
    idx.push_back(base + 1);
}

void emitRect(std::vector<TceVertex>& v, std::vector<uint32_t>& idx,
              float x1, float y1, float x2, float y2, TceColor c) {
    uint32_t base = static_cast<uint32_t>(v.size());
    v.push_back({x1, y1, c.r, c.g, c.b, c.a});
    v.push_back({x2, y1, c.r, c.g, c.b, c.a});
    v.push_back({x2, y2, c.r, c.g, c.b, c.a});
    v.push_back({x1, y2, c.r, c.g, c.b, c.a});
    idx.push_back(base);     idx.push_back(base + 1); idx.push_back(base + 2);
    idx.push_back(base);     idx.push_back(base + 2); idx.push_back(base + 3);
}

void emitPolyline(std::vector<TceVertex>& v, std::vector<uint32_t>& idx,
                  const std::vector<std::optional<double>>& values,
                  size_t from, size_t to, float slot,
                  const YMap& ymap, TceColor c) {
    std::optional<float> px, py;
    for (size_t i = from; i < to; ++i) {
        if (!values[i]) { px.reset(); py.reset(); continue; }
        float x = (static_cast<float>(i - from) + 0.5f) * slot;
        float y = ymap.yFor(*values[i]);
        if (px && py) emitLine(v, idx, *px, *py, x, y, c);
        px = x; py = y;
    }
}

TceColor candleColor(bool isUp, TceColorScheme scheme) {
    if (scheme == TCE_SCHEME_KOREA) {
        return isUp ? TceColor{0.93f, 0.27f, 0.27f, 1.0f}
                    : TceColor{0.27f, 0.53f, 1.00f, 1.0f};
    }
    return isUp ? TceColor{0.16f, 0.74f, 0.45f, 1.0f}
                : TceColor{0.93f, 0.27f, 0.27f, 1.0f};
}

// 보조 패널 한 개에 대한 스켈레톤 — `series`는 indicator 계산에 쓰이는 시리즈(원본).
void buildSubpanel(std::vector<TceVertex>& vTri, std::vector<uint32_t>& iTri,
                   std::vector<TceVertex>& vLine, std::vector<uint32_t>& iLine,
                   const Series& series,
                   size_t from, size_t to,
                   float slot, float panelTop, float panelBottom,
                   const SubpanelSpec& spec) {
    YMap ymap;
    ymap.top = panelTop + 4.0f;
    ymap.bottom = panelBottom - 4.0f;

    auto rangeOf = [&](const std::vector<std::optional<double>>& a,
                        const std::vector<std::optional<double>>& b = {},
                        const std::vector<std::optional<double>>& c = {}) {
        double mn =  std::numeric_limits<double>::infinity();
        double mx = -std::numeric_limits<double>::infinity();
        auto absorb = [&](const std::vector<std::optional<double>>& s) {
            if (s.empty()) return;
            for (size_t i = from; i < to && i < s.size(); ++i) {
                if (!s[i]) continue;
                mn = std::min(mn, *s[i]);
                mx = std::max(mx, *s[i]);
            }
        };
        absorb(a); absorb(b); absorb(c);
        if (!std::isfinite(mn) || !std::isfinite(mx) || mn == mx) {
            mn = 0; mx = 1;
        }
        return std::make_pair(mn, mx);
    };

    if (spec.kind == TCE_IND_RSI) {
        auto v = rsi(series, spec.p1);
        ymap.minP = 0; ymap.maxP = 100;
        // 30/70 가이드 라인
        TceColor guide{0.4f, 0.4f, 0.5f, 0.4f};
        emitLine(vLine, iLine, 0, ymap.yFor(30), slot * (to - from), ymap.yFor(30), guide);
        emitLine(vLine, iLine, 0, ymap.yFor(70), slot * (to - from), ymap.yFor(70), guide);
        emitPolyline(vLine, iLine, v, from, to, slot, ymap, spec.color1);
        return;
    }
    if (spec.kind == TCE_IND_MACD) {
        auto m = macd(series, spec.p1, spec.p2, spec.p3);
        auto rng = rangeOf(m.line, m.signal, m.histogram);
        ymap.minP = rng.first;  ymap.maxP = rng.second;
        // histogram = bar
        const float w = std::max(1.0f, slot * 0.65f);
        float zeroY = ymap.yFor(0.0);
        for (size_t i = from; i < to; ++i) {
            if (!m.histogram[i]) continue;
            float cx = (static_cast<float>(i - from) + 0.5f) * slot;
            float y = ymap.yFor(*m.histogram[i]);
            float top = std::min(y, zeroY);
            float bot = std::max(y, zeroY);
            if (bot - top < 1.0f) bot = top + 1.0f;
            emitRect(vTri, iTri, cx - w*0.5f, top, cx + w*0.5f, bot, spec.color3);
        }
        emitPolyline(vLine, iLine, m.line,   from, to, slot, ymap, spec.color1);
        emitPolyline(vLine, iLine, m.signal, from, to, slot, ymap, spec.color2);
        return;
    }
    if (spec.kind == TCE_IND_STOCHASTIC) {
        auto s = stochastic(series, spec.p1, spec.p2, spec.p3);
        ymap.minP = 0; ymap.maxP = 100;
        TceColor guide{0.4f, 0.4f, 0.5f, 0.4f};
        emitLine(vLine, iLine, 0, ymap.yFor(20), slot * (to - from), ymap.yFor(20), guide);
        emitLine(vLine, iLine, 0, ymap.yFor(80), slot * (to - from), ymap.yFor(80), guide);
        emitPolyline(vLine, iLine, s.k, from, to, slot, ymap, spec.color1);
        emitPolyline(vLine, iLine, s.d, from, to, slot, ymap, spec.color2);
        return;
    }
    if (spec.kind == TCE_IND_STOCHASTIC_RSI) {
        // p1=rsiPeriod, p2=kPeriod, p3=dPeriod, p4=smooth
        auto s = stochasticRSI(series, spec.p1, spec.p2, spec.p3, spec.p4);
        ymap.minP = 0; ymap.maxP = 100;
        TceColor guide{0.4f, 0.4f, 0.5f, 0.4f};
        emitLine(vLine, iLine, 0, ymap.yFor(20), slot * (to - from), ymap.yFor(20), guide);
        emitLine(vLine, iLine, 0, ymap.yFor(80), slot * (to - from), ymap.yFor(80), guide);
        emitPolyline(vLine, iLine, s.k, from, to, slot, ymap, spec.color1);
        emitPolyline(vLine, iLine, s.d, from, to, slot, ymap, spec.color2);
        return;
    }
    if (spec.kind == TCE_IND_ATR) {
        auto a = atr(series, spec.p1);
        auto rng = rangeOf(a);
        ymap.minP = rng.first; ymap.maxP = rng.second;
        emitPolyline(vLine, iLine, a, from, to, slot, ymap, spec.color1);
        return;
    }
    if (spec.kind == TCE_IND_DMI_ADX) {
        auto d = dmi(series, spec.p1);
        ymap.minP = 0; ymap.maxP = 100;
        emitPolyline(vLine, iLine, d.plusDI,  from, to, slot, ymap, spec.color1);
        emitPolyline(vLine, iLine, d.minusDI, from, to, slot, ymap, spec.color2);
        emitPolyline(vLine, iLine, d.adx,     from, to, slot, ymap, spec.color3);
        return;
    }
    if (spec.kind == TCE_IND_CCI) {
        auto c = cci(series, spec.p1);
        auto rng = rangeOf(c);
        ymap.minP = std::min(rng.first, -200.0);
        ymap.maxP = std::max(rng.second, 200.0);
        TceColor guide{0.4f, 0.4f, 0.5f, 0.4f};
        emitLine(vLine, iLine, 0, ymap.yFor(100),  slot * (to - from), ymap.yFor(100),  guide);
        emitLine(vLine, iLine, 0, ymap.yFor(-100), slot * (to - from), ymap.yFor(-100), guide);
        emitPolyline(vLine, iLine, c, from, to, slot, ymap, spec.color1);
        return;
    }
    if (spec.kind == TCE_IND_WILLIAMS_R) {
        auto w = williamsR(series, spec.p1);
        ymap.minP = -100; ymap.maxP = 0;
        TceColor guide{0.4f, 0.4f, 0.5f, 0.4f};
        emitLine(vLine, iLine, 0, ymap.yFor(-20), slot * (to - from), ymap.yFor(-20), guide);
        emitLine(vLine, iLine, 0, ymap.yFor(-80), slot * (to - from), ymap.yFor(-80), guide);
        emitPolyline(vLine, iLine, w, from, to, slot, ymap, spec.color1);
        return;
    }
    if (spec.kind == TCE_IND_OBV) {
        auto o = obv(series);
        auto rng = rangeOf(o);
        ymap.minP = rng.first; ymap.maxP = rng.second;
        emitPolyline(vLine, iLine, o, from, to, slot, ymap, spec.color1);
        return;
    }
    if (spec.kind == TCE_IND_MFI) {
        auto m = mfi(series, spec.p1);
        ymap.minP = 0; ymap.maxP = 100;
        TceColor guide{0.4f, 0.4f, 0.5f, 0.4f};
        emitLine(vLine, iLine, 0, ymap.yFor(20), slot * (to - from), ymap.yFor(20), guide);
        emitLine(vLine, iLine, 0, ymap.yFor(80), slot * (to - from), ymap.yFor(80), guide);
        emitPolyline(vLine, iLine, m, from, to, slot, ymap, spec.color1);
        return;
    }
}

} // namespace

void FrameBuilder::build(const Series& series,
                         const Viewport& vp,
                         const ChartConfig& cfg,
                         const std::vector<OverlaySpec>& overlays,
                         const std::vector<SubpanelSpec>& subpanels,
                         const CrosshairState& cross,
                         FrameOutput& out,
                         PanelLayout& layout,
                         const Series* indicatorSeries) {
    out.clear();
    layout = PanelLayout{};

    const auto& cs = series.candles();
    if (cs.empty()) return;
    // 지표 계산용 시리즈 (HA 모드에선 원본 series_가 넘어옴).
    // 별도 지정 없으면 draw series와 동일하게 처리.
    const Series& iSeries = indicatorSeries ? *indicatorSeries : series;

    size_t from, to;
    vp.rangeFor(cs.size(), from, to);
    if (to <= from) return;

    const float W = vp.width();
    const float H = vp.height();

    // ===== 축 영역 예약 =====
    constexpr float PRICE_AXIS_W = 60.0f;
    constexpr float TIME_AXIS_H  = 20.0f;
    const float plotW = std::max(10.0f, W - PRICE_AXIS_W);
    const float plotH = std::max(10.0f, H - TIME_AXIS_H);

    // 가시 슬롯 폭은 plot 영역 기준으로 재계산
    const float slot = plotW / static_cast<float>(vp.visibleCount());

    // ===== 패널 레이아웃 =====
    int subCount = static_cast<int>(subpanels.size()) + (cfg.volumeVisible ? 1 : 0);
    float mainRatio = 1.0f;
    if (subCount > 0) {
        float subRatio = std::min(0.50f, 0.18f * subCount);
        mainRatio = 1.0f - subRatio;
    }

    const float mainTop    = 0.0f;
    const float mainBottom = plotH * mainRatio;
    const float subBlockTop = mainBottom + PANEL_GAP;
    const float subBlockBottom = plotH;
    const float subBlockH = std::max(20.0f, subBlockBottom - subBlockTop);
    const float perSubH = subBlockH / std::max(1, subCount);

    // ===== 메인 패널 — 가격 Y 매핑 =====
    YMap priceY{};
    priceY.top = mainTop + 8.0f;
    priceY.bottom = mainBottom - 8.0f;
    priceY.mode = cfg.priceMode;
    if (cfg.priceMode == TCE_PRICE_PERCENT && from < cs.size()) {
        priceY.percentBase = cs[from].close;
    }
    {
        double rawMin = series.minLowInRange(from, to);
        double rawMax = series.maxHighInRange(from, to);
        priceY.minP = priceY.normalize(rawMin);
        priceY.maxP = priceY.normalize(rawMax);
        if (priceY.minP > priceY.maxP) std::swap(priceY.minP, priceY.maxP);
    }
    // 메인 패널 가시 범위를 위/아래로 벗어날 수 있는 overlay를 모두 흡수.
    // BB/Donchian/Keltner/Pivot/Ichimoku/SuperTrend/PSAR/VWAP — MA/EMA는 원래 가격 봉 안.
    // 흡수 시 indicator series 기준 (HA: 원본 OHLC, Renko: brick).
    auto absorb = [&](const std::optional<double>& v) {
        if (!v) return;
        double nv = priceY.normalize(*v);
        priceY.maxP = std::max(priceY.maxP, nv);
        priceY.minP = std::min(priceY.minP, nv);
    };
    auto absorbRange = [&](const std::vector<std::optional<double>>& s) {
        for (size_t i = from; i < to && i < s.size(); ++i) absorb(s[i]);
    };
    for (const auto& ov : overlays) {
        switch (ov.kind) {
        case TCE_IND_BOLLINGER: {
            auto bb = bollinger(iSeries, ov.period, ov.param);
            absorbRange(bb.upper); absorbRange(bb.lower);
            break;
        }
        case TCE_IND_DONCHIAN: {
            auto dc = donchian(iSeries, ov.period > 0 ? ov.period : 20);
            absorbRange(dc.upper); absorbRange(dc.lower);
            break;
        }
        case TCE_IND_KELTNER: {
            const int eP = ov.period > 0 ? ov.period : 20;
            const int aP = ov.p2     > 0 ? ov.p2     : 10;
            const double m = ov.param > 0 ? ov.param : 2.0;
            auto e = ema(iSeries, eP);
            auto a = atr(iSeries, aP);
            for (size_t i = from; i < to && i < e.size() && i < a.size(); ++i) {
                if (e[i] && a[i]) {
                    double upper = *e[i] + m * (*a[i]);
                    double lower = *e[i] - m * (*a[i]);
                    absorb(upper); absorb(lower);
                }
            }
            break;
        }
        case TCE_IND_ICHIMOKU: {
            const int tk = ov.period > 0 ? ov.period : 9;
            const int kj = ov.p2     > 0 ? ov.p2     : 26;
            const int sB = ov.p3     > 0 ? ov.p3     : 52;
            const int dp = ov.p4     > 0 ? ov.p4     : 26;
            auto ic = ichimoku(iSeries, tk, kj, sB, dp);
            // senkouA/B는 displacement만큼 미래 plot 가능 — 흡수 범위를 displacement까지 확장
            const size_t ext = std::min(to + static_cast<size_t>(dp), ic.senkouA.size());
            for (size_t i = from; i < ext; ++i) {
                if (i < ic.senkouA.size()) absorb(ic.senkouA[i]);
                if (i < ic.senkouB.size()) absorb(ic.senkouB[i]);
            }
            break;
        }
        case TCE_IND_PIVOT_STANDARD:
        case TCE_IND_PIVOT_FIBONACCI:
        case TCE_IND_PIVOT_CAMARILLA: {
            PivotKind k =
                (ov.kind == TCE_IND_PIVOT_STANDARD)  ? PivotKind::Standard :
                (ov.kind == TCE_IND_PIVOT_FIBONACCI) ? PivotKind::Fibonacci :
                                                       PivotKind::Camarilla;
            auto p = pivot(iSeries, k, cfg.sessionOffsetSeconds);
            absorbRange(p.r3); absorbRange(p.s3);
            // R1/R2/S1/S2는 보통 R3/S3 안에 들어가지만 안전을 위해 흡수
            absorbRange(p.r1); absorbRange(p.r2);
            absorbRange(p.s1); absorbRange(p.s2);
            break;
        }
        case TCE_IND_SUPERTREND: {
            auto st = superTrend(iSeries,
                                 ov.period > 0 ? ov.period : 10,
                                 ov.param  > 0 ? ov.param  : 3.0);
            absorbRange(st.line);
            break;
        }
        case TCE_IND_PSAR: {
            const double step    = ov.param  > 0 ? ov.param  : 0.02;
            const double maxStep = ov.param2 > 0 ? ov.param2 : 0.20;
            auto p = psar(iSeries, step, maxStep);
            absorbRange(p);
            break;
        }
        case TCE_IND_VWAP: {
            // VWAP은 보통 가격 봉 안에 들지만 일중 누적이 양 끝으로 빠지는 시점에서 안전 흡수.
            // param > 0이면 numStdev 밴드 — upper/lower도 흡수.
            if (ov.param > 0) {
                auto vb = vwapBands(iSeries, cfg.sessionOffsetSeconds, ov.param);
                absorbRange(vb.upper); absorbRange(vb.lower);
            } else {
                auto v = vwap(iSeries, cfg.sessionOffsetSeconds);
                absorbRange(v);
            }
            break;
        }
        default: break;
        }
    }
    {
        double pad = (priceY.maxP - priceY.minP) * 0.05;
        priceY.minP -= pad; priceY.maxP += pad;
    }

    // outparam: 라벨 빌더가 같은 매핑을 사용
    layout.plot       = Rect{0, mainTop, plotW, mainBottom - mainTop};
    layout.priceAxis  = Rect{plotW, 0, PRICE_AXIS_W, plotH};
    layout.timeAxis   = Rect{0, plotH, plotW, TIME_AXIS_H};
    layout.priceMin   = priceY.minP;
    layout.priceMax   = priceY.maxP;
    layout.priceMode  = priceY.mode;
    layout.percentBase = priceY.percentBase;
    layout.volumeProfile = VolumeProfileResult{};  // present=false로 reset
    layout.subpanels.clear();

    std::vector<TceVertex> vTri, vLine;
    std::vector<uint32_t>  iTri, iLine;

    // 캔들/라인/면적/OHLC bar 분기 (Heikin-Ashi/Renko는 Chart에서 변환된 시리즈가 들어오므로 캔들 모양으로 처리)
    auto isCandleShape = [&](TceSeriesType t) {
        return t == TCE_SERIES_CANDLE || t == TCE_SERIES_HEIKIN_ASHI || t == TCE_SERIES_RENKO;
    };

    if (isCandleShape(cfg.seriesType)) {
        const float candleW = std::max(1.0f, slot * 0.65f);
        for (size_t i = from; i < to; ++i) {
            const auto& c = cs[i];
            const bool up = c.close >= c.open;
            const TceColor col = candleColor(up, cfg.scheme);
            const float cx = (static_cast<float>(i - from) + 0.5f) * slot;

            emitLine(vLine, iLine, cx, priceY.yFor(c.high), cx, priceY.yFor(c.low), col);

            float yOpen  = priceY.yFor(c.open);
            float yClose = priceY.yFor(c.close);
            float yTop    = std::min(yOpen, yClose);
            float yBottom = std::max(yOpen, yClose);
            if (yBottom - yTop < 1.0f) yBottom = yTop + 1.0f;
            emitRect(vTri, iTri, cx - candleW * 0.5f, yTop, cx + candleW * 0.5f, yBottom, col);
        }
    } else if (cfg.seriesType == TCE_SERIES_OHLC_BAR) {
        const float tickW = std::max(2.0f, slot * 0.30f);
        for (size_t i = from; i < to; ++i) {
            const auto& c = cs[i];
            const bool up = c.close >= c.open;
            const TceColor col = candleColor(up, cfg.scheme);
            const float cx = (static_cast<float>(i - from) + 0.5f) * slot;
            // 세로 H/L
            emitLine(vLine, iLine, cx, priceY.yFor(c.high), cx, priceY.yFor(c.low), col);
            // 좌측 open tick
            float yo = priceY.yFor(c.open);
            emitLine(vLine, iLine, cx - tickW, yo, cx, yo, col);
            // 우측 close tick
            float yc = priceY.yFor(c.close);
            emitLine(vLine, iLine, cx, yc, cx + tickW, yc, col);
        }
    } else if (cfg.seriesType == TCE_SERIES_AREA) {
        const TceColor lineCol{0.30f, 0.65f, 1.0f, 1.0f};
        const TceColor fillCol{0.30f, 0.65f, 1.0f, 0.25f};
        const float bottomY = mainBottom - 8.0f;
        for (size_t i = from + 1; i < to; ++i) {
            float x1 = (static_cast<float>(i - 1 - from) + 0.5f) * slot;
            float x2 = (static_cast<float>(i - from) + 0.5f) * slot;
            float y1 = priceY.yFor(cs[i - 1].close);
            float y2 = priceY.yFor(cs[i].close);
            // 면적: 사각형 (x1,y1)-(x2,bottomY) — quad 두 개의 삼각형
            uint32_t base = static_cast<uint32_t>(vTri.size());
            vTri.push_back({x1, y1,      fillCol.r, fillCol.g, fillCol.b, fillCol.a});
            vTri.push_back({x2, y2,      fillCol.r, fillCol.g, fillCol.b, fillCol.a});
            vTri.push_back({x2, bottomY, fillCol.r, fillCol.g, fillCol.b, fillCol.a});
            vTri.push_back({x1, bottomY, fillCol.r, fillCol.g, fillCol.b, fillCol.a});
            iTri.push_back(base);     iTri.push_back(base + 1); iTri.push_back(base + 2);
            iTri.push_back(base);     iTri.push_back(base + 2); iTri.push_back(base + 3);
            // 라인
            emitLine(vLine, iLine, x1, y1, x2, y2, lineCol);
        }
    } else { // LINE
        const TceColor col{0.30f, 0.65f, 1.0f, 1.0f};
        for (size_t i = from + 1; i < to; ++i) {
            float x1 = (static_cast<float>(i - 1 - from) + 0.5f) * slot;
            float x2 = (static_cast<float>(i - from) + 0.5f) * slot;
            float y1 = priceY.yFor(cs[i - 1].close);
            float y2 = priceY.yFor(cs[i].close);
            emitLine(vLine, iLine, x1, y1, x2, y2, col);
        }
    }

    // Overlay 지표
    for (const auto& ov : overlays) {
        switch (ov.kind) {
        case TCE_IND_SMA:
            emitPolyline(vLine, iLine, sma(iSeries, ov.period), from, to, slot, priceY, ov.color);
            break;
        case TCE_IND_EMA:
            emitPolyline(vLine, iLine, ema(iSeries, ov.period), from, to, slot, priceY, ov.color);
            break;
        case TCE_IND_HMA:
            emitPolyline(vLine, iLine, hma(iSeries, ov.period), from, to, slot, priceY, ov.color);
            break;
        case TCE_IND_BOLLINGER: {
            auto bb = bollinger(iSeries, ov.period, ov.param);
            emitPolyline(vLine, iLine, bb.middle, from, to, slot, priceY, ov.color);
            emitPolyline(vLine, iLine, bb.upper,  from, to, slot, priceY, ov.color2);
            emitPolyline(vLine, iLine, bb.lower,  from, to, slot, priceY, ov.color2);
            break;
        }
        case TCE_IND_DONCHIAN: {
            auto dc = donchian(iSeries, ov.period > 0 ? ov.period : 20);
            // upper/lower는 ov.color2 (옅게), middle은 ov.color
            emitPolyline(vLine, iLine, dc.middle, from, to, slot, priceY, ov.color);
            emitPolyline(vLine, iLine, dc.upper,  from, to, slot, priceY, ov.color2);
            emitPolyline(vLine, iLine, dc.lower,  from, to, slot, priceY, ov.color2);
            break;
        }
        case TCE_IND_KELTNER: {
            const int eP = ov.period > 0 ? ov.period : 20;
            const int aP = ov.p2     > 0 ? ov.p2     : 10;
            const double m = ov.param > 0 ? ov.param : 2.0;
            auto k = keltner(iSeries, eP, aP, m);
            emitPolyline(vLine, iLine, k.middle, from, to, slot, priceY, ov.color);
            emitPolyline(vLine, iLine, k.upper,  from, to, slot, priceY, ov.color2);
            emitPolyline(vLine, iLine, k.lower,  from, to, slot, priceY, ov.color2);
            break;
        }
        case TCE_IND_VOLUME_PROFILE: {
            // Renko 모드는 미지원 (brick 시리즈가 시간 무관이라 volume 매핑이 의미 없음)
            if (cfg.seriesType == TCE_SERIES_RENKO) break;
            const int bins = ov.period > 1 ? ov.period : 24;
            const double wr = (ov.param > 0 && ov.param < 1) ? ov.param : 0.20;

            // 가시 가격 범위 — frame_builder 흡수 후 priceY 사용 (raw min/max보다 padded)
            // 메인 패널 plot 좌표
            const float plotX = 0.0f;          // mainPanel x 시작
            const float plotW2 = plotW;
            const float plotR  = plotX + plotW2;
            const float profL  = plotR - plotW2 * static_cast<float>(wr);

            // bin 누적 (volume 합산)
            std::vector<double> volPerBin(bins, 0.0);
            for (size_t i = from; i < to && i < cs.size(); ++i) {
                // 캔들의 typical price를 bin에 누적 — 정밀하려면 OHLC 분배도 가능하나 근사로 충분
                double tp = (cs[i].high + cs[i].low + cs[i].close) / 3.0;
                double nv = priceY.normalize(tp);
                if (priceY.maxP <= priceY.minP) continue;
                double t = (nv - priceY.minP) / (priceY.maxP - priceY.minP);
                int bi = static_cast<int>(std::floor(t * bins));
                if (bi < 0) bi = 0; if (bi >= bins) bi = bins - 1;
                volPerBin[bi] += cs[i].volume;
            }
            double maxVol = 0;
            int pocBin = 0;
            double totalVol = 0;
            for (int b = 0; b < bins; ++b) {
                if (volPerBin[b] > maxVol) { maxVol = volPerBin[b]; pocBin = b; }
                totalVol += volPerBin[b];
            }
            if (maxVol <= 0 || totalVol <= 0) break;

            // 막대 그리기 (alpha 0.30 강제)
            TceColor barCol = ov.color; barCol.a = 0.30f;
            const float binH = (priceY.bottom - priceY.top) / static_cast<float>(bins);
            for (int b = 0; b < bins; ++b) {
                if (volPerBin[b] <= 0) continue;
                double w = volPerBin[b] / maxVol;
                float yTop = priceY.bottom - static_cast<float>(b + 1) * binH;
                float yBot = priceY.bottom - static_cast<float>(b) * binH;
                float xR = plotR;
                float xL = plotR - (plotR - profL) * static_cast<float>(w);
                emitRect(vTri, iTri, xL, yTop + 0.5f, xR, yBot - 0.5f, barCol);
            }

            // POC 가로선 (가장 진한)
            TceColor pocCol = ov.color2;
            float pocY = priceY.bottom - (static_cast<float>(pocBin) + 0.5f) * binH;
            emitLine(vLine, iLine, profL, pocY, plotR, pocY, pocCol);

            // PanelLayout 결과 — bin index를 raw price로 환산해 라벨에 노출.
            // priceY.minP/maxP는 normalized 공간이므로 layoutDenormalizePrice로 raw 복원.
            auto binNvToPrice = [&](double binIdxFloat) {
                double t = binIdxFloat / static_cast<double>(bins);
                double nv = priceY.minP + t * (priceY.maxP - priceY.minP);
                return layoutDenormalizePrice(layout, nv);
            };
            layout.volumeProfile.present  = true;
            layout.volumeProfile.pocPrice = binNvToPrice(pocBin + 0.5);

            // VAH/VAL — POC bin부터 위/아래로 확장하며 누적 70% 도달 시점 양 끝 bin
            double cum = volPerBin[pocBin];
            int vah = pocBin, val = pocBin;
            const double target = totalVol * 0.70;
            while (cum < target && (vah < bins - 1 || val > 0)) {
                double upVol = (vah < bins - 1) ? volPerBin[vah + 1] : -1;
                double dnVol = (val > 0) ? volPerBin[val - 1] : -1;
                if (upVol >= dnVol && vah < bins - 1) {
                    ++vah; cum += volPerBin[vah];
                } else if (val > 0) {
                    --val; cum += volPerBin[val];
                } else break;
            }
            TceColor vaCol = pocCol; vaCol.a *= 0.5f;
            float vahY = priceY.bottom - (static_cast<float>(vah + 1)) * binH;
            float valY = priceY.bottom - (static_cast<float>(val)) * binH;
            emitLine(vLine, iLine, profL, vahY, plotR, vahY, vaCol);
            emitLine(vLine, iLine, profL, valY, plotR, valY, vaCol);
            // VAH = vah bin 상단(=vah+1), VAL = val bin 하단(=val)
            layout.volumeProfile.vahPrice = binNvToPrice(static_cast<double>(vah + 1));
            layout.volumeProfile.valPrice = binNvToPrice(static_cast<double>(val));
            break;
        }
        case TCE_IND_ZIGZAG: {
            // sparse swing point들을 직선 연결 — viewport [from, to) 양 끝의 segment가 끊기지 않도록
            // from 이전 마지막 swing 1개와 to 이후 첫 swing 1개를 확장 흡수.
            const double dev = ov.param > 0 ? ov.param : 5.0;
            auto zz = zigzag(iSeries, dev);
            std::optional<float> px, py;
            // from 이전 마지막 swing 검색 (있으면 그 점부터 line 시작)
            for (size_t k = from; k > 0; --k) {
                size_t i = k - 1;
                if (i < zz.size() && zz[i]) {
                    float x = (static_cast<float>(i) - static_cast<float>(from) + 0.5f) * slot;
                    px = x; py = priceY.yFor(*zz[i]);
                    break;
                }
            }
            for (size_t i = from; i < to && i < zz.size(); ++i) {
                if (!zz[i]) continue;
                float x = (static_cast<float>(i - from) + 0.5f) * slot;
                float y = priceY.yFor(*zz[i]);
                if (px && py) emitLine(vLine, iLine, *px, *py, x, y, ov.color);
                px = x; py = y;
            }
            // to 이후 첫 swing 검색 (있으면 마지막 viewport swing → 그 점까지 line)
            if (px && py) {
                for (size_t i = to; i < zz.size(); ++i) {
                    if (zz[i]) {
                        float x = (static_cast<float>(i) - static_cast<float>(from) + 0.5f) * slot;
                        float y = priceY.yFor(*zz[i]);
                        emitLine(vLine, iLine, *px, *py, x, y, ov.color);
                        break;
                    }
                }
            }
            break;
        }
        case TCE_IND_ICHIMOKU: {
            const int tenkan = ov.period > 0 ? ov.period : 9;
            const int kijun  = ov.p2     > 0 ? ov.p2     : 26;
            const int senkB  = ov.p3     > 0 ? ov.p3     : 52;
            const int disp   = ov.p4     > 0 ? ov.p4     : 26;
            auto ic = ichimoku(iSeries, tenkan, kijun, senkB, disp);
            emitPolyline(vLine, iLine, ic.tenkan, from, to, slot, priceY, ov.color);
            TceColor kijunCol{ov.color2.r, ov.color2.g, ov.color2.b, ov.color2.a};
            emitPolyline(vLine, iLine, ic.kijun,  from, to, slot, priceY, kijunCol);
            std::vector<std::optional<double>> aA(cs.size()), aB(cs.size());
            for (size_t i = 0; i < cs.size(); ++i) {
                if (i < ic.senkouA.size()) aA[i] = ic.senkouA[i];
                if (i < ic.senkouB.size()) aB[i] = ic.senkouB[i];
            }
            TceColor sA{0.20f, 0.85f, 0.45f, 0.85f};
            TceColor sB{0.95f, 0.45f, 0.45f, 0.85f};
            emitPolyline(vLine, iLine, aA, from, to, slot, priceY, sA);
            emitPolyline(vLine, iLine, aB, from, to, slot, priceY, sB);
            TceColor ck{0.65f, 0.45f, 0.95f, 0.85f};
            emitPolyline(vLine, iLine, ic.chikou, from, to, slot, priceY, ck);
            break;
        }
        case TCE_IND_PSAR: {
            const double step    = ov.param  > 0 ? ov.param  : 0.02;
            const double maxStep = ov.param2 > 0 ? ov.param2 : 0.20;
            auto p = psar(iSeries, step, maxStep);
            const float r = std::max(2.0f, slot * 0.20f);
            for (size_t i = from; i < to; ++i) {
                if (!p[i]) continue;
                float cx = (static_cast<float>(i - from) + 0.5f) * slot;
                float cy = priceY.yFor(*p[i]);
                emitRect(vTri, iTri, cx - r, cy - r, cx + r, cy + r, ov.color);
            }
            break;
        }
        case TCE_IND_SUPERTREND: {
            auto st = superTrend(iSeries, ov.period > 0 ? ov.period : 10,
                                 ov.param > 0 ? ov.param : 3.0);
            // 방향 따라 색 분기 (양봉 색/음봉 색)
            std::optional<float> px, py;
            int prevDir = 0;
            for (size_t i = from; i < to; ++i) {
                if (!st.line[i]) { px.reset(); py.reset(); continue; }
                float x = (static_cast<float>(i - from) + 0.5f) * slot;
                float y = priceY.yFor(*st.line[i]);
                if (px && py && st.direction[i] == prevDir) {
                    TceColor col = (st.direction[i] == 1)
                        ? candleColor(true,  cfg.scheme)
                        : candleColor(false, cfg.scheme);
                    emitLine(vLine, iLine, *px, *py, x, y, col);
                }
                prevDir = st.direction[i];
                px = x; py = y;
            }
            break;
        }
        case TCE_IND_VWAP: {
            if (ov.param > 0) {
                auto vb = vwapBands(iSeries, cfg.sessionOffsetSeconds, ov.param);
                emitPolyline(vLine, iLine, vb.middle, from, to, slot, priceY, ov.color);
                emitPolyline(vLine, iLine, vb.upper,  from, to, slot, priceY, ov.color2);
                emitPolyline(vLine, iLine, vb.lower,  from, to, slot, priceY, ov.color2);
            } else {
                emitPolyline(vLine, iLine,
                             vwap(iSeries, cfg.sessionOffsetSeconds),
                             from, to, slot, priceY, ov.color);
            }
            break;
        }
        case TCE_IND_PIVOT_STANDARD:
        case TCE_IND_PIVOT_FIBONACCI:
        case TCE_IND_PIVOT_CAMARILLA: {
            PivotKind k =
                (ov.kind == TCE_IND_PIVOT_STANDARD)  ? PivotKind::Standard :
                (ov.kind == TCE_IND_PIVOT_FIBONACCI) ? PivotKind::Fibonacci :
                                                        PivotKind::Camarilla;
            auto p = pivot(iSeries, k, cfg.sessionOffsetSeconds);
            // P는 ov.color, R/S는 ov.color2 (옅게)
            TceColor pCol = ov.color;
            TceColor rsCol = ov.color2;
            emitPolyline(vLine, iLine, p.p,  from, to, slot, priceY, pCol);
            emitPolyline(vLine, iLine, p.r1, from, to, slot, priceY, rsCol);
            emitPolyline(vLine, iLine, p.r2, from, to, slot, priceY, rsCol);
            emitPolyline(vLine, iLine, p.r3, from, to, slot, priceY, rsCol);
            emitPolyline(vLine, iLine, p.s1, from, to, slot, priceY, rsCol);
            emitPolyline(vLine, iLine, p.s2, from, to, slot, priceY, rsCol);
            emitPolyline(vLine, iLine, p.s3, from, to, slot, priceY, rsCol);
            break;
        }
        default: break;
        }
    }

    // 메인 패널 메시 추가
    if (!vTri.empty())  out.addMesh(std::move(vTri),  std::move(iTri),  PRIM_TRIANGLES);
    if (!vLine.empty()) out.addMesh(std::move(vLine), std::move(iLine), PRIM_LINES);

    // ===== Subpanel + Volume =====
    int panelIdx = 0;
    auto panelTopY = [&](int idx) -> float {
        return subBlockTop + idx * perSubH;
    };
    auto panelBotY = [&](int idx) -> float {
        return subBlockTop + (idx + 1) * perSubH - PANEL_GAP * 0.5f;
    };

    for (const auto& sp : subpanels) {
        std::vector<TceVertex> vT, vL;
        std::vector<uint32_t>  iT, iL;
        const float top = panelTopY(panelIdx);
        const float bot = panelBotY(panelIdx);
        buildSubpanel(vT, iT, vL, iL, iSeries, from, to, slot, top, bot, sp);
        if (!vT.empty()) out.addMesh(std::move(vT), std::move(iT), PRIM_TRIANGLES);
        if (!vL.empty()) out.addMesh(std::move(vL), std::move(iL), PRIM_LINES);
        layout.subpanels.push_back(Rect{0, top, plotW, bot - top});
        ++panelIdx;
    }

    // Volume
    if (cfg.volumeVisible) {
        double maxVol = series.maxVolumeInRange(from, to);
        std::vector<TceVertex> vV;
        std::vector<uint32_t>  iV;
        if (maxVol > 0.0) {
            const float bottomY = panelBotY(panelIdx);
            const float topY    = panelTopY(panelIdx);
            const float volW    = std::max(1.0f, slot * 0.65f);
            for (size_t i = from; i < to; ++i) {
                const auto& c = cs[i];
                const bool up = c.close >= c.open;
                const TceColor col = candleColor(up, cfg.scheme);
                float cx = (static_cast<float>(i - from) + 0.5f) * slot;
                float h = static_cast<float>((c.volume / maxVol) * (bottomY - topY));
                emitRect(vV, iV, cx - volW*0.5f, bottomY - h, cx + volW*0.5f, bottomY, col);
            }
        }
        if (!vV.empty()) out.addMesh(std::move(vV), std::move(iV), PRIM_TRIANGLES);
        const float top = panelTopY(panelIdx);
        const float bot = panelBotY(panelIdx);
        layout.volumePanel = Rect{0, top, plotW, bot - top};
    }

    // ===== 크로스헤어 =====
    if (cross.visible) {
        std::vector<TceVertex> v;
        std::vector<uint32_t>  i;
        const TceColor c{0.55f, 0.60f, 0.70f, 0.65f};
        emitLine(v, i, cross.x, 0, cross.x, H, c);
        emitLine(v, i, 0, cross.y, W, cross.y, c);
        out.addMesh(std::move(v), std::move(i), PRIM_LINES);
    }
}

} // namespace tce

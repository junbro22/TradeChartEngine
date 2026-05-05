#include "renderer/frame_builder.h"
#include "data/series.h"
#include "viewport/viewport.h"
#include "indicator/ma.h"
#include "indicator/ema.h"
#include "indicator/rsi.h"
#include "indicator/macd.h"
#include "indicator/bollinger.h"
#include "indicator/stochastic.h"
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
#include <algorithm>
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
    float yFor(double v) const {
        if (maxP == minP) return (top + bottom) * 0.5f;
        double t = (v - minP) / (maxP - minP);
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

// 보조 패널 한 개에 대한 스켈레톤
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
                         PanelLayout& layout) {
    out.clear();
    layout = PanelLayout{};

    const auto& cs = series.candles();
    if (cs.empty()) return;

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
    YMap priceY{
        mainTop + 8.0f,
        mainBottom - 8.0f,
        series.minLowInRange(from, to),
        series.maxHighInRange(from, to)
    };
    // BB가 메인보다 위/아래로 나갈 수 있어 그것까지 포함
    for (const auto& ov : overlays) {
        if (ov.kind == TCE_IND_BOLLINGER) {
            auto bb = bollinger(series, ov.period, ov.param);
            for (size_t i = from; i < to; ++i) {
                if (bb.upper[i]) priceY.maxP = std::max(priceY.maxP, *bb.upper[i]);
                if (bb.lower[i]) priceY.minP = std::min(priceY.minP, *bb.lower[i]);
            }
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
    layout.subpanels.clear();

    std::vector<TceVertex> vTri, vLine;
    std::vector<uint32_t>  iTri, iLine;

    // 캔들 / 라인
    if (cfg.seriesType == TCE_SERIES_CANDLE) {
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
    } else {
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
            emitPolyline(vLine, iLine, sma(series, ov.period), from, to, slot, priceY, ov.color);
            break;
        case TCE_IND_EMA:
            emitPolyline(vLine, iLine, ema(series, ov.period), from, to, slot, priceY, ov.color);
            break;
        case TCE_IND_BOLLINGER: {
            auto bb = bollinger(series, ov.period, ov.param);
            emitPolyline(vLine, iLine, bb.middle, from, to, slot, priceY, ov.color);
            emitPolyline(vLine, iLine, bb.upper,  from, to, slot, priceY, ov.color2);
            emitPolyline(vLine, iLine, bb.lower,  from, to, slot, priceY, ov.color2);
            break;
        }
        case TCE_IND_ICHIMOKU: {
            auto ic = ichimoku(series, 9, 26, 52, 26);
            emitPolyline(vLine, iLine, ic.tenkan, from, to, slot, priceY, ov.color);
            TceColor kijunCol{ov.color2.r, ov.color2.g, ov.color2.b, ov.color2.a};
            emitPolyline(vLine, iLine, ic.kijun,  from, to, slot, priceY, kijunCol);
            // senkouA / senkouB는 displacement 만큼 미래로 — 가시 범위 안만 그림
            // 인덱스가 series.size + displacement까지 있으므로 to 범위 한도 내에서 그림
            std::vector<std::optional<double>> aA(cs.size()), aB(cs.size());
            for (size_t i = 0; i < cs.size(); ++i) {
                if (i < ic.senkouA.size()) aA[i] = ic.senkouA[i];
                if (i < ic.senkouB.size()) aB[i] = ic.senkouB[i];
            }
            TceColor sA{0.20f, 0.85f, 0.45f, 0.85f};
            TceColor sB{0.95f, 0.45f, 0.45f, 0.85f};
            emitPolyline(vLine, iLine, aA, from, to, slot, priceY, sA);
            emitPolyline(vLine, iLine, aB, from, to, slot, priceY, sB);
            // 후행스팬 (chikou)
            TceColor ck{0.65f, 0.45f, 0.95f, 0.85f};
            emitPolyline(vLine, iLine, ic.chikou, from, to, slot, priceY, ck);
            break;
        }
        case TCE_IND_PSAR: {
            auto p = psar(series, ov.param > 0 ? ov.param : 0.02, 0.2);
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
            auto st = superTrend(series, ov.period > 0 ? ov.period : 10,
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
            emitPolyline(vLine, iLine, vwap(series), from, to, slot, priceY, ov.color);
            break;
        }
        case TCE_IND_PIVOT_STANDARD:
        case TCE_IND_PIVOT_FIBONACCI:
        case TCE_IND_PIVOT_CAMARILLA: {
            PivotKind k =
                (ov.kind == TCE_IND_PIVOT_STANDARD)  ? PivotKind::Standard :
                (ov.kind == TCE_IND_PIVOT_FIBONACCI) ? PivotKind::Fibonacci :
                                                        PivotKind::Camarilla;
            auto p = pivot(series, k);
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
        buildSubpanel(vT, iT, vL, iL, series, from, to, slot, top, bot, sp);
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

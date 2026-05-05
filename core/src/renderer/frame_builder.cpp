#include "renderer/frame_builder.h"
#include "data/series.h"
#include "viewport/viewport.h"
#include "indicator/ma.h"
#include "indicator/ema.h"
#include <algorithm>

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

// 가격 패널이 차지하는 세로 비율 (volume panel 보일 때)
constexpr float PRICE_PANEL_RATIO_WITH_VOL = 0.78f;
constexpr float VOLUME_PANEL_RATIO         = 0.20f;
constexpr float PANEL_GAP                  = 0.02f;

struct YMap {
    float top, bottom;     // 화면 px
    double minP, maxP;
    float yFor(double price) const {
        if (maxP == minP) return (top + bottom) * 0.5f;
        double t = (price - minP) / (maxP - minP);
        return static_cast<float>(bottom - t * (bottom - top));
    }
};

void emitLineSegment(std::vector<TceVertex>& v,
                     std::vector<uint32_t>&  idx,
                     float x1, float y1, float x2, float y2,
                     TceColor c) {
    uint32_t base = static_cast<uint32_t>(v.size());
    v.push_back({x1, y1, c.r, c.g, c.b, c.a});
    v.push_back({x2, y2, c.r, c.g, c.b, c.a});
    idx.push_back(base);
    idx.push_back(base + 1);
}

void emitRect(std::vector<TceVertex>& v,
              std::vector<uint32_t>&  idx,
              float x1, float y1, float x2, float y2,
              TceColor c) {
    uint32_t base = static_cast<uint32_t>(v.size());
    v.push_back({x1, y1, c.r, c.g, c.b, c.a});
    v.push_back({x2, y1, c.r, c.g, c.b, c.a});
    v.push_back({x2, y2, c.r, c.g, c.b, c.a});
    v.push_back({x1, y2, c.r, c.g, c.b, c.a});
    idx.push_back(base);     idx.push_back(base + 1); idx.push_back(base + 2);
    idx.push_back(base);     idx.push_back(base + 2); idx.push_back(base + 3);
}

TceColor candleColor(bool isUp, TceColorScheme scheme) {
    if (scheme == TCE_SCHEME_KOREA) {
        return isUp ? TceColor{0.93f, 0.27f, 0.27f, 1.0f}
                    : TceColor{0.27f, 0.53f, 1.00f, 1.0f};
    }
    return isUp ? TceColor{0.16f, 0.74f, 0.45f, 1.0f}
                : TceColor{0.93f, 0.27f, 0.27f, 1.0f};
}

} // namespace

void FrameBuilder::build(const Series& series,
                         const Viewport& vp,
                         const ChartConfig& cfg,
                         const std::vector<IndicatorSpec>& indicators,
                         const CrosshairState& cross,
                         FrameOutput& out) {
    out.clear();

    const auto& cs = series.candles();
    if (cs.empty()) return;

    size_t from, to;
    vp.rangeFor(cs.size(), from, to);
    if (to <= from) return;

    const float W = vp.width();
    const float H = vp.height();
    const float slot = vp.slotWidth();

    // 패널 영역
    const float pricePanelTop    = 0.0f;
    const float pricePanelBottom = cfg.volumeVisible
        ? H * PRICE_PANEL_RATIO_WITH_VOL
        : H;
    const float volPanelTop    = pricePanelBottom + H * PANEL_GAP;
    const float volPanelBottom = H;

    // 가격 Y 매핑
    YMap priceY{
        pricePanelTop + 8.0f,
        pricePanelBottom - 8.0f,
        series.minLowInRange(from, to),
        series.maxHighInRange(from, to)
    };
    // 약간의 여유
    {
        double pad = (priceY.maxP - priceY.minP) * 0.05;
        priceY.minP -= pad;
        priceY.maxP += pad;
    }

    // ===== 캔들 또는 라인 =====
    std::vector<TceVertex> v_main;
    std::vector<uint32_t>  i_main;
    std::vector<TceVertex> v_main_lines;
    std::vector<uint32_t>  i_main_lines;

    if (cfg.seriesType == TCE_SERIES_CANDLE) {
        const float candleW = std::max(1.0f, slot * 0.65f);
        for (size_t i = from; i < to; ++i) {
            const auto& c = cs[i];
            const bool up = c.close >= c.open;
            const TceColor col = candleColor(up, cfg.scheme);
            const float cx = (static_cast<float>(i - from) + 0.5f) * slot;

            // 심지 (line)
            emitLineSegment(v_main_lines, i_main_lines,
                            cx, priceY.yFor(c.high),
                            cx, priceY.yFor(c.low), col);

            // 몸통 (rect)
            float yOpen  = priceY.yFor(c.open);
            float yClose = priceY.yFor(c.close);
            float yTop    = std::min(yOpen, yClose);
            float yBottom = std::max(yOpen, yClose);
            if (yBottom - yTop < 1.0f) yBottom = yTop + 1.0f;
            emitRect(v_main, i_main,
                     cx - candleW * 0.5f, yTop,
                     cx + candleW * 0.5f, yBottom, col);
        }
    } else {
        // 라인 차트 — 종가 연결
        const TceColor col{0.30f, 0.65f, 1.0f, 1.0f};
        for (size_t i = from + 1; i < to; ++i) {
            float x1 = (static_cast<float>(i - 1 - from) + 0.5f) * slot;
            float x2 = (static_cast<float>(i - from) + 0.5f) * slot;
            float y1 = priceY.yFor(cs[i - 1].close);
            float y2 = priceY.yFor(cs[i].close);
            emitLineSegment(v_main_lines, i_main_lines, x1, y1, x2, y2, col);
        }
    }

    // ===== 지표 =====
    for (const auto& spec : indicators) {
        std::vector<std::optional<double>> values;
        if (spec.kind == TCE_IND_SMA)      values = sma(series, spec.period);
        else if (spec.kind == TCE_IND_EMA) values = ema(series, spec.period);
        else continue;

        std::optional<float> prevX;
        std::optional<float> prevY;
        for (size_t i = from; i < to; ++i) {
            if (!values[i]) { prevX.reset(); prevY.reset(); continue; }
            float x = (static_cast<float>(i - from) + 0.5f) * slot;
            float y = priceY.yFor(*values[i]);
            if (prevX && prevY) {
                emitLineSegment(v_main_lines, i_main_lines,
                                *prevX, *prevY, x, y, spec.color);
            }
            prevX = x;
            prevY = y;
        }
    }

    // ===== 거래량 패널 =====
    std::vector<TceVertex> v_vol;
    std::vector<uint32_t>  i_vol;
    if (cfg.volumeVisible) {
        double maxVol = series.maxVolumeInRange(from, to);
        if (maxVol > 0.0) {
            const float bottomY = volPanelBottom - 4.0f;
            const float topY    = volPanelTop + 4.0f;
            const float volW    = std::max(1.0f, slot * 0.65f);
            for (size_t i = from; i < to; ++i) {
                const auto& c = cs[i];
                const bool up = c.close >= c.open;
                const TceColor col = candleColor(up, cfg.scheme);
                float cx = (static_cast<float>(i - from) + 0.5f) * slot;
                float h = static_cast<float>((c.volume / maxVol) * (bottomY - topY));
                emitRect(v_vol, i_vol,
                         cx - volW * 0.5f, bottomY - h,
                         cx + volW * 0.5f, bottomY, col);
            }
        }
    }

    // ===== 크로스헤어 =====
    std::vector<TceVertex> v_cross;
    std::vector<uint32_t>  i_cross;
    if (cross.visible) {
        const TceColor c{0.55f, 0.60f, 0.70f, 0.65f};
        emitLineSegment(v_cross, i_cross, cross.x, 0.0f, cross.x, H, c);
        emitLineSegment(v_cross, i_cross, 0.0f, cross.y, W, cross.y, c);
    }

    if (!v_main.empty())       out.addMesh(std::move(v_main),       std::move(i_main),       PRIM_TRIANGLES);
    if (!v_main_lines.empty()) out.addMesh(std::move(v_main_lines), std::move(i_main_lines), PRIM_LINES);
    if (!v_vol.empty())        out.addMesh(std::move(v_vol),        std::move(i_vol),        PRIM_TRIANGLES);
    if (!v_cross.empty())      out.addMesh(std::move(v_cross),      std::move(i_cross),      PRIM_LINES);
}

} // namespace tce

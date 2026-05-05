#include "drawing/drawing_renderer.h"
#include "data/series.h"
#include "viewport/viewport.h"
#include <algorithm>
#include <cstdio>
#include <cmath>

namespace tce {

namespace {

constexpr int PRIM_LINES     = 1;
constexpr int PRIM_TRIANGLES = 0;

// 도메인 timestamp → 화면 X (선형 보간 사이 캔들)
float xForTimestamp(const Series& series, const Viewport& vp,
                    const PanelLayout& layout, double ts) {
    const auto& cs = series.candles();
    if (cs.empty()) return 0.0f;
    size_t from, to; vp.rangeFor(cs.size(), from, to);
    const float plotW = layout.plot.w;
    const float slot = plotW / static_cast<float>(vp.visibleCount());

    // ts가 cs[0..].timestamp 내에 있으면 선형 보간
    if (ts <= cs.front().timestamp) {
        size_t idx = 0;
        return (static_cast<float>(idx) - from + 0.5f) * slot;
    }
    if (ts >= cs.back().timestamp) {
        size_t idx = cs.size() - 1;
        return (static_cast<float>(idx) - from + 0.5f) * slot;
    }
    // binary search
    auto it = std::lower_bound(cs.begin(), cs.end(), ts,
        [](const TceCandle& c, double v) { return c.timestamp < v; });
    size_t hi = static_cast<size_t>(it - cs.begin());
    size_t lo = hi - 1;
    double t1 = cs[lo].timestamp, t2 = cs[hi].timestamp;
    double frac = (t2 != t1) ? (ts - t1) / (t2 - t1) : 0.0;
    double idx = (double)lo + frac;
    return static_cast<float>((idx - (double)from + 0.5) * slot);
}

float yForPrice(const PanelLayout& layout, double price) {
    const float top = layout.plot.y;
    const float bot = layout.plot.y + layout.plot.h;
    if (layout.priceMax == layout.priceMin) return (top + bot) * 0.5f;
    double t = (price - layout.priceMin) / (layout.priceMax - layout.priceMin);
    return static_cast<float>(bot - t * (bot - top));
}

void emitLine(std::vector<TceVertex>& v, std::vector<uint32_t>& idx,
              float x1, float y1, float x2, float y2, TceColor c) {
    uint32_t base = static_cast<uint32_t>(v.size());
    v.push_back({x1, y1, c.r, c.g, c.b, c.a});
    v.push_back({x2, y2, c.r, c.g, c.b, c.a});
    idx.push_back(base); idx.push_back(base + 1);
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

std::string fmtPriceShort(double p) {
    char buf[24];
    if (std::fabs(p) >= 1000) std::snprintf(buf, sizeof(buf), "%.0f", p);
    else                      std::snprintf(buf, sizeof(buf), "%.2f", p);
    return std::string(buf);
}

std::string fmtPercent(double v) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%+.2f%%", v * 100.0);
    return std::string(buf);
}

} // namespace

void DrawingRenderer::build(const Series& series,
                            const Viewport& vp,
                            const PanelLayout& layout,
                            const std::vector<Drawing>& drawings,
                            FrameOutput& meshOut,
                            LabelOutput& labelOut) {
    if (drawings.empty()) return;

    std::vector<TceVertex> vL;  std::vector<uint32_t> iL;
    std::vector<TceVertex> vT;  std::vector<uint32_t> iT;

    const float plotL = layout.plot.x;
    const float plotR = layout.plot.x + layout.plot.w;
    const float plotT = layout.plot.y;
    const float plotB = layout.plot.y + layout.plot.h;

    for (const auto& d : drawings) {
        if (d.points.empty()) continue;

        switch (d.kind) {
        case TCE_DRAW_HORIZONTAL: {
            float y = yForPrice(layout, d.points[0].price);
            if (y < plotT || y > plotB) break;
            emitLine(vL, iL, plotL, y, plotR, y, d.color);
            // 가격 라벨 (우측 priceAxis 안)
            float lx = layout.priceAxis.x + layout.priceAxis.w * 0.5f;
            labelOut.add(fmtPriceShort(d.points[0].price),
                         lx, y,
                         TCE_ANCHOR_CENTER_CENTER, TCE_LABEL_PRICE_AXIS,
                         d.color, {0.13f, 0.16f, 0.20f, 0.95f});
            break;
        }
        case TCE_DRAW_VERTICAL: {
            float x = xForTimestamp(series, vp, layout, d.points[0].timestamp);
            if (x < plotL || x > plotR) break;
            emitLine(vL, iL, x, plotT, x, plotB, d.color);
            break;
        }
        case TCE_DRAW_TRENDLINE: {
            if (d.points.size() < 2) break;
            float x1 = xForTimestamp(series, vp, layout, d.points[0].timestamp);
            float y1 = yForPrice(layout, d.points[0].price);
            float x2 = xForTimestamp(series, vp, layout, d.points[1].timestamp);
            float y2 = yForPrice(layout, d.points[1].price);
            emitLine(vL, iL, x1, y1, x2, y2, d.color);
            break;
        }
        case TCE_DRAW_RECTANGLE: {
            if (d.points.size() < 2) break;
            float x1 = xForTimestamp(series, vp, layout, d.points[0].timestamp);
            float y1 = yForPrice(layout, d.points[0].price);
            float x2 = xForTimestamp(series, vp, layout, d.points[1].timestamp);
            float y2 = yForPrice(layout, d.points[1].price);
            TceColor fill{d.color.r, d.color.g, d.color.b, 0.18f};
            emitRect(vT, iT, std::min(x1, x2), std::min(y1, y2),
                              std::max(x1, x2), std::max(y1, y2), fill);
            // 외곽 라인
            emitLine(vL, iL, x1, y1, x2, y1, d.color);
            emitLine(vL, iL, x2, y1, x2, y2, d.color);
            emitLine(vL, iL, x2, y2, x1, y2, d.color);
            emitLine(vL, iL, x1, y2, x1, y1, d.color);
            break;
        }
        case TCE_DRAW_FIB_RETRACEMENT: {
            if (d.points.size() < 2) break;
            double p1 = d.points[0].price, p2 = d.points[1].price;
            // p1이 시작(0%), p2가 100%
            const double levels[] = {0.0, 0.236, 0.382, 0.5, 0.618, 0.786, 1.0};
            for (double lv : levels) {
                double price = p1 + (p2 - p1) * lv;
                float y = yForPrice(layout, price);
                if (y < plotT || y > plotB) continue;
                TceColor c = d.color;
                c.a *= (lv == 0.0 || lv == 1.0) ? 1.0f : 0.6f;
                emitLine(vL, iL, plotL, y, plotR, y, c);
                // 라벨
                char buf[24];
                std::snprintf(buf, sizeof(buf), "%.1f%%  %s",
                              lv * 100.0, fmtPriceShort(price).c_str());
                float lx = layout.priceAxis.x + layout.priceAxis.w * 0.5f;
                labelOut.add(std::string(buf), lx, y,
                             TCE_ANCHOR_CENTER_CENTER, TCE_LABEL_PRICE_AXIS,
                             c, {0.13f, 0.16f, 0.20f, 0.95f});
            }
            break;
        }
        case TCE_DRAW_MEASURE: {
            if (d.points.size() < 2) break;
            float x1 = xForTimestamp(series, vp, layout, d.points[0].timestamp);
            float y1 = yForPrice(layout, d.points[0].price);
            float x2 = xForTimestamp(series, vp, layout, d.points[1].timestamp);
            float y2 = yForPrice(layout, d.points[1].price);
            // 박스 + 대각선
            TceColor fill{d.color.r, d.color.g, d.color.b, 0.12f};
            emitRect(vT, iT, std::min(x1, x2), std::min(y1, y2),
                              std::max(x1, x2), std::max(y1, y2), fill);
            emitLine(vL, iL, x1, y1, x2, y2, d.color);

            // 라벨: 가격 차이 + %
            double dp = d.points[1].price - d.points[0].price;
            double pct = (d.points[0].price != 0)
                ? (d.points[1].price / d.points[0].price - 1.0) : 0.0;
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%s  %s",
                          fmtPriceShort(dp).c_str(), fmtPercent(pct).c_str());
            float midX = (x1 + x2) * 0.5f;
            float midY = std::min(y1, y2) - 4.0f;
            labelOut.add(std::string(buf), midX, midY,
                         TCE_ANCHOR_CENTER_CENTER, TCE_LABEL_CROSSHAIR_PRICE,
                         d.color, {0.13f, 0.16f, 0.20f, 0.95f});
            break;
        }
        }
    }

    if (!vL.empty()) meshOut.addMesh(std::move(vL), std::move(iL), PRIM_LINES);
    if (!vT.empty()) meshOut.addMesh(std::move(vT), std::move(iT), PRIM_TRIANGLES);
}

} // namespace tce

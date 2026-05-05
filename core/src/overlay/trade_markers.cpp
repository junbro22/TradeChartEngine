#include "overlay/trade_markers.h"
#include "data/series.h"
#include "viewport/viewport.h"
#include <algorithm>
#include <cstdio>
#include <cmath>

namespace tce {

int TradeMarkerStore::add(double ts, double price, bool isBuy, double qty) {
    TradeMarker m{nextId_++, ts, price, isBuy, qty};
    items_.push_back(m);
    return m.id;
}

void TradeMarkerStore::remove(int id) {
    items_.erase(std::remove_if(items_.begin(), items_.end(),
        [&](const TradeMarker& m) { return m.id == id; }), items_.end());
}

void TradeMarkerStore::clear() { items_.clear(); }

int AlertLineStore::add(double price, TceColor color) {
    AlertLine a{nextId_++, price, color};
    items_.push_back(a);
    return a.id;
}

bool AlertLineStore::updatePrice(int id, double price) {
    auto it = std::find_if(items_.begin(), items_.end(),
        [&](const AlertLine& a) { return a.id == id; });
    if (it == items_.end()) return false;
    it->price = price;
    return true;
}

void AlertLineStore::remove(int id) {
    items_.erase(std::remove_if(items_.begin(), items_.end(),
        [&](const AlertLine& a) { return a.id == id; }), items_.end());
}

void AlertLineStore::clear() { items_.clear(); }

namespace {

constexpr int PRIM_LINES     = 1;
constexpr int PRIM_TRIANGLES = 0;

float xForTimestamp(const Series& s, const Viewport& vp,
                    const PanelLayout& layout, double ts) {
    const auto& cs = s.candles();
    if (cs.empty()) return 0.0f;
    size_t from, to; vp.rangeFor(cs.size(), from, to);
    const float plotW = layout.plot.w;
    const float slot = plotW / static_cast<float>(vp.visibleCount());
    if (ts <= cs.front().timestamp) return (-(double)from + 0.5) * slot;
    if (ts >= cs.back().timestamp)  return ((double)cs.size() - 1 - from + 0.5) * slot;
    auto it = std::lower_bound(cs.begin(), cs.end(), ts,
        [](const TceCandle& c, double v) { return c.timestamp < v; });
    size_t hi = it - cs.begin();
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

void emitTriangle(std::vector<TceVertex>& v, std::vector<uint32_t>& idx,
                  float x1, float y1, float x2, float y2, float x3, float y3,
                  TceColor c) {
    uint32_t base = static_cast<uint32_t>(v.size());
    v.push_back({x1, y1, c.r, c.g, c.b, c.a});
    v.push_back({x2, y2, c.r, c.g, c.b, c.a});
    v.push_back({x3, y3, c.r, c.g, c.b, c.a});
    idx.push_back(base); idx.push_back(base + 1); idx.push_back(base + 2);
}

void emitLine(std::vector<TceVertex>& v, std::vector<uint32_t>& idx,
              float x1, float y1, float x2, float y2, TceColor c) {
    uint32_t base = static_cast<uint32_t>(v.size());
    v.push_back({x1, y1, c.r, c.g, c.b, c.a});
    v.push_back({x2, y2, c.r, c.g, c.b, c.a});
    idx.push_back(base); idx.push_back(base + 1);
}

TceColor markerColor(bool isBuy, TceColorScheme scheme) {
    if (scheme == TCE_SCHEME_KOREA) {
        return isBuy ? TceColor{0.93f, 0.27f, 0.27f, 1.0f}
                     : TceColor{0.27f, 0.53f, 1.00f, 1.0f};
    }
    return isBuy ? TceColor{0.16f, 0.74f, 0.45f, 1.0f}
                 : TceColor{0.93f, 0.27f, 0.27f, 1.0f};
}

std::string fmtQty(double q) {
    char buf[32];
    if (q >= 10000) std::snprintf(buf, sizeof(buf), "%.1fK", q / 1000.0);
    else            std::snprintf(buf, sizeof(buf), "%.0f",  q);
    return std::string(buf);
}

} // namespace

void MarkerRenderer::buildMarkers(const Series& series,
                                   const Viewport& vp,
                                   const PanelLayout& layout,
                                   const std::vector<TradeMarker>& markers,
                                   TceColorScheme scheme,
                                   FrameOutput& meshOut,
                                   LabelOutput& labelOut) {
    if (markers.empty()) return;

    std::vector<TceVertex> vT; std::vector<uint32_t> iT;
    const float r = 6.0f; // 삼각형 반지름

    for (const auto& m : markers) {
        float x = xForTimestamp(series, vp, layout, m.timestamp);
        if (x < layout.plot.x || x > layout.plot.x + layout.plot.w) continue;
        float y = yForPrice(layout, m.price);
        TceColor c = markerColor(m.isBuy, scheme);

        if (m.isBuy) {
            // 캔들 아래 ▲ (꼭지점 위)
            float baseY = y + 8.0f;
            emitTriangle(vT, iT, x, baseY, x - r, baseY + r * 1.6f, x + r, baseY + r * 1.6f, c);
        } else {
            // 캔들 위 ▼ (꼭지점 아래)
            float baseY = y - 8.0f;
            emitTriangle(vT, iT, x, baseY, x - r, baseY - r * 1.6f, x + r, baseY - r * 1.6f, c);
        }

        if (m.quantity > 0) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%s %s",
                          m.isBuy ? "B" : "S", fmtQty(m.quantity).c_str());
            float ly = m.isBuy ? y + r * 2.0f + 14.0f : y - r * 2.0f - 14.0f;
            labelOut.add(std::string(buf), x, ly,
                         TCE_ANCHOR_CENTER_CENTER, TCE_LABEL_PRICE_AXIS,
                         {1, 1, 1, 1}, {c.r * 0.4f, c.g * 0.4f, c.b * 0.4f, 0.95f});
        }
    }
    if (!vT.empty()) meshOut.addMesh(std::move(vT), std::move(iT), PRIM_TRIANGLES);
}

void MarkerRenderer::buildAlerts(const PanelLayout& layout,
                                  const std::vector<AlertLine>& lines,
                                  FrameOutput& meshOut,
                                  LabelOutput& labelOut) {
    if (lines.empty()) return;
    std::vector<TceVertex> vL; std::vector<uint32_t> iL;
    const float plotL = layout.plot.x;
    const float plotR = layout.plot.x + layout.plot.w;

    for (const auto& a : lines) {
        float y = yForPrice(layout, a.price);
        if (y < layout.plot.y || y > layout.plot.y + layout.plot.h) continue;
        emitLine(vL, iL, plotL, y, plotR, y, a.color);

        // 우측 priceAxis 영역에 라벨 (벨 아이콘 + 가격)
        char buf[32];
        std::snprintf(buf, sizeof(buf), "🔔 %.2f", a.price);
        float lx = layout.priceAxis.x + layout.priceAxis.w * 0.5f;
        labelOut.add(std::string(buf), lx, y,
                     TCE_ANCHOR_CENTER_CENTER, TCE_LABEL_LAST_PRICE,
                     {1, 1, 1, 1}, {a.color.r, a.color.g, a.color.b, 0.95f});
    }
    if (!vL.empty()) meshOut.addMesh(std::move(vL), std::move(iL), PRIM_LINES);
}

} // namespace tce

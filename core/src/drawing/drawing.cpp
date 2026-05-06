#include "drawing/drawing.h"
#include <algorithm>

namespace tce {

int DrawingStore::add(TceDrawingKind kind, TceColor color) {
    Drawing d{};
    d.id = nextId_++;
    d.kind = kind;
    d.color = color;
    items_.push_back(d);
    return d.id;
}

bool DrawingStore::addPoint(int id, double ts, double price) {
    auto it = std::find_if(items_.begin(), items_.end(),
        [&](const Drawing& d) { return d.id == id; });
    if (it == items_.end()) return false;
    it->points.push_back({ts, price});
    return true;
}

bool DrawingStore::replacePoint(int id, size_t idx, double ts, double price) {
    auto it = std::find_if(items_.begin(), items_.end(),
        [&](const Drawing& d) { return d.id == id; });
    if (it == items_.end()) return false;
    if (idx >= it->points.size()) {
        it->points.resize(idx + 1);
    }
    it->points[idx] = {ts, price};
    return true;
}

bool DrawingStore::translate(int id, double dts, double dprice) {
    auto it = std::find_if(items_.begin(), items_.end(),
        [&](const Drawing& d) { return d.id == id; });
    if (it == items_.end()) return false;
    for (auto& p : it->points) {
        p.timestamp += dts;
        p.price     += dprice;
    }
    return true;
}

void DrawingStore::remove(int id) {
    items_.erase(std::remove_if(items_.begin(), items_.end(),
        [&](const Drawing& d) { return d.id == id; }), items_.end());
}

void DrawingStore::clear() {
    items_.clear();
}

int DrawingStore::addImported(TceDrawingKind kind, TceColor color,
                              const DrawingPoint* points, size_t point_count) {
    // kind 범위 검증 (signed cast — enum이 unsigned면 음수 비교가 항상 false라 fallback 필요)
    const int k = static_cast<int>(kind);
    if (k < static_cast<int>(TCE_DRAW_TRENDLINE) ||
        k > static_cast<int>(TCE_DRAW_FIB_EXTENSION)) return 0;
    // point_count 검증 — kind에 따라 1 또는 2
    const bool onePoint = (kind == TCE_DRAW_HORIZONTAL || kind == TCE_DRAW_VERTICAL);
    const size_t expected = onePoint ? 1 : 2;
    if (point_count != expected || !points) return 0;

    Drawing d{};
    d.id = nextId_++;
    d.kind = kind;
    d.color = color;
    d.points.reserve(point_count);
    for (size_t i = 0; i < point_count; ++i) {
        d.points.push_back(points[i]);
    }
    items_.push_back(std::move(d));
    return items_.back().id;
}

} // namespace tce

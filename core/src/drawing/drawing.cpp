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

void DrawingStore::remove(int id) {
    items_.erase(std::remove_if(items_.begin(), items_.end(),
        [&](const Drawing& d) { return d.id == id; }), items_.end());
}

void DrawingStore::clear() {
    items_.clear();
}

} // namespace tce

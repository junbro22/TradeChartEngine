#include "chart.h"
#include "data/transforms.h"
#include <algorithm>

namespace tce {

Chart::Chart() = default;

void Chart::setHistory(const TceCandle* data, size_t count) {
    series_.setHistory(data, count);
}

void Chart::appendCandle(const TceCandle& c) {
    const size_t prevSize = series_.size();
    const bool wasAtRightEdge = (viewport_.rightOffset() == 0);
    series_.append(c);
    const bool grew = (series_.size() > prevSize);
    if (!grew) return; // 갱신/거부된 경우는 viewport 손대지 않음

    if (autoScroll_ && wasAtRightEdge) {
        // 우측 끝을 보던 사용자: 새 캔들이 우측 끝이 되도록 rightOffset 0 유지 (no-op).
    } else if (!wasAtRightEdge) {
        // 사용자가 과거를 보고 있었음 — 보고 있던 캔들을 유지하기 위해 한 칸 뒤로.
        viewport_.setRightOffset(viewport_.rightOffset() + 1);
    }
}

void Chart::updateLast(double close, double volume) {
    series_.updateLast(close, volume);
}

void Chart::addOverlay(TceIndicatorKind k, int period, double param,
                       TceColor color, TceColor color2) {
    auto it = std::find_if(overlays_.begin(), overlays_.end(),
        [&](const OverlaySpec& s) { return s.kind == k && s.period == period; });
    if (it != overlays_.end()) {
        it->param = param; it->color = color; it->color2 = color2; return;
    }
    OverlaySpec s;
    s.kind = k; s.period = period; s.param = param;
    s.color = color; s.color2 = color2;
    overlays_.push_back(s);
}

void Chart::addOverlayEx(TceIndicatorKind k,
                         int period, int p2, int p3, int p4,
                         double param, double param2,
                         TceColor color, TceColor color2) {
    auto it = std::find_if(overlays_.begin(), overlays_.end(),
        [&](const OverlaySpec& s) { return s.kind == k && s.period == period; });
    if (it != overlays_.end()) {
        it->p2 = p2; it->p3 = p3; it->p4 = p4;
        it->param = param; it->param2 = param2;
        it->color = color; it->color2 = color2;
        return;
    }
    OverlaySpec s;
    s.kind = k; s.period = period; s.p2 = p2; s.p3 = p3; s.p4 = p4;
    s.param = param; s.param2 = param2;
    s.color = color; s.color2 = color2;
    overlays_.push_back(s);
}

void Chart::removeOverlay(TceIndicatorKind k, int period) {
    overlays_.erase(std::remove_if(overlays_.begin(), overlays_.end(),
        [&](const OverlaySpec& s) { return s.kind == k && s.period == period; }),
        overlays_.end());
}

void Chart::clearOverlays() { overlays_.clear(); }

void Chart::addSubpanel(TceIndicatorKind k, int p1, int p2, int p3,
                        TceColor color1, TceColor color2, TceColor color3) {
    auto it = std::find_if(subpanels_.begin(), subpanels_.end(),
        [&](const SubpanelSpec& s) { return s.kind == k; });
    if (it != subpanels_.end()) {
        it->p1 = p1; it->p2 = p2; it->p3 = p3;
        it->color1 = color1; it->color2 = color2; it->color3 = color3;
        return;
    }
    subpanels_.push_back({k, p1, p2, p3, color1, color2, color3});
}

void Chart::removeSubpanel(TceIndicatorKind k, int period) {
    subpanels_.erase(std::remove_if(subpanels_.begin(), subpanels_.end(),
        [&](const SubpanelSpec& s) {
            if (s.kind != k) return false;
            return period <= 0 || s.p1 == period;
        }),
        subpanels_.end());
}

void Chart::clearSubpanels() { subpanels_.clear(); }

void Chart::setCrosshair(float x, float y) {
    crosshair_.visible = true;
    crosshair_.x = x;
    crosshair_.y = y;
    int idx = viewport_.indexForX(series_.size(), x);
    crosshair_.candleIndex = idx;
    if (idx >= 0 && static_cast<size_t>(idx) < series_.size()) {
        crosshair_.timestamp = series_.candles()[idx].timestamp;
    }
}

void Chart::clearCrosshair() {
    crosshair_ = CrosshairState{};
}

FrameOutput& Chart::buildFrame() {
    // Heikin-Ashi: 변환된 시리즈로 그리고 지표는 원본(series_) 기준 — 길이가 같아 인덱스 일치.
    // Renko: brick 시리즈로 그리고 지표도 brick 기준 — 원본과 길이가 달라 인덱스 매칭이 깨지므로
    //        drawSeries와 indicatorSeries를 분리하지 않는다. (정책 결정)
    Series tmp;
    const Series* drawSeries = &series_;
    const Series* indicatorSeries = nullptr; // nullptr = drawSeries와 동일
    if (config_.seriesType == TCE_SERIES_HEIKIN_ASHI) {
        auto hac = toHeikinAshi(series_);
        tmp.setHistory(hac.data(), hac.size());
        drawSeries = &tmp;
        indicatorSeries = &series_; // HA: 길이 동일 → 지표는 원본 OHLC
    } else if (config_.seriesType == TCE_SERIES_RENKO) {
        double brick = config_.renkoBrickSize;
        if (brick <= 0 && !series_.candles().empty())
            brick = series_.candles().back().close * 0.005;
        if (brick > 0) {
            auto rkc = toRenko(series_, brick);
            if (!rkc.empty()) {
                tmp.setHistory(rkc.data(), rkc.size());
                drawSeries = &tmp;
                indicatorSeries = nullptr; // Renko: brick 기준으로 지표 계산
            }
        }
    }
    builder_.build(*drawSeries, viewport_, config_, overlays_, subpanels_,
                   crosshair_, output_, lastLayout_, indicatorSeries);
    return output_;
}

LabelOutput& Chart::buildLabels() {
    labelBuilder_.build(series_, viewport_, config_, crosshair_,
                        lastLayout_, output_, labelOutput_);
    // 드로잉도 현재 layout 기준으로 메시 + 라벨 추가
    drawingRenderer_.build(series_, viewport_, lastLayout_,
                            drawings_.all(), output_, labelOutput_);
    // 매수/매도 마커 + 알림선
    markerRenderer_.buildMarkers(series_, viewport_, lastLayout_,
                                  markers_.all(), config_.scheme,
                                  output_, labelOutput_);
    markerRenderer_.buildAlerts(lastLayout_, alerts_.all(), output_, labelOutput_);
    return labelOutput_;
}

const PanelLayout& Chart::layout() const { return lastLayout_; }

void Chart::applyPinch(float scale, float anchorPx) {
    if (scale > 0.0f) viewport_.zoom(scale, anchorPx);
}

void Chart::applyPan(float dxPx) {
    viewport_.pan(dxPx);
}

double Chart::screenToTimestamp(float screenX) const {
    const auto& cs = series_.candles();
    if (cs.empty()) return 0.0;
    size_t from, to; viewport_.rangeFor(cs.size(), from, to);
    const float plotW = lastLayout_.plot.w > 0
        ? lastLayout_.plot.w
        : viewport_.width();
    const float slot = plotW / static_cast<float>(viewport_.visibleCount());
    if (slot <= 0) return cs[from].timestamp;

    double rel = (double)screenX / slot - 0.5;
    double idx = (double)from + rel;
    if (idx <= 0)        return cs.front().timestamp;
    if (idx >= cs.size() - 1) return cs.back().timestamp;
    size_t lo = (size_t)idx;
    double frac = idx - (double)lo;
    return cs[lo].timestamp + frac * (cs[lo+1].timestamp - cs[lo].timestamp);
}

double Chart::screenToPrice(float screenY) const {
    const float top = lastLayout_.plot.y;
    const float bot = lastLayout_.plot.y + lastLayout_.plot.h;
    if (lastLayout_.priceMax == lastLayout_.priceMin) return lastLayout_.priceMin;
    double t = ((double)bot - (double)screenY) / ((double)bot - (double)top);
    return lastLayout_.priceMin + t * (lastLayout_.priceMax - lastLayout_.priceMin);
}

int Chart::beginDrawing(TceDrawingKind kind, float screenX, float screenY, TceColor color) {
    int id = drawings_.add(kind, color);
    double ts = screenToTimestamp(screenX);
    double px = screenToPrice(screenY);
    drawings_.addPoint(id, ts, px);
    // 2점 도구는 시작 시 두 번째 점도 같은 위치로 미리 추가
    if (kind == TCE_DRAW_TRENDLINE   || kind == TCE_DRAW_RECTANGLE
     || kind == TCE_DRAW_FIB_RETRACEMENT || kind == TCE_DRAW_MEASURE) {
        drawings_.addPoint(id, ts, px);
    }
    return id;
}

void Chart::updateDrawing(int id, size_t pointIdx, float screenX, float screenY) {
    drawings_.replacePoint(id, pointIdx,
                           screenToTimestamp(screenX),
                           screenToPrice(screenY));
}

void Chart::removeDrawing(int id)     { drawings_.remove(id); }
void Chart::clearDrawings()           { drawings_.clear(); }

namespace {

// 점-선분 거리 (제곱)
float distSqPointToSegment(float px, float py, float x1, float y1, float x2, float y2) {
    float dx = x2 - x1, dy = y2 - y1;
    float len2 = dx * dx + dy * dy;
    if (len2 < 1e-6f) {
        float ex = px - x1, ey = py - y1;
        return ex * ex + ey * ey;
    }
    float t = ((px - x1) * dx + (py - y1) * dy) / len2;
    t = std::max(0.0f, std::min(1.0f, t));
    float qx = x1 + t * dx, qy = y1 + t * dy;
    float ex = px - qx, ey = py - qy;
    return ex * ex + ey * ey;
}

} // namespace

int Chart::hitTestDrawing(float screenX, float screenY, float tolPx) const {
    const auto& items = drawings_.all();
    if (items.empty()) return 0;
    const float tol2 = tolPx * tolPx;
    int best = 0;
    float bestDist = tol2;

    for (const auto& d : items) {
        if (d.points.empty()) continue;
        // 도메인 → 화면 변환은 chart.cpp 안에서 — 헬퍼 람다 (drawing_renderer와 중복이지만 단순화)
        auto toScreen = [&](const DrawingPoint& p) -> std::pair<float, float> {
            float x = 0, y = 0;
            const auto& cs = series_.candles();
            if (!cs.empty()) {
                size_t from, to; viewport_.rangeFor(cs.size(), from, to);
                const float plotW = lastLayout_.plot.w;
                const float slot = plotW / static_cast<float>(viewport_.visibleCount());
                if (p.timestamp <= cs.front().timestamp)
                    x = (-(double)from + 0.5) * slot;
                else if (p.timestamp >= cs.back().timestamp)
                    x = ((double)cs.size() - 1 - from + 0.5) * slot;
                else {
                    auto it = std::lower_bound(cs.begin(), cs.end(), p.timestamp,
                        [](const TceCandle& c, double v) { return c.timestamp < v; });
                    size_t hi = it - cs.begin();
                    size_t lo = hi - 1;
                    double frac = (cs[hi].timestamp != cs[lo].timestamp)
                        ? (p.timestamp - cs[lo].timestamp) / (cs[hi].timestamp - cs[lo].timestamp)
                        : 0;
                    double idx = (double)lo + frac;
                    x = (idx - (double)from + 0.5) * slot;
                }
            }
            const float top = lastLayout_.plot.y;
            const float bot = lastLayout_.plot.y + lastLayout_.plot.h;
            if (lastLayout_.priceMax != lastLayout_.priceMin) {
                double t = (p.price - lastLayout_.priceMin) /
                           (lastLayout_.priceMax - lastLayout_.priceMin);
                y = bot - t * (bot - top);
            }
            return {x, y};
        };

        // 도구별 hit-test
        if (d.kind == TCE_DRAW_HORIZONTAL) {
            auto [_, y] = toScreen(d.points[0]);
            (void)_;
            float dy = screenY - y;
            float dist = dy * dy;
            if (dist < bestDist) { bestDist = dist; best = d.id; }
        } else if (d.kind == TCE_DRAW_VERTICAL) {
            auto [x, _] = toScreen(d.points[0]);
            (void)_;
            float dx = screenX - x;
            float dist = dx * dx;
            if (dist < bestDist) { bestDist = dist; best = d.id; }
        } else if (d.points.size() >= 2) {
            auto [x1, y1] = toScreen(d.points[0]);
            auto [x2, y2] = toScreen(d.points[1]);
            float dist = distSqPointToSegment(screenX, screenY, x1, y1, x2, y2);
            if (dist < bestDist) { bestDist = dist; best = d.id; }
        }
    }
    return best;
}

void Chart::translateDrawing(int id, float dxPx, float dyPx) {
    // dxPx, dyPx → 도메인 변화량으로 변환 후 DrawingStore에 위임.
    const auto& cs = series_.candles();
    if (cs.empty()) return;
    const float plotW = lastLayout_.plot.w;
    const float slot = plotW / static_cast<float>(viewport_.visibleCount());
    if (slot <= 0) return;
    double slotsMoved = (double)dxPx / slot;
    double avgDeltaTs = (cs.size() > 1)
        ? (cs.back().timestamp - cs.front().timestamp) / static_cast<double>(cs.size() - 1)
        : 0.0;
    double dts = slotsMoved * avgDeltaTs;

    const float top = lastLayout_.plot.y;
    const float bot = lastLayout_.plot.y + lastLayout_.plot.h;
    double dprice = 0.0;
    if (bot > top && lastLayout_.priceMax > lastLayout_.priceMin) {
        dprice = -(double)dyPx / (bot - top) * (lastLayout_.priceMax - lastLayout_.priceMin);
    }
    drawings_.translate(id, dts, dprice);
}

int  Chart::addTradeMarker(double ts, double price, bool isBuy, double qty) {
    return markers_.add(ts, price, isBuy, qty);
}
void Chart::removeTradeMarker(int id) { markers_.remove(id); }
void Chart::clearTradeMarkers()        { markers_.clear(); }

int  Chart::addAlertLine(double price, TceColor color) {
    return alerts_.add(price, color);
}
void Chart::updateAlertLineByScreen(int id, float screenY) {
    alerts_.updatePrice(id, screenToPrice(screenY));
}
void Chart::removeAlertLine(int id) { alerts_.remove(id); }
void Chart::clearAlertLines()        { alerts_.clear(); }

int  Chart::hitTestAlertLine(float screenY, float tolPx) const {
    int best = 0;
    float bestDist = tolPx * tolPx;
    const float top = lastLayout_.plot.y;
    const float bot = lastLayout_.plot.y + lastLayout_.plot.h;
    if (bot <= top || lastLayout_.priceMax == lastLayout_.priceMin) return 0;
    for (const auto& a : alerts_.all()) {
        double t = (a.price - lastLayout_.priceMin) /
                   (lastLayout_.priceMax - lastLayout_.priceMin);
        float y = static_cast<float>(bot - t * (bot - top));
        float dy = screenY - y;
        float dist = dy * dy;
        if (dist < bestDist) { bestDist = dist; best = a.id; }
    }
    return best;
}

} // namespace tce

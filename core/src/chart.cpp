#include "chart.h"
#include "data/transforms.h"
#include "indicator/ma.h"
#include "indicator/ema.h"
#include "indicator/rsi.h"
#include "indicator/macd.h"
#include "indicator/bollinger.h"
#include "indicator/donchian.h"
#include "indicator/keltner.h"
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
#include <cmath>

namespace tce {

Chart::Chart() = default;

void Chart::setHistory(const TceCandle* data, size_t count) {
    series_.setHistory(data, count);
}

void Chart::appendCandle(const TceCandle& c) {
    const size_t prevSize = series_.size();
    const bool wasAtRightEdge = (viewport_.rightOffset() == 0);
    const double prevClose = (prevSize > 0)
        ? series_.candles().back().close
        : c.close;
    series_.append(c);
    const bool grew = (series_.size() > prevSize);
    const bool updated = (!grew && series_.size() == prevSize && prevSize > 0);

    if (grew) {
        if (autoScroll_ && wasAtRightEdge) {
            // 우측 끝을 보던 사용자: 새 캔들이 우측 끝이 되도록 rightOffset 0 유지 (no-op).
        } else if (!wasAtRightEdge) {
            // 사용자가 과거를 보고 있었음 — 보고 있던 캔들을 유지하기 위해 한 칸 뒤로.
            viewport_.setRightOffset(viewport_.rightOffset() + 1);
        }
        fireAlertCrossings_(prevClose, c.close);
    } else if (updated) {
        // 동일 timestamp 갱신 — 새 close로 cross 평가
        fireAlertCrossings_(prevClose, series_.candles().back().close);
    }
    // 거부된 경우(작은 timestamp)는 콜백/뷰포트 모두 손대지 않음.
}

void Chart::updateLast(double close, double volume) {
    const auto& cs = series_.candles();
    if (cs.empty()) {
        series_.updateLast(close, volume);
        return;
    }
    const double prevClose = cs.back().close;
    series_.updateLast(close, volume);
    fireAlertCrossings_(prevClose, close);
}

void Chart::fireAlertCrossings_(double prevClose, double newClose) {
    if (!alertCb_) return;
    if (prevClose == newClose) return;
    const double lo = std::min(prevClose, newClose);
    const double hi = std::max(prevClose, newClose);
    for (const auto& a : alerts_.all()) {
        // 한쪽 끝 포함, 다른 쪽 미포함 — 같은 가격에 머물 때 중복 fire 방지.
        if (a.price > lo && a.price <= hi) {
            alertCb_(a.id, a.price, alertUser_);
        }
    }
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
    return layoutYToPrice(lastLayout_, screenY);
}

int Chart::screenXToIndex(float screenX) const {
    const auto& cs = series_.candles();
    if (cs.empty()) return -1;
    const float plotW = lastLayout_.plot.w;
    if (plotW <= 0) return -1;
    if (screenX < lastLayout_.plot.x ||
        screenX > lastLayout_.plot.x + plotW) return -1;
    const float slot = plotW / static_cast<float>(viewport_.visibleCount());
    if (slot <= 0) return -1;
    size_t from, to; viewport_.rangeFor(cs.size(), from, to);
    int rel = static_cast<int>(std::floor((screenX - lastLayout_.plot.x) / slot));
    int idx = static_cast<int>(from) + rel;
    if (idx < static_cast<int>(from) || idx >= static_cast<int>(to)) return -1;
    return idx;
}

bool Chart::indexToScreenX(int index, float& out_x) const {
    const auto& cs = series_.candles();
    if (cs.empty() || index < 0) return false;
    size_t from, to; viewport_.rangeFor(cs.size(), from, to);
    if (static_cast<size_t>(index) < from ||
        static_cast<size_t>(index) >= to) return false;
    const float plotW = lastLayout_.plot.w;
    if (plotW <= 0) return false;
    const float slot = plotW / static_cast<float>(viewport_.visibleCount());
    out_x = lastLayout_.plot.x +
            (static_cast<float>(index - static_cast<int>(from)) + 0.5f) * slot;
    return true;
}

float Chart::priceToScreenY(double price) const {
    return layoutPriceToY(lastLayout_, price);
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
            y = layoutPriceToY(lastLayout_, p.price);
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
    // 가격 변환은 LOG/PERCENT 모드에서 정확히 비선형이라 single-delta로는 근사.
    // 사용자 드래그 감각상 충분 — 1점 도구는 begin/update가 정확하므로 큰 손실 없음.
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

    // 화면 중심점 기준으로 dyPx를 가격 차로 환산 — 평균적인 raw price 변환.
    const float top = lastLayout_.plot.y;
    const float bot = lastLayout_.plot.y + lastLayout_.plot.h;
    double dprice = 0.0;
    if (bot > top) {
        const float midY = (top + bot) * 0.5f;
        double pHere   = layoutYToPrice(lastLayout_, midY);
        double pThere  = layoutYToPrice(lastLayout_, midY + dyPx);
        dprice = pThere - pHere;
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

// ============================================================
// 지표 값 query — host crosshair hover 라벨용
//
// 정책 명세
//   - **idx는 항상 원본 series 인덱스(`candleCount()` 기준)**.
//   - HA 모드: 지표는 원본 OHLC로 계산되므로 query 인덱스가 자연 일치.
//   - Renko 모드: 메인 패널은 brick 시리즈로 그려지지만 query는 원본 OHLC 기준 결과를
//     반환한다(host hover의 시간 좌표가 원본 series의 timestamp이기 때문).
//     이 정책은 hover 라벨과 화면 라인이 한 칸씩 어긋나지 않게 하기 위함이며,
//     Renko에서 차트 위 라인은 brick 시리즈 기반이라 시각·query 의미가 다를 수 있음을
//     호스트에게 README로 안내한다.
// ============================================================

namespace {

template <typename V>
bool readOpt(const std::vector<V>& v, size_t i, double& out) {
    if (i >= v.size() || !v[i]) return false;
    out = *v[i];
    return true;
}

} // namespace

bool Chart::queryIndicatorValue(TceIndicatorKind kind, int period, size_t idx, double& out) const {
    if (idx >= series_.size()) return false;
    // (kind, period) 일치 spec 존재 확인 (period 무시 지표는 OBV/VWAP/Pivot)
    const bool periodIgnored = (kind == TCE_IND_OBV || kind == TCE_IND_VWAP);
    auto matchOverlay = [&]() {
        for (const auto& s : overlays_) {
            if (s.kind != kind) continue;
            if (periodIgnored || s.period == period) return true;
        }
        return false;
    };
    auto matchSubpanel = [&]() {
        for (const auto& s : subpanels_) {
            if (s.kind != kind) continue;
            if (periodIgnored || s.p1 == period) return true;
        }
        return false;
    };

    const Series* iSeries = &series_;  // query는 원본 series 기준 (정책: 위 주석 참조)

    switch (kind) {
    case TCE_IND_SMA:
        if (!matchOverlay()) return false;
        return readOpt(sma(*iSeries, period), idx, out);
    case TCE_IND_EMA:
        if (!matchOverlay()) return false;
        return readOpt(ema(*iSeries, period), idx, out);
    case TCE_IND_VWAP:
        if (!matchOverlay()) return false;
        return readOpt(vwap(*iSeries, config_.sessionOffsetSeconds), idx, out);
    case TCE_IND_SUPERTREND: {
        // spec param(multiplier) 사용 — frame_builder와 일관성 보장.
        const OverlaySpec* spec = nullptr;
        for (const auto& s : overlays_) {
            if (s.kind == TCE_IND_SUPERTREND && s.period == period) { spec = &s; break; }
        }
        if (!spec) return false;
        auto st = superTrend(*iSeries, period > 0 ? period : 10,
                                       spec->param > 0 ? spec->param : 3.0);
        return readOpt(st.line, idx, out);
    }
    case TCE_IND_RSI:
        if (!matchSubpanel()) return false;
        return readOpt(rsi(*iSeries, period), idx, out);
    case TCE_IND_ATR:
        if (!matchSubpanel()) return false;
        return readOpt(atr(*iSeries, period), idx, out);
    case TCE_IND_CCI:
        if (!matchSubpanel()) return false;
        return readOpt(cci(*iSeries, period), idx, out);
    case TCE_IND_WILLIAMS_R:
        if (!matchSubpanel()) return false;
        return readOpt(williamsR(*iSeries, period), idx, out);
    case TCE_IND_MFI:
        if (!matchSubpanel()) return false;
        return readOpt(mfi(*iSeries, period), idx, out);
    case TCE_IND_OBV:
        if (!matchSubpanel()) return false;
        return readOpt(obv(*iSeries), idx, out);
    default:
        return false;
    }
}

bool Chart::queryBollinger(int period, size_t idx, double& upper, double& middle, double& lower) const {
    if (idx >= series_.size()) return false;
    const OverlaySpec* spec = nullptr;
    for (const auto& s : overlays_) {
        if (s.kind == TCE_IND_BOLLINGER && s.period == period) { spec = &s; break; }
    }
    if (!spec) return false;
    const Series* iSeries = &series_;  // query는 원본 series 기준 (정책: 위 주석 참조)
    auto bb = bollinger(*iSeries, period, spec->param);
    return readOpt(bb.upper, idx, upper)
        && readOpt(bb.middle, idx, middle)
        && readOpt(bb.lower, idx, lower);
}

bool Chart::queryDonchian(int period, size_t idx, double& upper, double& middle, double& lower) const {
    if (idx >= series_.size()) return false;
    bool found = false;
    for (const auto& s : overlays_) {
        if (s.kind == TCE_IND_DONCHIAN && s.period == period) { found = true; break; }
    }
    if (!found) return false;
    const Series* iSeries = &series_;  // query는 원본 series 기준 (정책: 위 주석 참조)
    auto dc = donchian(*iSeries, period);
    return readOpt(dc.upper, idx, upper)
        && readOpt(dc.middle, idx, middle)
        && readOpt(dc.lower, idx, lower);
}

bool Chart::queryKeltner(int emaPeriod, size_t idx, double& upper, double& middle, double& lower) const {
    if (idx >= series_.size()) return false;
    const OverlaySpec* spec = nullptr;
    for (const auto& s : overlays_) {
        if (s.kind == TCE_IND_KELTNER && s.period == emaPeriod) { spec = &s; break; }
    }
    if (!spec) return false;
    const Series* iSeries = &series_;  // query는 원본 series 기준 (정책: 위 주석 참조)
    auto k = keltner(*iSeries, emaPeriod, spec->p2 > 0 ? spec->p2 : 10,
                                spec->param > 0 ? spec->param : 2.0);
    return readOpt(k.upper, idx, upper)
        && readOpt(k.middle, idx, middle)
        && readOpt(k.lower, idx, lower);
}

bool Chart::queryMACD(size_t idx, double& line, double& signal, double& hist) const {
    if (idx >= series_.size()) return false;
    const SubpanelSpec* spec = nullptr;
    for (const auto& s : subpanels_) {
        if (s.kind == TCE_IND_MACD) { spec = &s; break; }
    }
    if (!spec) return false;
    const Series* iSeries = &series_;  // query는 원본 series 기준 (정책: 위 주석 참조)
    auto m = macd(*iSeries, spec->p1, spec->p2, spec->p3);
    return readOpt(m.line, idx, line)
        && readOpt(m.signal, idx, signal)
        && readOpt(m.histogram, idx, hist);
}

bool Chart::queryStochastic(size_t idx, double& k, double& d) const {
    if (idx >= series_.size()) return false;
    const SubpanelSpec* spec = nullptr;
    for (const auto& s : subpanels_) {
        if (s.kind == TCE_IND_STOCHASTIC) { spec = &s; break; }
    }
    if (!spec) return false;
    const Series* iSeries = &series_;  // query는 원본 series 기준 (정책: 위 주석 참조)
    auto st = stochastic(*iSeries, spec->p1, spec->p2, spec->p3);
    return readOpt(st.k, idx, k) && readOpt(st.d, idx, d);
}

bool Chart::queryDMI(int period, size_t idx, double& plusDI, double& minusDI, double& adx) const {
    if (idx >= series_.size()) return false;
    bool found = false;
    for (const auto& s : subpanels_) {
        if (s.kind == TCE_IND_DMI_ADX && s.p1 == period) { found = true; break; }
    }
    if (!found) return false;
    const Series* iSeries = &series_;  // query는 원본 series 기준 (정책: 위 주석 참조)
    auto d = dmi(*iSeries, period);
    return readOpt(d.plusDI, idx, plusDI)
        && readOpt(d.minusDI, idx, minusDI)
        && readOpt(d.adx, idx, adx);
}

bool Chart::queryPivot(TceIndicatorKind kind, size_t idx,
                       double& p, double& r1, double& r2, double& r3,
                       double& s1, double& s2, double& s3) const {
    if (idx >= series_.size()) return false;
    if (kind != TCE_IND_PIVOT_STANDARD && kind != TCE_IND_PIVOT_FIBONACCI
        && kind != TCE_IND_PIVOT_CAMARILLA) return false;
    bool found = false;
    for (const auto& s : overlays_) {
        if (s.kind == kind) { found = true; break; }
    }
    if (!found) return false;
    PivotKind pk =
        (kind == TCE_IND_PIVOT_STANDARD)  ? PivotKind::Standard :
        (kind == TCE_IND_PIVOT_FIBONACCI) ? PivotKind::Fibonacci :
                                            PivotKind::Camarilla;
    const Series* iSeries = &series_;  // query는 원본 series 기준 (정책: 위 주석 참조)
    auto pv = pivot(*iSeries, pk, config_.sessionOffsetSeconds);
    return readOpt(pv.p,  idx, p)
        && readOpt(pv.r1, idx, r1) && readOpt(pv.r2, idx, r2) && readOpt(pv.r3, idx, r3)
        && readOpt(pv.s1, idx, s1) && readOpt(pv.s2, idx, s2) && readOpt(pv.s3, idx, s3);
}

bool Chart::queryIchimoku(size_t idx, double& tenkan, double& kijun,
                          double& senkouA, double& senkouB, double& chikou) const {
    if (idx >= series_.size()) return false;
    const OverlaySpec* spec = nullptr;
    for (const auto& s : overlays_) {
        if (s.kind == TCE_IND_ICHIMOKU) { spec = &s; break; }
    }
    if (!spec) return false;
    const Series* iSeries = &series_;  // query는 원본 series 기준 (정책: 위 주석 참조)
    const int tk = spec->period > 0 ? spec->period : 9;
    const int kj = spec->p2     > 0 ? spec->p2     : 26;
    const int sB = spec->p3     > 0 ? spec->p3     : 52;
    const int dp = spec->p4     > 0 ? spec->p4     : 26;
    auto ic = ichimoku(*iSeries, tk, kj, sB, dp);
    bool ok = readOpt(ic.tenkan, idx, tenkan) && readOpt(ic.kijun, idx, kijun);
    if (!ok) return false;
    // senkouA/B/chikou는 idx에서 nullopt일 수 있음 — 실패해도 부분 성공 허용은 안 함(전체 fail).
    return readOpt(ic.senkouA, idx, senkouA)
        && readOpt(ic.senkouB, idx, senkouB)
        && readOpt(ic.chikou, idx, chikou);
}

bool Chart::querySuperTrend(size_t idx, double& line, int& direction) const {
    if (idx >= series_.size()) return false;
    const OverlaySpec* spec = nullptr;
    for (const auto& s : overlays_) {
        if (s.kind == TCE_IND_SUPERTREND) { spec = &s; break; }
    }
    if (!spec) return false;
    const Series* iSeries = &series_;  // query는 원본 series 기준 (정책: 위 주석 참조)
    auto st = superTrend(*iSeries, spec->period > 0 ? spec->period : 10,
                                   spec->param > 0 ? spec->param : 3.0);
    if (idx >= st.line.size() || !st.line[idx]) return false;
    line = *st.line[idx];
    direction = (idx < st.direction.size()) ? st.direction[idx] : 0;
    return true;
}

bool Chart::queryVWAPBands(size_t idx, double& middle, double& upper, double& lower) const {
    if (idx >= series_.size()) return false;
    const OverlaySpec* spec = nullptr;
    for (const auto& s : overlays_) {
        if (s.kind == TCE_IND_VWAP) { spec = &s; break; }
    }
    if (!spec || spec->param <= 0) return false;  // 밴드 미등록
    const Series* iSeries = &series_;
    auto vb = vwapBands(*iSeries, config_.sessionOffsetSeconds, spec->param);
    return readOpt(vb.middle, idx, middle)
        && readOpt(vb.upper,  idx, upper)
        && readOpt(vb.lower,  idx, lower);
}

int  Chart::hitTestAlertLine(float screenY, float tolPx) const {
    int best = 0;
    float bestDist = tolPx * tolPx;
    const float top = lastLayout_.plot.y;
    const float bot = lastLayout_.plot.y + lastLayout_.plot.h;
    if (bot <= top || lastLayout_.priceMax == lastLayout_.priceMin) return 0;
    for (const auto& a : alerts_.all()) {
        float y = layoutPriceToY(lastLayout_, a.price);
        float dy = screenY - y;
        float dist = dy * dy;
        if (dist < bestDist) { bestDist = dist; best = a.id; }
    }
    return best;
}

} // namespace tce

#include "renderer/label_builder.h"
#include "data/series.h"
#include "viewport/viewport.h"
#include <cmath>
#include <cstdio>
#include <ctime>
#include <algorithm>

namespace tce {

void LabelOutput::clear() {
    texts_.clear();
    items_.clear();
}

void LabelOutput::add(std::string text, float x, float y,
                     TceTextAnchor anchor, TceLabelKind kind,
                     TceColor color, TceColor background) {
    texts_.push_back(std::move(text));
    TceLabel l{};
    l.text       = nullptr; // 아래에서 일괄 채움
    l.x          = x;
    l.y          = y;
    l.anchor     = anchor;
    l.kind       = kind;
    l.color      = color;
    l.background = background;
    items_.push_back(l);

    // 모든 텍스트 포인터 재계산 (벡터 재할당 시 무효화 방지)
    for (size_t i = 0; i < items_.size(); ++i) {
        items_[i].text = texts_[i].c_str();
    }
}

namespace {

constexpr int  PRIM_LINES = 1;

// 가격 라벨 단위 — 가격대 따라 5/10/100/1000 단위로 반올림된 grid 값 생성
std::vector<double> niceGridValues(double minP, double maxP, int approxCount) {
    std::vector<double> out;
    if (maxP <= minP || approxCount < 2) return out;

    double range = maxP - minP;
    double rough = range / approxCount;
    double exp10 = std::pow(10.0, std::floor(std::log10(rough)));
    double frac = rough / exp10;
    double step;
    if      (frac < 1.5) step = 1.0  * exp10;
    else if (frac < 3.0) step = 2.0  * exp10;
    else if (frac < 7.5) step = 5.0  * exp10;
    else                 step = 10.0 * exp10;

    double start = std::ceil(minP / step) * step;
    for (double v = start; v <= maxP; v += step) out.push_back(v);
    return out;
}

std::string formatPrice(double p) {
    char buf[32];
    if (std::fabs(p) >= 1000) {
        // 천 단위 콤마, 정수 부분
        long long n = (long long)std::llround(p);
        long long absn = n < 0 ? -n : n;
        char tmp[24];
        std::snprintf(tmp, sizeof(tmp), "%lld", absn);
        // 콤마 삽입
        std::string s = tmp;
        for (int i = (int)s.size() - 3; i > 0; i -= 3) s.insert(i, ",");
        if (n < 0) s.insert(0, "-");
        return s;
    }
    std::snprintf(buf, sizeof(buf), "%.2f", p);
    return std::string(buf);
}

void emitLine(std::vector<TceVertex>& v, std::vector<uint32_t>& idx,
              float x1, float y1, float x2, float y2, TceColor c) {
    uint32_t base = static_cast<uint32_t>(v.size());
    v.push_back({x1, y1, c.r, c.g, c.b, c.a});
    v.push_back({x2, y2, c.r, c.g, c.b, c.a});
    idx.push_back(base); idx.push_back(base + 1);
}

std::string formatTimeShort(double ts) {
    std::time_t t = static_cast<std::time_t>(ts);
    std::tm tm_v{};
    localtime_r(&t, &tm_v);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d/%02d", tm_v.tm_mon + 1, tm_v.tm_mday);
    return std::string(buf);
}

std::string formatTimeFull(double ts) {
    std::time_t t = static_cast<std::time_t>(ts);
    std::tm tm_v{};
    localtime_r(&t, &tm_v);
    char buf[24];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d",
                  tm_v.tm_year + 1900, tm_v.tm_mon + 1, tm_v.tm_mday,
                  tm_v.tm_hour, tm_v.tm_min);
    return std::string(buf);
}

} // namespace

void LabelBuilder::build(const Series& series,
                         const Viewport& vp,
                         const ChartConfig& cfg,
                         const CrosshairState& cross,
                         const PanelLayout& layout,
                         FrameOutput& meshOut,
                         LabelOutput& labelOut) {
    labelOut.clear();
    const auto& cs = series.candles();
    const double minP = layout.priceMin;
    const double maxP = layout.priceMax;
    const float panelTop    = layout.plot.y;
    const float panelBottom = layout.plot.y + layout.plot.h;
    if (cs.empty() || maxP <= minP) return;

    size_t from, to;
    vp.rangeFor(cs.size(), from, to);
    if (to <= from) return;

    const float plotW = layout.plot.w;
    const float plotH = layout.plot.h;
    const float slot = plotW / static_cast<float>(vp.visibleCount());
    // 라벨은 priceAxis 영역 중앙에 정렬
    const float rightAxisX = layout.priceAxis.x + layout.priceAxis.w * 0.5f;
    const float W = plotW; // 그리드는 plot 영역까지만

    // ===== 가격 그리드 + Y축 라벨 =====
    std::vector<TceVertex> vGrid; std::vector<uint32_t> iGrid;
    auto prices = niceGridValues(minP, maxP, 6);
    const TceColor gridCol{0.3f, 0.34f, 0.4f, 0.35f};
    const TceColor textCol{0.78f, 0.82f, 0.88f, 1.0f};
    auto yFor = [&](double p) {
        double t = (p - minP) / (maxP - minP);
        return static_cast<float>(panelBottom - t * (panelBottom - panelTop));
    };
    for (double p : prices) {
        float y = yFor(p);
        if (y < panelTop || y > panelBottom) continue;
        if (cfg.showGrid) emitLine(vGrid, iGrid, 0, y, W, y, gridCol);
        // 라벨: priceAxis 영역 중앙. wrapper는 그대로 .position() 으로 사용.
        // LOG 모드면 정규화 값(log price)을 raw price로 환산해서 표시.
        // PERCENT 모드는 정규화 값(% 변화)을 그대로 표시 — 사용자 의도에 부합.
        double display = (layout.priceMode == TCE_PRICE_LOG)
            ? layoutDenormalizePrice(layout, p)
            : p;
        labelOut.add(formatPrice(display),
                     rightAxisX, y,
                     TCE_ANCHOR_CENTER_CENTER,
                     TCE_LABEL_PRICE_AXIS,
                     textCol);
    }

    // ===== 시간 그리드 + X축 라벨 =====
    int targetTicks = 6;
    int count = static_cast<int>(to - from);
    int step = std::max(1, count / targetTicks);
    const float timeAxisY = layout.timeAxis.y + layout.timeAxis.h * 0.5f;
    for (size_t i = from; i < to; i += step) {
        float x = (static_cast<float>(i - from) + 0.5f) * slot;
        if (cfg.showGrid) emitLine(vGrid, iGrid, x, panelTop, x, plotH, gridCol);
        labelOut.add(formatTimeShort(cs[i].timestamp),
                     x, timeAxisY,
                     TCE_ANCHOR_CENTER_CENTER,
                     TCE_LABEL_TIME_AXIS,
                     textCol);
    }

    if (!vGrid.empty()) meshOut.addMesh(std::move(vGrid), std::move(iGrid), PRIM_LINES);

    // ===== 마지막 가격선 + 우측 라벨 =====
    if (!cs.empty()) {
        const auto& last = cs.back();
        // raw price → Y. LOG/PERCENT 모드 자동 변환.
        float y = layoutPriceToY(layout, last.close);
        if (y >= panelTop && y <= panelBottom) {
            std::vector<TceVertex> vLast; std::vector<uint32_t> iLast;
            const TceColor lastCol{1.0f, 0.85f, 0.20f, 0.85f};
            // 점선 효과: 짧은 dash 반복 (간단히 단색 실선)
            emitLine(vLast, iLast, 0, y, W, y, lastCol);
            meshOut.addMesh(std::move(vLast), std::move(iLast), PRIM_LINES);

            TceColor bg{0.0f, 0.0f, 0.0f, 0.0f};
            labelOut.add(formatPrice(last.close),
                         rightAxisX, y,
                         TCE_ANCHOR_CENTER_CENTER,
                         TCE_LABEL_LAST_PRICE,
                         lastCol, bg);
        }
    }

    // ===== 크로스헤어 라벨 =====
    if (cross.visible && cross.candleIndex >= 0
        && static_cast<size_t>(cross.candleIndex) < cs.size()) {
        // 가격 라벨: 크로스헤어 Y에 해당하는 raw price (LOG/PERCENT 역변환 포함)
        double price = layoutYToPrice(layout, cross.y);
        if (cross.y >= panelTop && cross.y <= panelBottom) {
            const TceColor crossLabelBg{0.13f, 0.16f, 0.20f, 0.95f};
            const TceColor crossLabelTxt{1.0f, 1.0f, 1.0f, 1.0f};
            labelOut.add(formatPrice(price),
                         rightAxisX, cross.y,
                         TCE_ANCHOR_CENTER_CENTER,
                         TCE_LABEL_CROSSHAIR_PRICE,
                         crossLabelTxt, crossLabelBg);
        }

        // 시간 라벨: candleIndex의 timestamp
        double ts = cs[cross.candleIndex].timestamp;
        labelOut.add(formatTimeFull(ts),
                     cross.x, timeAxisY,
                     TCE_ANCHOR_CENTER_CENTER,
                     TCE_LABEL_CROSSHAIR_TIME,
                     {1.0f, 1.0f, 1.0f, 1.0f},
                     {0.13f, 0.16f, 0.20f, 0.95f});
    }
}

} // namespace tce

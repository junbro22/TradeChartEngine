#include "indicator/pivot.h"
#include "data/series.h"
#include <cmath>

namespace tce {

namespace {

// UTC + offset 기반 day key — 빌드 머신 timezone 의존성 없음.
int64_t dayKey(double ts, double offset) {
    return static_cast<int64_t>(std::floor((ts + offset) / 86400.0));
}

struct DaySummary { double H, L, C; };

DaySummary computeDay(const std::vector<TceCandle>& cs, size_t fromIdx, size_t toIdx) {
    double H = cs[fromIdx].high, L = cs[fromIdx].low, C = cs[toIdx - 1].close;
    for (size_t i = fromIdx; i < toIdx; ++i) {
        if (cs[i].high > H) H = cs[i].high;
        if (cs[i].low  < L) L = cs[i].low;
    }
    return {H, L, C};
}

struct Levels { double p, r1, r2, r3, s1, s2, s3; };

Levels standard(const DaySummary& d) {
    double P  = (d.H + d.L + d.C) / 3.0;
    double R  = d.H - d.L;
    return {P,
            2*P - d.L,
            P + R,
            d.H + 2*(P - d.L),
            2*P - d.H,
            P - R,
            d.L - 2*(d.H - P)};
}

Levels fibonacci(const DaySummary& d) {
    double P = (d.H + d.L + d.C) / 3.0;
    double R = d.H - d.L;
    return {P,
            P + 0.382 * R,
            P + 0.618 * R,
            P + 1.000 * R,
            P - 0.382 * R,
            P - 0.618 * R,
            P - 1.000 * R};
}

Levels camarilla(const DaySummary& d) {
    double R = d.H - d.L;
    double P = (d.H + d.L + d.C) / 3.0;
    return {P,
            d.C + R * 1.1 / 12.0,
            d.C + R * 1.1 /  6.0,
            d.C + R * 1.1 /  4.0,
            d.C - R * 1.1 / 12.0,
            d.C - R * 1.1 /  6.0,
            d.C - R * 1.1 /  4.0};
}

} // namespace

PivotResult pivot(const Series& series, PivotKind kind, double sessionOffsetSeconds) {
    const auto& cs = series.candles();
    PivotResult r;
    const size_t N = cs.size();
    r.p.assign(N, std::nullopt);
    r.r1.assign(N, std::nullopt);
    r.r2.assign(N, std::nullopt);
    r.r3.assign(N, std::nullopt);
    r.s1.assign(N, std::nullopt);
    r.s2.assign(N, std::nullopt);
    r.s3.assign(N, std::nullopt);
    if (N == 0) return r;

    // 거래일 segments
    struct Seg { size_t from, to; };
    std::vector<Seg> segs;
    int64_t currentDay = dayKey(cs[0].timestamp, sessionOffsetSeconds);
    size_t segStart = 0;
    for (size_t i = 1; i < N; ++i) {
        int64_t d = dayKey(cs[i].timestamp, sessionOffsetSeconds);
        if (d != currentDay) {
            segs.push_back({segStart, i});
            segStart = i;
            currentDay = d;
        }
    }
    segs.push_back({segStart, N});

    // segs[k]의 levels는 segs[k+1] 동안 사용
    for (size_t s = 0; s + 1 < segs.size(); ++s) {
        DaySummary d = computeDay(cs, segs[s].from, segs[s].to);
        Levels lv;
        switch (kind) {
            case PivotKind::Standard:  lv = standard(d);  break;
            case PivotKind::Fibonacci: lv = fibonacci(d); break;
            case PivotKind::Camarilla: lv = camarilla(d); break;
        }
        for (size_t i = segs[s + 1].from; i < segs[s + 1].to; ++i) {
            r.p[i]  = lv.p;
            r.r1[i] = lv.r1; r.r2[i] = lv.r2; r.r3[i] = lv.r3;
            r.s1[i] = lv.s1; r.s2[i] = lv.s2; r.s3[i] = lv.s3;
        }
    }
    return r;
}

} // namespace tce

#pragma once

#include "tce/types.h"
#include <vector>

namespace tce {

// 도메인 좌표 (시간 + 가격)
struct DrawingPoint {
    double timestamp;
    double price;
};

struct Drawing {
    int                  id;
    TceDrawingKind       kind;
    std::vector<DrawingPoint> points;   // 1 또는 2개
    TceColor             color;
};

class DrawingStore {
public:
    int  add(TceDrawingKind kind, TceColor color);
    bool addPoint(int id, double ts, double price);
    bool replacePoint(int id, size_t idx, double ts, double price);
    /// id의 모든 점을 (dts, dprice)만큼 평행이동. 없으면 false.
    bool translate(int id, double dts, double dprice);
    void remove(int id);
    void clear();

    /// 직렬화용 — 외부 스토리지로 export 후 import 시 사용. 새 id 부여.
    /// kind/point_count 검증: kind는 [0,5] 범위, point_count는 1(horizontal/vertical)
    /// 또는 2(나머지). 위반 시 0 반환(=invalid).
    int  addImported(TceDrawingKind kind, TceColor color,
                     const DrawingPoint* points, size_t point_count);

    const std::vector<Drawing>& all() const { return items_; }

private:
    std::vector<Drawing> items_;
    int                  nextId_ = 1;
};

} // namespace tce

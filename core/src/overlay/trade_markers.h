#pragma once

#include "tce/types.h"
#include "renderer/frame_builder.h"
#include "renderer/label_builder.h"
#include <vector>

namespace tce {

class Series;
class Viewport;

struct TradeMarker {
    int    id;
    double timestamp;
    double price;
    bool   isBuy;
    double quantity;     // 0이면 라벨 없음
};

class TradeMarkerStore {
public:
    int  add(double ts, double price, bool isBuy, double qty);
    void remove(int id);
    void clear();
    const std::vector<TradeMarker>& all() const { return items_; }
private:
    std::vector<TradeMarker> items_;
    int                      nextId_ = 1;
};

struct AlertLine {
    int      id;
    double   price;
    TceColor color;
};

class AlertLineStore {
public:
    int  add(double price, TceColor color);
    bool updatePrice(int id, double price);
    void remove(int id);
    void clear();
    const std::vector<AlertLine>& all() const { return items_; }
private:
    std::vector<AlertLine> items_;
    int                    nextId_ = 1;
};

class MarkerRenderer {
public:
    void buildMarkers(const Series& series,
                      const Viewport& viewport,
                      const PanelLayout& layout,
                      const std::vector<TradeMarker>& markers,
                      TceColorScheme scheme,
                      FrameOutput& meshOut,
                      LabelOutput& labelOut);

    void buildAlerts(const PanelLayout& layout,
                     const std::vector<AlertLine>& lines,
                     FrameOutput& meshOut,
                     LabelOutput& labelOut);
};

} // namespace tce

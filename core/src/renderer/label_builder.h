#pragma once

#include "tce/types.h"
#include "renderer/frame_builder.h"
#include <vector>
#include <string>

namespace tce {

class Series;
class Viewport;

// 한 프레임의 라벨 묶음 (소유)
class LabelOutput {
public:
    void clear();
    void add(std::string text, float x, float y,
             TceTextAnchor anchor, TceLabelKind kind,
             TceColor color, TceColor background = {0, 0, 0, 0});

    // C ABI 노출용 헤더
    const std::vector<TceLabel>& items() const { return items_; }

private:
    std::vector<std::string> texts_;     // 문자열 보유 (포인터 유효성 보장)
    std::vector<TceLabel>    items_;     // text 포인터는 texts_의 c_str()
};

// 그리드 + 라벨 빌더
class LabelBuilder {
public:
    void build(const Series& series,
               const Viewport& viewport,
               const ChartConfig& config,
               const CrosshairState& crosshair,
               const PanelLayout& layout,        // plot/priceAxis/timeAxis rect
               FrameOutput& meshOut,
               LabelOutput& labelOut);
};

} // namespace tce

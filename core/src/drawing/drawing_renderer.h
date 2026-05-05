#pragma once

#include "drawing/drawing.h"
#include "renderer/frame_builder.h"
#include "renderer/label_builder.h"

namespace tce {

class Series;
class Viewport;

class DrawingRenderer {
public:
    void build(const Series& series,
               const Viewport& viewport,
               const PanelLayout& layout,
               const std::vector<Drawing>& drawings,
               FrameOutput& meshOut,
               LabelOutput& labelOut);
};

} // namespace tce

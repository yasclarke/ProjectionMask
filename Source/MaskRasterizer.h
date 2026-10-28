#pragma once

#include "MaskModel.h"
#include <vector>

// Pixel-exact rasterisation shared by every output, so the projector preview,
// the DeckLink feed and the exported ATEM mask always agree.
namespace MaskRasterizer
{
    struct Buffer
    {
        int width = 0, height = 0;
        std::vector<uint8_t> fill; // quad coverage, 0..255
        std::vector<uint8_t> line; // outline coverage, 0..255

        size_t index (int x, int y) const { return (size_t) y * (size_t) width + (size_t) x; }
    };

    // Corners are normalised (0..1). Hard edges use an even-odd scanline
    // fill at pixel centres plus a 1px Bresenham outline, whose pixels also
    // count as filled. Antialiased edges use exact horizontal / 16x vertical
    // coverage for the fill and a 1px tent-filtered line for the outline.
    Buffer rasterize (const MaskModel::Corners& corners, int width, int height, bool antialiased);

    // Black exterior, white interior, red outline composited on top.
    juce::Image toPreviewImage (const Buffer& buffer);

    // Black exterior with alpha = 1 - fill coverage, so the interior is
    // transparent.
    juce::Image toMaskImage (const Buffer& buffer);

    // 8-bit 4:2:2 'UYVY' (BMD '2vuy'), Rec.709 video range.
    void toUYVY (const Buffer& buffer, uint8_t* dest, int rowBytes);
}

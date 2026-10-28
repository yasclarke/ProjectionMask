#pragma once

#include "MaskModel.h"
#include <vector>

// Pixel-exact rasterisation shared by every output, so the projector preview,
// the DeckLink feed and the exported ATEM mask always agree.
namespace MaskRasterizer
{
    enum Pixel : uint8_t { exterior = 0, interior = 1, outline = 2 };

    struct Buffer
    {
        int width = 0, height = 0;
        std::vector<uint8_t> pixels; // row-major, one Pixel per entry

        uint8_t at (int x, int y) const { return pixels[(size_t) y * (size_t) width + (size_t) x]; }
    };

    // Corners are normalised (0..1); the quad is filled with an even-odd
    // scanline rule and a 1px Bresenham outline is drawn along each edge.
    Buffer rasterize (const MaskModel::Corners& corners, int width, int height);

    // Black exterior, white interior, red outline.
    juce::Image toPreviewImage (const Buffer& buffer);

    // Opaque black exterior, fully transparent interior (outline pixels count
    // as interior, so the visible area matches the white+red preview).
    juce::Image toMaskImage (const Buffer& buffer);

    // 8-bit 4:2:2 'UYVY' (BMD '2vuy'), Rec.709 video range.
    void toUYVY (const Buffer& buffer, uint8_t* dest, int rowBytes);
}

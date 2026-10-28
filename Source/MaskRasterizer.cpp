#include "MaskRasterizer.h"

namespace MaskRasterizer
{

static void drawLine (Buffer& b, int x0, int y0, int x1, int y1)
{
    int dx = std::abs (x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs (y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    for (;;)
    {
        b.pixels[(size_t) y0 * (size_t) b.width + (size_t) x0] = outline;

        if (x0 == x1 && y0 == y1)
            break;

        auto e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

Buffer rasterize (const MaskModel::Corners& corners, int width, int height)
{
    Buffer b;
    b.width = width;
    b.height = height;
    b.pixels.assign ((size_t) width * (size_t) height, exterior);

    std::array<juce::Point<double>, MaskModel::numCorners> pts;
    for (size_t i = 0; i < pts.size(); ++i)
        pts[i] = { corners[i].x * width, corners[i].y * height };

    // Scanline fill, sampling at pixel centres
    std::vector<double> xs;
    for (int y = 0; y < height; ++y)
    {
        auto cy = y + 0.5;
        xs.clear();

        for (size_t i = 0; i < pts.size(); ++i)
        {
            auto a = pts[i];
            auto c = pts[(i + 1) % pts.size()];

            if ((a.y <= cy && c.y > cy) || (c.y <= cy && a.y > cy))
                xs.push_back (a.x + (cy - a.y) * (c.x - a.x) / (c.y - a.y));
        }

        std::sort (xs.begin(), xs.end());

        auto* row = b.pixels.data() + (size_t) y * (size_t) width;
        for (size_t i = 0; i + 1 < xs.size(); i += 2)
        {
            auto x0 = juce::jmax (0, (int) std::ceil (xs[i] - 0.5));
            auto x1 = juce::jmin (width - 1, (int) std::ceil (xs[i + 1] - 0.5) - 1);

            for (int x = x0; x <= x1; ++x)
                row[x] = interior;
        }
    }

    auto toPixel = [&] (juce::Point<double> p)
    {
        return juce::Point<int> { juce::jlimit (0, width - 1,  (int) std::floor (p.x)),
                                  juce::jlimit (0, height - 1, (int) std::floor (p.y)) };
    };

    for (size_t i = 0; i < pts.size(); ++i)
    {
        auto a = toPixel (pts[i]);
        auto c = toPixel (pts[(i + 1) % pts.size()]);
        drawLine (b, a.x, a.y, c.x, c.y);
    }

    return b;
}

juce::Image toPreviewImage (const Buffer& buffer)
{
    // ARGB rather than RGB: on macOS JUCE's native RGB images are 4-byte ARGB
    // anyway, so writing PixelRGB into them would be wrong
    juce::Image img (juce::Image::ARGB, buffer.width, buffer.height, false);
    juce::Image::BitmapData data (img, juce::Image::BitmapData::writeOnly);

    for (int y = 0; y < buffer.height; ++y)
    {
        auto* line = data.getLinePointer (y);
        for (int x = 0; x < buffer.width; ++x)
        {
            auto* px = reinterpret_cast<juce::PixelARGB*> (line + x * data.pixelStride);
            switch (buffer.at (x, y))
            {
                case interior: px->setARGB (255, 255, 255, 255); break;
                case outline:  px->setARGB (255, 255, 0, 0);     break;
                default:       px->setARGB (255, 0, 0, 0);       break;
            }
        }
    }

    return img;
}

juce::Image toMaskImage (const Buffer& buffer)
{
    juce::Image img (juce::Image::ARGB, buffer.width, buffer.height, false);
    juce::Image::BitmapData data (img, juce::Image::BitmapData::writeOnly);

    for (int y = 0; y < buffer.height; ++y)
    {
        auto* line = data.getLinePointer (y);
        for (int x = 0; x < buffer.width; ++x)
        {
            auto* px = reinterpret_cast<juce::PixelARGB*> (line + x * data.pixelStride);
            if (buffer.at (x, y) == exterior)
                px->setARGB (255, 0, 0, 0);
            else
                px->setARGB (0, 0, 0, 0);
        }
    }

    return img;
}

void toUYVY (const Buffer& buffer, uint8_t* dest, int rowBytes)
{
    struct YCbCr { int y, cb, cr; };
    // Rec.709, 8-bit video range
    static constexpr YCbCr black { 16, 128, 128 }, white { 235, 128, 128 }, red { 63, 102, 240 };

    auto lookup = [] (uint8_t p) -> const YCbCr&
    {
        return p == interior ? white : (p == outline ? red : black);
    };

    for (int y = 0; y < buffer.height; ++y)
    {
        auto* out = dest + (size_t) y * (size_t) rowBytes;
        for (int x = 0; x + 1 < buffer.width; x += 2)
        {
            auto& a = lookup (buffer.at (x, y));
            auto& c = lookup (buffer.at (x + 1, y));
            *out++ = (uint8_t) ((a.cb + c.cb + 1) / 2);
            *out++ = (uint8_t) a.y;
            *out++ = (uint8_t) ((a.cr + c.cr + 1) / 2);
            *out++ = (uint8_t) c.y;
        }
    }
}

} // namespace MaskRasterizer

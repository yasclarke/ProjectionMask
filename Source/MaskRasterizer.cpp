#include "MaskRasterizer.h"

namespace MaskRasterizer
{

using Points = std::array<juce::Point<double>, MaskModel::numCorners>;

// x positions where the quad's edges cross the horizontal line at y, sorted
static void crossings (const Points& pts, double y, std::vector<double>& xs)
{
    xs.clear();

    for (size_t i = 0; i < pts.size(); ++i)
    {
        auto a = pts[i];
        auto c = pts[(i + 1) % pts.size()];

        if ((a.y <= y && c.y > y) || (c.y <= y && a.y > y))
            xs.push_back (a.x + (y - a.y) * (c.x - a.x) / (c.y - a.y));
    }

    std::sort (xs.begin(), xs.end());
}

static void drawLine (Buffer& b, int x0, int y0, int x1, int y1)
{
    int dx = std::abs (x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs (y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    for (;;)
    {
        auto i = b.index (x0, y0);
        b.line[i] = 255;
        b.fill[i] = 255;

        if (x0 == x1 && y0 == y1)
            break;

        auto e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void rasterizeHard (Buffer& b, const Points& pts)
{
    std::vector<double> xs;
    for (int y = 0; y < b.height; ++y)
    {
        // Sample at pixel centres
        crossings (pts, y + 0.5, xs);

        auto* row = b.fill.data() + b.index (0, y);
        for (size_t i = 0; i + 1 < xs.size(); i += 2)
        {
            auto x0 = juce::jmax (0, (int) std::ceil (xs[i] - 0.5));
            auto x1 = juce::jmin (b.width - 1, (int) std::ceil (xs[i + 1] - 0.5) - 1);

            for (int x = x0; x <= x1; ++x)
                row[x] = 255;
        }
    }

    auto toPixel = [&] (juce::Point<double> p)
    {
        return juce::Point<int> { juce::jlimit (0, b.width - 1,  (int) std::floor (p.x)),
                                  juce::jlimit (0, b.height - 1, (int) std::floor (p.y)) };
    };

    for (size_t i = 0; i < pts.size(); ++i)
    {
        auto a = toPixel (pts[i]);
        auto c = toPixel (pts[(i + 1) % pts.size()]);
        drawLine (b, a.x, a.y, c.x, c.y);
    }
}

static void rasterizeAntialiased (Buffer& b, const Points& pts)
{
    constexpr int subRows = 16;
    constexpr float subWeight = 1.0f / subRows;

    std::vector<double> xs;
    std::vector<float> acc ((size_t) b.width);

    // Adds the exact overlap of [a, c) with each pixel column
    auto addSpan = [&] (double a, double c)
    {
        a = juce::jlimit (0.0, (double) b.width, a);
        c = juce::jlimit (0.0, (double) b.width, c);
        if (c <= a)
            return;

        auto ia = (int) a, ic = (int) c;
        if (ia == ic)
        {
            acc[(size_t) ia] += (float) (c - a) * subWeight;
            return;
        }

        acc[(size_t) ia] += (float) (ia + 1 - a) * subWeight;
        for (int x = ia + 1; x < ic; ++x)
            acc[(size_t) x] += subWeight;
        if (ic < b.width)
            acc[(size_t) ic] += (float) (c - ic) * subWeight;
    };

    for (int y = 0; y < b.height; ++y)
    {
        std::fill (acc.begin(), acc.end(), 0.0f);

        for (int k = 0; k < subRows; ++k)
        {
            crossings (pts, y + (k + 0.5) / subRows, xs);
            for (size_t i = 0; i + 1 < xs.size(); i += 2)
                addSpan (xs[i], xs[i + 1]);
        }

        auto* row = b.fill.data() + b.index (0, y);
        for (int x = 0; x < b.width; ++x)
            row[x] = (uint8_t) juce::roundToInt (juce::jlimit (0.0f, 1.0f, acc[(size_t) x]) * 255.0f);
    }

    // 1px line: tent filter on the distance from each pixel centre to the edge
    for (size_t i = 0; i < pts.size(); ++i)
    {
        auto a = pts[i];
        auto c = pts[(i + 1) % pts.size()];
        auto d = c - a;
        auto lenSq = d.x * d.x + d.y * d.y;

        auto x0 = juce::jmax (0, (int) std::floor (juce::jmin (a.x, c.x) - 1.0));
        auto x1 = juce::jmin (b.width - 1, (int) std::ceil (juce::jmax (a.x, c.x) + 1.0));
        auto y0 = juce::jmax (0, (int) std::floor (juce::jmin (a.y, c.y) - 1.0));
        auto y1 = juce::jmin (b.height - 1, (int) std::ceil (juce::jmax (a.y, c.y) + 1.0));

        for (int y = y0; y <= y1; ++y)
        {
            auto* row = b.line.data() + b.index (0, y);
            auto py = y + 0.5;

            for (int x = x0; x <= x1; ++x)
            {
                auto px = x + 0.5;
                auto t = lenSq > 0.0 ? juce::jlimit (0.0, 1.0, ((px - a.x) * d.x + (py - a.y) * d.y) / lenSq) : 0.0;
                auto dist = juce::Point<double> (px - (a.x + t * d.x), py - (a.y + t * d.y)).getDistanceFromOrigin();

                if (dist < 1.0)
                    row[x] = juce::jmax (row[x], (uint8_t) juce::roundToInt ((1.0 - dist) * 255.0));
            }
        }
    }
}

Buffer rasterize (const MaskModel::Corners& corners, int width, int height, bool antialiased)
{
    Buffer b;
    b.width = width;
    b.height = height;
    b.fill.assign ((size_t) width * (size_t) height, 0);
    b.line.assign ((size_t) width * (size_t) height, 0);

    Points pts;
    for (size_t i = 0; i < pts.size(); ++i)
        pts[i] = { corners[i].x * width, corners[i].y * height };

    if (antialiased)
        rasterizeAntialiased (b, pts);
    else
        rasterizeHard (b, pts);

    return b;
}

// Grey fill level with red composited over it by the line coverage
static inline juce::PixelARGB previewPixel (uint8_t fill, uint8_t line)
{
    auto red   = fill + (255 - fill) * line / 255;
    auto green = fill * (255 - line) / 255;
    juce::PixelARGB p;
    p.setARGB (255, (uint8_t) red, (uint8_t) green, (uint8_t) green);
    return p;
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
            auto i = buffer.index (x, y);
            *reinterpret_cast<juce::PixelARGB*> (line + x * data.pixelStride) = previewPixel (buffer.fill[i], buffer.line[i]);
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
            px->setARGB ((uint8_t) (255 - buffer.fill[buffer.index (x, y)]), 0, 0, 0);
        }
    }

    return img;
}

void toUYVY (const Buffer& buffer, uint8_t* dest, int rowBytes)
{
    // Rec.709, 8-bit video range
    auto toY  = [] (const juce::PixelARGB& p) { return 16.0 + (0.2126 * p.getRed() + 0.7152 * p.getGreen() + 0.0722 * p.getBlue()) * 219.0 / 255.0; };
    auto toCb = [] (const juce::PixelARGB& p) { return 128.0 + (-0.1146 * p.getRed() - 0.3854 * p.getGreen() + 0.5 * p.getBlue()) * 224.0 / 255.0; };
    auto toCr = [] (const juce::PixelARGB& p) { return 128.0 + (0.5 * p.getRed() - 0.4542 * p.getGreen() - 0.0458 * p.getBlue()) * 224.0 / 255.0; };
    auto byte = [] (double v) { return (uint8_t) juce::jlimit (0, 255, juce::roundToInt (v)); };

    for (int y = 0; y < buffer.height; ++y)
    {
        auto* out = dest + (size_t) y * (size_t) rowBytes;
        for (int x = 0; x + 1 < buffer.width; x += 2)
        {
            auto i = buffer.index (x, y);
            auto a = previewPixel (buffer.fill[i], buffer.line[i]);
            auto c = previewPixel (buffer.fill[i + 1], buffer.line[i + 1]);

            *out++ = byte ((toCb (a) + toCb (c)) * 0.5);
            *out++ = byte (toY (a));
            *out++ = byte ((toCr (a) + toCr (c)) * 0.5);
            *out++ = byte (toY (c));
        }
    }
}

} // namespace MaskRasterizer

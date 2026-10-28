#include "MaskModel.h"

const char* MaskModel::cornerName (int index)
{
    static const char* names[] { "Top left", "Top right", "Bottom right", "Bottom left" };
    return names[index];
}

const char* MaskModel::cornerShortName (int index)
{
    static const char* names[] { "TL", "TR", "BR", "BL" };
    return names[index];
}

MaskModel::MaskModel()
{
    resetCorners();
}

juce::Point<double> MaskModel::getCornerPixels (int index) const
{
    return { corners[(size_t) index].x * width, corners[(size_t) index].y * height };
}

void MaskModel::setCornerPixels (int index, juce::Point<double> pixels)
{
    auto px = juce::jlimit (0.0, (double) width,  std::round (pixels.x));
    auto py = juce::jlimit (0.0, (double) height, std::round (pixels.y));
    juce::Point<double> n { px / width, py / height };

    if (corners[(size_t) index] == n)
        return;

    corners[(size_t) index] = n;
    sendChangeMessage();
}

void MaskModel::setResolution (int newWidth, int newHeight)
{
    if (newWidth == width && newHeight == height)
        return;

    width = newWidth;
    height = newHeight;

    // Re-snap to the new pixel grid
    for (auto& c : corners)
        c = { std::round (c.x * width) / width, std::round (c.y * height) / height };

    sendChangeMessage();
}

void MaskModel::resetCorners()
{
    corners = { juce::Point<double> { 0.0, 0.0 }, { 1.0, 0.0 }, { 1.0, 1.0 }, { 0.0, 1.0 } };
    sendChangeMessage();
}

juce::var MaskModel::toVar() const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("width", width);
    obj->setProperty ("height", height);

    juce::Array<juce::var> list;
    for (int i = 0; i < numCorners; ++i)
    {
        auto p = getCornerPixels (i);
        list.add (juce::Array<juce::var> { p.x, p.y });
    }
    obj->setProperty ("corners", list);
    return juce::var (obj);
}

bool MaskModel::fromVar (const juce::var& v)
{
    auto w = (int) v.getProperty ("width", 0);
    auto h = (int) v.getProperty ("height", 0);
    auto* list = v.getProperty ("corners", {}).getArray();

    if (w <= 0 || h <= 0 || list == nullptr || list->size() != numCorners)
        return false;

    width = w;
    height = h;

    for (int i = 0; i < numCorners; ++i)
    {
        auto& pt = list->getReference (i);
        auto px = juce::jlimit (0.0, (double) w, (double) pt[0]);
        auto py = juce::jlimit (0.0, (double) h, (double) pt[1]);
        corners[(size_t) i] = { px / w, py / h };
    }

    sendChangeMessage();
    return true;
}

bool MaskModel::saveTo (const juce::File& file) const
{
    file.getParentDirectory().createDirectory();
    return file.replaceWithText (juce::JSON::toString (toVar()));
}

bool MaskModel::loadFrom (const juce::File& file)
{
    if (! file.existsAsFile())
        return false;

    return fromVar (juce::JSON::parse (file));
}

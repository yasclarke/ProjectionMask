#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

// Four corner pins (TL, TR, BR, BL) stored normalised to the frame, so the
// geometry survives a change of mask resolution. Positions set from the UI are
// snapped to whole pixels of the current mask resolution.
class MaskModel : public juce::ChangeBroadcaster
{
public:
    static constexpr int numCorners = 4;
    using Corners = std::array<juce::Point<double>, numCorners>;

    static const char* cornerName (int index);
    static const char* cornerShortName (int index);

    MaskModel();

    const Corners& getCorners() const noexcept { return corners; }
    juce::Point<double> getCornerPixels (int index) const;
    void setCornerPixels (int index, juce::Point<double> pixels);

    int getWidth() const noexcept  { return width; }
    int getHeight() const noexcept { return height; }
    void setResolution (int newWidth, int newHeight);

    void resetCorners();

    juce::var toVar() const;
    bool fromVar (const juce::var& v);

    bool saveTo (const juce::File& file) const;
    bool loadFrom (const juce::File& file);

private:
    Corners corners;
    int width = 1920, height = 1080;
};

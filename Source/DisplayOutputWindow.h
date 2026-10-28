#pragma once

#include "MaskRasterizer.h"

// Borderless window covering one display, drawing the preview raster at the
// display's native pixel resolution so the outline is a true 1px line.
// Double-click it to close.
class DisplayOutputWindow : public juce::Component
{
public:
    explicit DisplayOutputWindow (const juce::Displays::Display& display);
    ~DisplayOutputWindow() override;

    // Physical pixel size to rasterise at
    int getPixelWidth() const noexcept  { return pixelWidth; }
    int getPixelHeight() const noexcept { return pixelHeight; }

    void setImage (juce::Image newImage);

    std::function<void()> onCloseRequested;

    void paint (juce::Graphics& g) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    juce::Image image;
    int pixelWidth = 0, pixelHeight = 0;
};

#include "DisplayOutputWindow.h"
#include "WindowLevel.h"

DisplayOutputWindow::DisplayOutputWindow (const juce::Displays::Display& display)
{
    pixelWidth  = juce::roundToInt (display.totalArea.getWidth()  * display.scale);
    pixelHeight = juce::roundToInt (display.totalArea.getHeight() * display.scale);

    setOpaque (true);
    setMouseCursor (juce::MouseCursor::NoCursor);
    addToDesktop (0); // borderless
    setBounds (display.totalArea);
    setVisible (true);

    if (auto* peer = getPeer())
        raiseWindowAboveMenuBar (peer->getNativeHandle());
}

DisplayOutputWindow::~DisplayOutputWindow()
{
    removeFromDesktop();
}

void DisplayOutputWindow::setImage (juce::Image newImage)
{
    image = std::move (newImage);
    repaint();
}

void DisplayOutputWindow::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    if (image.isValid())
    {
        g.setImageResamplingQuality (juce::Graphics::lowResamplingQuality);
        g.drawImage (image, getLocalBounds().toFloat());
    }
}

void DisplayOutputWindow::mouseDoubleClick (const juce::MouseEvent&)
{
    if (onCloseRequested)
        onCloseRequested();
}

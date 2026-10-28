#pragma once

#include "MaskRasterizer.h"
#include <memory>

// Blackmagic Desktop Video output (DeckLink, UltraStudio, etc.). The SDK is
// loaded at runtime, so the app still runs when Desktop Video isn't installed.
// All calls are made from the message thread.
class DeckLinkOutput
{
public:
    struct Mode
    {
        uint32_t id = 0; // BMDDisplayMode
        juce::String name;
        int width = 0, height = 0;
    };

    struct Device
    {
        juce::String name;
        std::vector<Mode> modes;
    };

    DeckLinkOutput();
    ~DeckLinkOutput();

    bool isDriverAvailable() const;

    // Re-enumerates output-capable devices. Doesn't disturb a running output.
    const std::vector<Device>& rescan();
    const std::vector<Device>& getDevices() const noexcept { return devices; }

    bool start (int deviceIndex, int modeIndex, juce::String& error);
    void stop();
    bool isRunning() const noexcept;

    int getFrameWidth() const noexcept;
    int getFrameHeight() const noexcept;

    // Buffer must match getFrameWidth() x getFrameHeight()
    bool display (const MaskRasterizer::Buffer& buffer);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    std::vector<Device> devices;
};

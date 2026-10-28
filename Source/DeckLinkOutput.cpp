#include "DeckLinkAPI.h"
#include "DeckLinkOutput.h"

namespace
{
    juce::String fromCFString (CFStringRef s)
    {
        if (s == nullptr)
            return {};

        auto result = juce::String::fromCFString (s);
        CFRelease (s);
        return result;
    }

    template <typename T>
    void safeRelease (T*& p)
    {
        if (p != nullptr)
        {
            p->Release();
            p = nullptr;
        }
    }
}

struct DeckLinkOutput::Impl
{
    std::vector<IDeckLink*> deckLinks; // parallel to DeckLinkOutput::devices

    IDeckLinkOutput* output = nullptr;
    IDeckLinkMutableVideoFrame* frame = nullptr;
    int width = 0, height = 0;

    ~Impl()
    {
        stop();
        releaseDevices();
    }

    void releaseDevices()
    {
        for (auto*& d : deckLinks)
            safeRelease (d);
        deckLinks.clear();
    }

    void stop()
    {
        if (output != nullptr)
            output->DisableVideoOutput();

        safeRelease (frame);
        safeRelease (output);
        width = height = 0;
    }
};

DeckLinkOutput::DeckLinkOutput() : impl (std::make_unique<Impl>()) {}

DeckLinkOutput::~DeckLinkOutput() = default;

bool DeckLinkOutput::isDriverAvailable() const
{
    auto* iterator = CreateDeckLinkIteratorInstance();
    if (iterator == nullptr)
        return false;

    iterator->Release();
    return true;
}

const std::vector<DeckLinkOutput::Device>& DeckLinkOutput::rescan()
{
    impl->releaseDevices();
    devices.clear();

    auto* iterator = CreateDeckLinkIteratorInstance();
    if (iterator == nullptr)
        return devices;

    IDeckLink* deckLink = nullptr;
    while (iterator->Next (&deckLink) == S_OK)
    {
        IDeckLinkOutput* out = nullptr;
        if (deckLink->QueryInterface (IID_IDeckLinkOutput, (void**) &out) != S_OK || out == nullptr)
        {
            deckLink->Release();
            continue;
        }

        Device device;
        CFStringRef name = nullptr;
        if (deckLink->GetDisplayName (&name) == S_OK)
            device.name = fromCFString (name);

        IDeckLinkDisplayModeIterator* modeIterator = nullptr;
        if (out->GetDisplayModeIterator (&modeIterator) == S_OK)
        {
            IDeckLinkDisplayMode* mode = nullptr;
            while (modeIterator->Next (&mode) == S_OK)
            {
                bool supported = false;
                out->DoesSupportVideoMode (bmdVideoConnectionUnspecified, mode->GetDisplayMode(), bmdFormat8BitYUV,
                                           bmdNoVideoOutputConversion, bmdSupportedVideoModeDefault, nullptr, &supported);
                if (supported)
                {
                    Mode m;
                    m.id = (uint32_t) mode->GetDisplayMode();
                    CFStringRef modeName = nullptr;
                    if (mode->GetName (&modeName) == S_OK)
                        m.name = fromCFString (modeName);
                    m.width = (int) mode->GetWidth();
                    m.height = (int) mode->GetHeight();
                    device.modes.push_back (m);
                }
                mode->Release();
            }
            modeIterator->Release();
        }

        out->Release();

        devices.push_back (device);
        impl->deckLinks.push_back (deckLink); // keeps our reference
    }

    iterator->Release();
    return devices;
}

bool DeckLinkOutput::start (int deviceIndex, int modeIndex, juce::String& error)
{
    stop();

    if (! juce::isPositiveAndBelow (deviceIndex, (int) impl->deckLinks.size())
        || ! juce::isPositiveAndBelow (modeIndex, (int) devices[(size_t) deviceIndex].modes.size()))
    {
        error = "Invalid device or mode";
        return false;
    }

    auto& mode = devices[(size_t) deviceIndex].modes[(size_t) modeIndex];

    if (impl->deckLinks[(size_t) deviceIndex]->QueryInterface (IID_IDeckLinkOutput, (void**) &impl->output) != S_OK)
    {
        impl->output = nullptr;
        error = "Device has no video output";
        return false;
    }

    if (impl->output->EnableVideoOutput ((BMDDisplayMode) mode.id, bmdVideoOutputFlagDefault) != S_OK)
    {
        error = "Couldn't enable output (is another app using the device?)";
        impl->stop();
        return false;
    }

    if (impl->output->CreateVideoFrame (mode.width, mode.height, mode.width * 2, bmdFormat8BitYUV,
                                        bmdFrameFlagDefault, &impl->frame) != S_OK)
    {
        impl->frame = nullptr;
        error = "Couldn't create video frame";
        impl->stop();
        return false;
    }

    impl->width = mode.width;
    impl->height = mode.height;
    return true;
}

void DeckLinkOutput::stop()
{
    impl->stop();
}

bool DeckLinkOutput::isRunning() const noexcept { return impl->output != nullptr && impl->frame != nullptr; }
int DeckLinkOutput::getFrameWidth() const noexcept  { return impl->width; }
int DeckLinkOutput::getFrameHeight() const noexcept { return impl->height; }

bool DeckLinkOutput::display (const MaskRasterizer::Buffer& buffer)
{
    if (! isRunning() || buffer.width != impl->width || buffer.height != impl->height)
        return false;

    void* bytes = nullptr;
    if (impl->frame->GetBytes (&bytes) != S_OK || bytes == nullptr)
        return false;

    MaskRasterizer::toUYVY (buffer, static_cast<uint8_t*> (bytes), (int) impl->frame->GetRowBytes());
    return impl->output->DisplayVideoFrameSync (impl->frame) == S_OK;
}

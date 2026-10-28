#include "MainComponent.h"

namespace
{
    struct Resolution { int width, height; const char* name; };
    const Resolution resolutions[] {
        { 1280,  720, "1280 x 720 (720p)" },
        { 1920, 1080, "1920 x 1080 (1080p)" },
        { 3840, 2160, "3840 x 2160 (2160p)" },
    };

    // Output menu item IDs
    enum { offId = 1, displayIdBase = 100, deckLinkIdBase = 200 };

    const juce::Colour panelColour { 0xff26282c };
    const juce::Colour headerColour { 0xff8a8f98 };

    juce::String describe (const juce::Displays::Display& d, int index)
    {
        auto w = juce::roundToInt (d.totalArea.getWidth() * d.scale);
        auto h = juce::roundToInt (d.totalArea.getHeight() * d.scale);
        return "Display " + juce::String (index + 1) + "  -  " + juce::String (w) + " x " + juce::String (h)
             + (d.isMain ? "  (main)" : "");
    }
}

MainComponent::MainComponent()
{
    setWantsKeyboardFocus (true);
    addAndMakeVisible (editor);

    for (auto* header : { &maskHeader, &cornersHeader, &outputsHeader, &exportHeader })
    {
        header->setFont (juce::FontOptions (11.0f, juce::Font::bold));
        header->setColour (juce::Label::textColourId, headerColour);
        addAndMakeVisible (*header);
    }

    for (auto* label : { &resolutionLabel, &outputLabel })
        addAndMakeVisible (*label);
    addChildComponent (modeLabel);

    for (int i = 0; i < (int) std::size (resolutions); ++i)
        resolutionBox.addItem (resolutions[i].name, i + 1);
    resolutionBox.onChange = [this]
    {
        auto& r = resolutions[resolutionBox.getSelectedItemIndex()];
        model.setResolution (r.width, r.height);
    };
    addAndMakeVisible (resolutionBox);

    for (int i = 0; i < MaskModel::numCorners; ++i)
    {
        cornerLabels[(size_t) i].setText (MaskModel::cornerShortName (i), juce::dontSendNotification);
        cornerLabels[(size_t) i].setTooltip (MaskModel::cornerName (i));
        addAndMakeVisible (cornerLabels[(size_t) i]);

        for (auto* s : { &xSliders[(size_t) i], &ySliders[(size_t) i] })
        {
            s->setSliderStyle (juce::Slider::IncDecButtons);
            s->setTextBoxStyle (juce::Slider::TextBoxLeft, false, 60, 24);
            s->setIncDecButtonsMode (juce::Slider::incDecButtonsDraggable_Horizontal);
            s->onValueChange = [this, i]
            {
                editor.setSelectedCorner (i);
                model.setCornerPixels (i, { xSliders[(size_t) i].getValue(), ySliders[(size_t) i].getValue() });
            };
            addAndMakeVisible (*s);
        }
        xSliders[(size_t) i].setTextValueSuffix (" x");
        ySliders[(size_t) i].setTextValueSuffix (" y");
    }

    resetButton.onClick = [this] { model.resetCorners(); };
    addAndMakeVisible (resetButton);

    outputBox.onChange = [this]
    {
        auto index = getSelectedDeckLinkIndex();
        preferredDeckLinkDevice = index >= 0 ? deckLink.getDevices()[(size_t) index].name : juce::String();
        applyOutput();
        stateDirty = true;
    };
    addAndMakeVisible (outputBox);

    deckModeBox.onChange = [this]
    {
        preferredDeckLinkMode = deckModeBox.getText();
        startDeckLink();
        stateDirty = true;
    };
    addChildComponent (deckModeBox);

    refreshButton.onClick = [this] { refreshOutputs(); };
    addAndMakeVisible (refreshButton);

    exportButton.onClick = [this] { exportMask(); };
    saveButton.onClick = [this] { saveSetup(); };
    loadButton.onClick = [this] { loadSetup(); };
    addAndMakeVisible (exportButton);
    addAndMakeVisible (saveButton);
    addAndMakeVisible (loadButton);

    statusLabel.setJustificationType (juce::Justification::topLeft);
    statusLabel.setFont (juce::FontOptions (12.0f));
    statusLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible (statusLabel);

    hintLabel.setText ("Drag corners to pin them. Arrow keys nudge the selected corner 1px "
                       "(shift: 10px), tab cycles corners, alt-drag for fine control. "
                       "Double-click a display output (or press Esc) to close it.",
                       juce::dontSendNotification);
    hintLabel.setJustificationType (juce::Justification::topLeft);
    hintLabel.setFont (juce::FontOptions (11.0f));
    hintLabel.setColour (juce::Label::textColourId, headerColour);
    addAndMakeVisible (hintLabel);

    loadState();
    model.addChangeListener (this);

    syncControlsFromModel();
    refreshOutputs();

    setSize (1280, 760);
    startTimerHz (30);
}

MainComponent::~MainComponent()
{
    stopTimer();
    model.removeChangeListener (this);
    saveState();
    closeDisplayOutput();
    deckLink.stop();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (panelColour);
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    auto panel = area.removeFromRight (340).reduced (16, 12);
    editor.setBounds (area);

    auto row = [&panel] (int h) { auto r = panel.removeFromTop (h); panel.removeFromTop (6); return r; };
    auto labelled = [&] (juce::Label& label, juce::Component& c)
    {
        auto r = row (26);
        label.setBounds (r.removeFromLeft (84));
        c.setBounds (r);
    };

    maskHeader.setBounds (row (18));
    labelled (resolutionLabel, resolutionBox);

    panel.removeFromTop (8);
    cornersHeader.setBounds (row (18));
    for (int i = 0; i < MaskModel::numCorners; ++i)
    {
        auto r = row (26);
        cornerLabels[(size_t) i].setBounds (r.removeFromLeft (34));
        auto half = r.getWidth() / 2;
        xSliders[(size_t) i].setBounds (r.removeFromLeft (half).withTrimmedRight (4));
        ySliders[(size_t) i].setBounds (r.withTrimmedLeft (4));
    }
    resetButton.setBounds (row (26));

    panel.removeFromTop (8);
    outputsHeader.setBounds (row (18));
    labelled (outputLabel, outputBox);
    if (deckModeBox.isVisible())
        labelled (modeLabel, deckModeBox);
    refreshButton.setBounds (row (26));

    panel.removeFromTop (8);
    exportHeader.setBounds (row (18));
    exportButton.setBounds (row (30));
    {
        auto r = row (26);
        saveButton.setBounds (r.removeFromLeft (r.getWidth() / 2).withTrimmedRight (3));
        loadButton.setBounds (r.withTrimmedLeft (3));
    }

    panel.removeFromTop (8);
    hintLabel.setBounds (panel.removeFromBottom (64));
    statusLabel.setBounds (panel);
}

bool MainComponent::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey && displayWindow != nullptr)
    {
        outputBox.setSelectedId (offId);
        return true;
    }
    return false;
}

//==============================================================================
void MainComponent::changeListenerCallback (juce::ChangeBroadcaster*)
{
    syncControlsFromModel();
    outputsDirty = true;
    stateDirty = true;
}

void MainComponent::timerCallback()
{
    if (outputsDirty)
    {
        outputsDirty = false;
        pushOutputs();
    }

    if (stateDirty && ! juce::ModifierKeys::currentModifiers.isAnyMouseButtonDown())
    {
        stateDirty = false;
        saveState();
    }
}

void MainComponent::syncControlsFromModel()
{
    for (int i = 0; i < (int) std::size (resolutions); ++i)
        if (resolutions[i].width == model.getWidth() && resolutions[i].height == model.getHeight())
            resolutionBox.setSelectedId (i + 1, juce::dontSendNotification);

    for (int i = 0; i < MaskModel::numCorners; ++i)
    {
        auto p = model.getCornerPixels (i);
        xSliders[(size_t) i].setRange (0.0, model.getWidth(), 1.0);
        ySliders[(size_t) i].setRange (0.0, model.getHeight(), 1.0);
        xSliders[(size_t) i].setValue (p.x, juce::dontSendNotification);
        ySliders[(size_t) i].setValue (p.y, juce::dontSendNotification);
    }

    updateStatus();
}

//==============================================================================
void MainComponent::refreshOutputs()
{
    // Keep the open display output if that display still exists
    auto previousDisplay = displayWindow != nullptr ? displays[getSelectedDisplayIndex()].totalArea
                                                    : juce::Rectangle<int>();

    displays.clear();
    for (auto& d : juce::Desktop::getInstance().getDisplays().displays)
        displays.add (d);

    // Rescanning invalidates device indices, so stop any running output first
    deckLink.stop();
    auto& devices = deckLink.rescan();

    outputBox.clear (juce::dontSendNotification);
    outputBox.addItem ("Off", offId);

    auto selected = (int) offId;

    outputBox.addSectionHeading ("Displays");
    for (int i = 0; i < displays.size(); ++i)
    {
        outputBox.addItem (describe (displays.getReference (i), i), displayIdBase + i);
        if (displayWindow != nullptr && displays.getReference (i).totalArea == previousDisplay)
            selected = displayIdBase + i;
    }

    if (! devices.empty())
    {
        outputBox.addSectionHeading ("Blackmagic");
        for (int i = 0; i < (int) devices.size(); ++i)
        {
            outputBox.addItem (devices[(size_t) i].name, deckLinkIdBase + i);
            if (selected == offId && devices[(size_t) i].name == preferredDeckLinkDevice)
                selected = deckLinkIdBase + i;
        }
    }

    outputBox.setSelectedId (selected, juce::dontSendNotification);
    applyOutput();
}

int MainComponent::getSelectedDisplayIndex() const
{
    auto index = outputBox.getSelectedId() - displayIdBase;
    return juce::isPositiveAndBelow (index, displays.size()) ? index : -1;
}

int MainComponent::getSelectedDeckLinkIndex() const
{
    auto index = outputBox.getSelectedId() - deckLinkIdBase;
    return juce::isPositiveAndBelow (index, (int) deckLink.getDevices().size()) ? index : -1;
}

void MainComponent::applyOutput()
{
    closeDisplayOutput();
    deckLink.stop();
    deckLinkError.clear();

    populateDeckLinkModes();

    if (auto display = getSelectedDisplayIndex(); display >= 0)
        openDisplayOutput (display);
    else if (getSelectedDeckLinkIndex() >= 0)
        startDeckLink();

    updateStatus();
}

void MainComponent::populateDeckLinkModes()
{
    deckModeBox.clear (juce::dontSendNotification);

    auto index = getSelectedDeckLinkIndex();
    auto show = index >= 0;
    if (show != deckModeBox.isVisible())
    {
        deckModeBox.setVisible (show);
        modeLabel.setVisible (show);
        resized();
    }

    if (! show)
        return;

    auto& modes = deckLink.getDevices()[(size_t) index].modes;
    auto selected = 0;
    for (int i = 0; i < (int) modes.size(); ++i)
    {
        auto& m = modes[(size_t) i];
        deckModeBox.addItem (m.name, i + 1);

        // Prefer the remembered format, else the first matching the mask size
        if (m.name == preferredDeckLinkMode)
            selected = i + 1;
        else if (selected == 0 && m.width == model.getWidth() && m.height == model.getHeight())
            selected = i + 1;
    }

    deckModeBox.setEnabled (! modes.empty());
    deckModeBox.setSelectedId (selected > 0 ? selected : 1, juce::dontSendNotification);
}

//==============================================================================
void MainComponent::openDisplayOutput (int displayIndex)
{
    displayWindow = std::make_unique<DisplayOutputWindow> (displays.getReference (displayIndex));
    displayWindow->onCloseRequested = [this] { outputBox.setSelectedId (offId); };
    outputsDirty = true;

    // Keep keyboard control in the editor
    if (auto* top = getTopLevelComponent())
        top->toFront (true);
    editor.grabKeyboardFocus();
    updateStatus();
}

void MainComponent::closeDisplayOutput()
{
    displayWindow.reset();
    updateStatus();
}

void MainComponent::startDeckLink()
{
    deckLink.stop();
    deckLinkError.clear();

    auto device = getSelectedDeckLinkIndex();
    auto mode = deckModeBox.getSelectedId() - 1;

    if (device >= 0 && mode >= 0)
        if (! deckLink.start (device, mode, deckLinkError))
            deckLink.stop();

    outputsDirty = true;
    updateStatus();
}

void MainComponent::pushOutputs()
{
    if (displayWindow != nullptr)
    {
        auto buffer = MaskRasterizer::rasterize (model.getCorners(), displayWindow->getPixelWidth(),
                                                 displayWindow->getPixelHeight());
        displayWindow->setImage (MaskRasterizer::toPreviewImage (buffer));
    }

    if (deckLink.isRunning())
    {
        auto buffer = MaskRasterizer::rasterize (model.getCorners(), deckLink.getFrameWidth(),
                                                 deckLink.getFrameHeight());
        if (! deckLink.display (buffer))
            deckLinkError = "Frame output failed";
    }
}

void MainComponent::updateStatus()
{
    juce::StringArray lines;
    lines.add ("Mask: " + juce::String (model.getWidth()) + " x " + juce::String (model.getHeight()));

    if (displayWindow != nullptr)
        lines.add ("Display out: " + juce::String (displayWindow->getPixelWidth()) + " x "
                   + juce::String (displayWindow->getPixelHeight()));

    if (deckLink.isRunning())
    {
        lines.add ("Blackmagic out: " + deckModeBox.getText());
        if (deckLink.getFrameWidth() != model.getWidth() || deckLink.getFrameHeight() != model.getHeight())
            lines.add ("Note: Blackmagic format size differs from the mask resolution. "
                       "Corners are proportional, but set the mask resolution to match the ATEM format before exporting.");
    }

    if (deckLinkError.isNotEmpty())
        lines.add ("Blackmagic: " + deckLinkError);

    statusLabel.setText (lines.joinIntoString ("\n"), juce::dontSendNotification);
}

//==============================================================================
void MainComponent::exportMask()
{
    auto name = "ATEM Mask " + juce::String (model.getWidth()) + "x" + juce::String (model.getHeight()) + ".png";
    chooser = std::make_unique<juce::FileChooser> ("Export ATEM mask",
                                                   juce::File::getSpecialLocation (juce::File::userDesktopDirectory)
                                                       .getChildFile (name),
                                                   "*.png");

    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file == juce::File())
            return;

        file = file.withFileExtension ("png");
        auto buffer = MaskRasterizer::rasterize (model.getCorners(), model.getWidth(), model.getHeight());
        auto image = MaskRasterizer::toMaskImage (buffer);

        file.deleteFile();
        juce::FileOutputStream out (file);
        juce::PNGImageFormat png;

        if (! out.openedOk() || ! png.writeImageToStream (image, out))
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Export failed",
                                                    "Couldn't write " + file.getFullPathName());
    });
}

void MainComponent::saveSetup()
{
    chooser = std::make_unique<juce::FileChooser> ("Save setup",
                                                   juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                                       .getChildFile ("Projection Mask.pmask"),
                                                   "*.pmask");

    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file != juce::File() && ! model.saveTo (file.withFileExtension ("pmask")))
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Save failed",
                                                    "Couldn't write " + file.getFullPathName());
    });
}

void MainComponent::loadSetup()
{
    chooser = std::make_unique<juce::FileChooser> ("Load setup",
                                                   juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                                                   "*.pmask");

    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file != juce::File() && ! model.loadFrom (file))
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Load failed",
                                                    "Not a valid setup file: " + file.getFullPathName());
    });
}

//==============================================================================
juce::File MainComponent::getStateFile()
{
    // userApplicationDataDirectory is ~/Library on macOS
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("Application Support/Projection Mask/state.json");
}

void MainComponent::saveState()
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("mask", model.toVar());
    obj->setProperty ("deckLinkDevice", preferredDeckLinkDevice);
    obj->setProperty ("deckLinkMode", preferredDeckLinkMode);

    auto file = getStateFile();
    file.getParentDirectory().createDirectory();
    file.replaceWithText (juce::JSON::toString (juce::var (obj)));
}

void MainComponent::loadState()
{
    auto state = juce::JSON::parse (getStateFile());
    model.fromVar (state.getProperty ("mask", {}));
    preferredDeckLinkDevice = state.getProperty ("deckLinkDevice", {}).toString();
    preferredDeckLinkMode = state.getProperty ("deckLinkMode", {}).toString();
}

#pragma once

#include "CornerEditor.h"
#include "DeckLinkOutput.h"
#include "DisplayOutputWindow.h"

class MainComponent : public juce::Component,
                      private juce::ChangeListener,
                      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress& key) override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;

    void syncControlsFromModel();
    void refreshOutputs();
    void applyOutput();
    int getSelectedDisplayIndex() const;
    int getSelectedDeckLinkIndex() const;
    void populateDeckLinkModes();

    void openDisplayOutput (int displayIndex);
    void closeDisplayOutput();
    void startDeckLink();

    void pushOutputs();
    void updateStatus();

    void exportMask();
    void saveSetup();
    void loadSetup();

    static juce::File getStateFile();
    void saveState();
    void loadState();

    MaskModel model;
    CornerEditor editor { model };
    DeckLinkOutput deckLink;

    std::unique_ptr<DisplayOutputWindow> displayWindow;
    juce::Array<juce::Displays::Display> displays;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::Label maskHeader { {}, "MASK" }, cornersHeader { {}, "CORNER PINS" },
                outputsHeader { {}, "OUTPUTS" }, exportHeader { {}, "EXPORT" };

    juce::Label resolutionLabel { {}, "Resolution" };
    juce::ComboBox resolutionBox;

    std::array<juce::Label, MaskModel::numCorners> cornerLabels;
    std::array<juce::Slider, MaskModel::numCorners> xSliders, ySliders;
    juce::TextButton resetButton { "Reset to full frame" };

    juce::Label outputLabel { {}, "Output" }, modeLabel { {}, "Format" };
    juce::ComboBox outputBox, deckModeBox;
    juce::TextButton refreshButton { "Refresh devices" };

    juce::TextButton exportButton { "Export ATEM mask PNG..." };
    juce::TextButton saveButton { "Save setup..." }, loadButton { "Load setup..." };

    juce::Label statusLabel, hintLabel;

    juce::String preferredDeckLinkDevice, preferredDeckLinkMode;
    juce::String deckLinkError;
    bool outputsDirty = true, stateDirty = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

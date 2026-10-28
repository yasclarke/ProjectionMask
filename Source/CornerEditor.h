#pragma once

#include "MaskModel.h"

// Interactive main-window preview: drag a corner handle to pin it, arrow keys
// nudge the selected corner by 1px (shift: 10px), tab cycles corners.
class CornerEditor : public juce::Component,
                     private juce::ChangeListener
{
public:
    explicit CornerEditor (MaskModel& model);
    ~CornerEditor() override;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    bool keyPressed (const juce::KeyPress& key) override;

    int getSelectedCorner() const noexcept { return selected; }
    void setSelectedCorner (int index);

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    juce::Rectangle<float> frameArea() const;
    juce::Point<float> toScreen (int corner) const;
    juce::Point<double> toPixels (juce::Point<float> screen) const;
    int hitTestCorner (juce::Point<float> screen) const;

    MaskModel& model;
    int selected = 0, dragging = -1, hover = -1;
    juce::Point<float> dragOffset;

    static constexpr float handleRadius = 7.0f;
    static constexpr float margin = 28.0f;
};

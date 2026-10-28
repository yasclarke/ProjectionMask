#include "CornerEditor.h"

CornerEditor::CornerEditor (MaskModel& m) : model (m)
{
    setWantsKeyboardFocus (true);
    model.addChangeListener (this);
}

CornerEditor::~CornerEditor()
{
    model.removeChangeListener (this);
}

void CornerEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    repaint();
}

void CornerEditor::setSelectedCorner (int index)
{
    selected = index;
    repaint();
}

juce::Rectangle<float> CornerEditor::frameArea() const
{
    auto area = getLocalBounds().toFloat().reduced (margin);
    auto aspect = (float) model.getWidth() / (float) model.getHeight();

    auto w = juce::jmin (area.getWidth(), area.getHeight() * aspect);
    auto h = w / aspect;
    return juce::Rectangle<float> (w, h).withCentre (area.getCentre());
}

juce::Point<float> CornerEditor::toScreen (int corner) const
{
    auto f = frameArea();
    auto n = model.getCorners()[(size_t) corner];
    return { f.getX() + (float) n.x * f.getWidth(), f.getY() + (float) n.y * f.getHeight() };
}

juce::Point<double> CornerEditor::toPixels (juce::Point<float> screen) const
{
    auto f = frameArea();
    return { (screen.x - f.getX()) / f.getWidth()  * (double) model.getWidth(),
             (screen.y - f.getY()) / f.getHeight() * (double) model.getHeight() };
}

int CornerEditor::hitTestCorner (juce::Point<float> screen) const
{
    int best = -1;
    auto bestDist = handleRadius * 2.0f;

    for (int i = 0; i < MaskModel::numCorners; ++i)
    {
        auto d = toScreen (i).getDistanceFrom (screen);
        if (d < bestDist)
        {
            bestDist = d;
            best = i;
        }
    }

    return best;
}

void CornerEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1f22));

    auto f = frameArea();
    g.setColour (juce::Colours::black);
    g.fillRect (f);

    juce::Path quad;
    quad.setUsingNonZeroWinding (false); // match the rasteriser's even-odd rule
    quad.startNewSubPath (toScreen (0));
    for (int i = 1; i < MaskModel::numCorners; ++i)
        quad.lineTo (toScreen (i));
    quad.closeSubPath();

    g.setColour (juce::Colours::white);
    g.fillPath (quad);
    g.setColour (juce::Colours::red);
    g.strokePath (quad, juce::PathStrokeType (1.0f));

    g.setColour (juce::Colour (0xff4a4c52));
    g.drawRect (f.expanded (1.0f), 1.0f);

    // Handles and labels, pushed away from the quad's centroid
    juce::Point<float> centroid;
    for (int i = 0; i < MaskModel::numCorners; ++i)
        centroid += toScreen (i) / (float) MaskModel::numCorners;

    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));

    for (int i = 0; i < MaskModel::numCorners; ++i)
    {
        auto p = toScreen (i);
        auto isActive = (i == selected);
        auto fill = isActive ? juce::Colour (0xffff9f1c)
                             : (i == hover ? juce::Colour (0xff7fb7ff) : juce::Colour (0xff3d8bfd));

        auto r = juce::Rectangle<float> (handleRadius * 2, handleRadius * 2).withCentre (p);
        g.setColour (fill.withAlpha (0.35f));
        g.fillEllipse (r);
        g.setColour (fill);
        g.drawEllipse (r, isActive ? 2.0f : 1.5f);
        g.drawLine (p.x - 3, p.y, p.x + 3, p.y, 1.0f);
        g.drawLine (p.x, p.y - 3, p.x, p.y + 3, 1.0f);

        auto dir = p - centroid;
        auto len = dir.getDistanceFromOrigin();
        dir = len > 0.001f ? dir / len : juce::Point<float> (0, -1);

        auto px = model.getCornerPixels (i);
        auto text = juce::String (MaskModel::cornerShortName (i)) + "  "
                  + juce::String ((int) px.x) + ", " + juce::String ((int) px.y);

        auto labelCentre = p + dir * 22.0f;
        auto label = juce::Rectangle<float> (96.0f, 16.0f).withCentre (labelCentre)
                         .constrainedWithin (getLocalBounds().toFloat().reduced (2.0f));
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (label, 3.0f);
        g.setColour (isActive ? fill : juce::Colours::lightgrey);
        g.drawText (text, label, juce::Justification::centred);
    }
}

void CornerEditor::mouseMove (const juce::MouseEvent& e)
{
    auto h = hitTestCorner (e.position);
    if (h != hover)
    {
        hover = h;
        setMouseCursor (h >= 0 ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void CornerEditor::mouseDown (const juce::MouseEvent& e)
{
    grabKeyboardFocus();
    dragging = hitTestCorner (e.position);

    if (dragging >= 0)
    {
        selected = dragging;
        dragOffset = toScreen (dragging) - e.position;
        repaint();
    }
}

void CornerEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging < 0)
        return;

    // Alt for fine positioning
    auto pos = e.position + dragOffset;
    if (e.mods.isAltDown())
    {
        auto start = e.mouseDownPosition + dragOffset;
        pos = start + (pos - start) * 0.2f;
    }

    model.setCornerPixels (dragging, toPixels (pos));
}

void CornerEditor::mouseUp (const juce::MouseEvent&)
{
    dragging = -1;
}

bool CornerEditor::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::tabKey)
    {
        setSelectedCorner ((selected + (key.getModifiers().isShiftDown() ? 3 : 1)) % MaskModel::numCorners);
        return true;
    }

    auto step = key.getModifiers().isShiftDown() ? 10.0 : 1.0;
    auto code = key.getKeyCode();
    juce::Point<double> delta;

    if      (code == juce::KeyPress::leftKey)  delta = { -step, 0 };
    else if (code == juce::KeyPress::rightKey) delta = {  step, 0 };
    else if (code == juce::KeyPress::upKey)    delta = { 0, -step };
    else if (code == juce::KeyPress::downKey)  delta = { 0,  step };
    else return false;

    model.setCornerPixels (selected, model.getCornerPixels (selected) + delta);
    return true;
}

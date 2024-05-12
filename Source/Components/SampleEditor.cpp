/*
  ==============================================================================

    SampleEditor.cpp
    Created: 19 Sep 2023 2:03:29pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>

#include "SampleEditor.h"

SampleEditorOverlay::SampleEditorOverlay(APVTS& apvts, juce::Array<CustomSamplerVoice*>& synthVoices) : synthVoices(synthVoices)
{
    viewStart = apvts.state.getPropertyAsValue(PluginParameters::UI_VIEW_START, apvts.undoManager);
    viewEnd = apvts.state.getPropertyAsValue(PluginParameters::UI_VIEW_END, apvts.undoManager);
    sampleStart = apvts.state.getPropertyAsValue(PluginParameters::SAMPLE_START, apvts.undoManager);
    sampleEnd = apvts.state.getPropertyAsValue(PluginParameters::SAMPLE_END, apvts.undoManager);
    loopStart = apvts.state.getPropertyAsValue(PluginParameters::LOOP_START, apvts.undoManager);
    loopEnd = apvts.state.getPropertyAsValue(PluginParameters::LOOP_END, apvts.undoManager);
    isLooping = apvts.getParameterAsValue(PluginParameters::IS_LOOPING);
    loopingHasStart = apvts.state.getPropertyAsValue(PluginParameters::LOOPING_HAS_START, apvts.undoManager);
    loopingHasEnd = apvts.state.getPropertyAsValue(PluginParameters::LOOPING_HAS_END, apvts.undoManager);

    viewStart.addListener(this);
    viewEnd.addListener(this);
    sampleStart.addListener(this);
    sampleEnd.addListener(this);
    loopStart.addListener(this);
    loopEnd.addListener(this);
    isLooping.addListener(this);
    loopingHasStart.addListener(this);
    loopingHasEnd.addListener(this);

    loopIcon.startNewSubPath(4, 0);
    loopIcon.lineTo(0, 0);
    loopIcon.lineTo(0, 10);
    loopIcon.lineTo(4, 10);
    loopIcon.startNewSubPath(11, 0);
    loopIcon.lineTo(15, 0);
    loopIcon.lineTo(15, 10);
    loopIcon.lineTo(11, 10);

    loopIconArrows.addTriangle(4, -3, 4, 3, 9, 0);
    loopIconArrows.addTriangle(11, 7, 11, 13, 6, 10);

    auto transform = juce::AffineTransform::scale(10.f / loopIcon.getBounds().getWidth()); // scaling to fit in 10px
    loopIcon.applyTransform(transform);
    loopIconArrows.applyTransform(transform);
}

SampleEditorOverlay::~SampleEditorOverlay()
{
    viewStart.removeListener(this);
    viewEnd.removeListener(this);
    sampleStart.removeListener(this);
    sampleEnd.removeListener(this);
    loopStart.removeListener(this);
    loopEnd.removeListener(this);
    isLooping.removeListener(this);
    loopingHasStart.removeListener(this);
    loopingHasEnd.removeListener(this);
}

void SampleEditorOverlay::paint(juce::Graphics& g)
{
    using namespace juce;
    if (sample && sample->getNumSamples() && !recordingMode)
    {
        // Draw voice positions
        double viewStartValue = viewStart.getValue();
        double viewEndValue = viewEnd.getValue();
        Path voicePositionsPath{};
        for (auto& voice : synthVoices)
        {
            if (voice->getCurrentlyPlayingSound() && voice->getVoiceState() != VoiceState::STOPPED)
            {
                auto location = voice->getEffectiveLocation();
                auto pos = int(jmap<double>(location - viewStartValue, 0., viewEndValue - viewStartValue, 0., double(getWidth())));
                voicePositionsPath.addLineSegment(Line<int>(pos, 0, pos, getHeight()).toFloat(), 1);
            }
        }
        g.setColour(lnf.VOICE_POSITION_COLOR);
        g.strokePath(voicePositionsPath, PathStrokeType(1.f));

        // Paint the start 
        auto iconBounds = loopIcon.getBounds();
        float startPos = sampleToPosition(sampleStart.getValue());
        g.setColour(isLooping.getValue() ? 
             (dragging && draggingTarget == EditorParts::SAMPLE_START ? lnf.LOOP_BOUNDS_SELECTED_COLOR : disabled(lnf.LOOP_BOUNDS_COLOR))
            : (dragging && draggingTarget == EditorParts::SAMPLE_START ? lnf.SAMPLE_BOUNDS_SELECTED_COLOR : disabled(lnf.SAMPLE_BOUNDS_COLOR)));
        g.fillPath(sampleStartPath, juce::AffineTransform::translation(startPos + lnf.EDITOR_BOUNDS_WIDTH / 2.f, 0.f));
        
        // Start icon
        if (isLooping.getValue())
        {
            g.fillRoundedRectangle(float(startPos), getHeight() - iconBounds.getHeight() - 4, iconBounds.getWidth() + 4, iconBounds.getHeight() + 4, 3.f);
            g.setColour(lnf.LOOP_ICON_COLOR);
            auto iconTranslation = juce::AffineTransform::translation(lnf.EDITOR_BOUNDS_WIDTH + startPos, getHeight() - iconBounds.getHeight() - 2);
            if (loopingHasStart.getValue())
            {
                // Modified icon
                iconTranslation = iconTranslation.scaled(0.7f, 1.f, lnf.EDITOR_BOUNDS_WIDTH + startPos + iconBounds.getWidth(), 0);
                g.drawLine(lnf.EDITOR_BOUNDS_WIDTH + startPos, getHeight() - iconBounds.getHeight() - 3,
                    lnf.EDITOR_BOUNDS_WIDTH + startPos, getHeight() - 1.f, 1.7f);
            }
            g.strokePath(loopIcon, PathStrokeType(1.6f, PathStrokeType::JointStyle::curved), iconTranslation);
            g.fillPath(loopIconArrows, iconTranslation);
        }
        
        // Paint the end bound
        float endPos = sampleToPosition(sampleEnd.getValue());
        g.setColour(isLooping.getValue() ?
            (dragging && draggingTarget == EditorParts::SAMPLE_END ? lnf.LOOP_BOUNDS_SELECTED_COLOR : disabled(lnf.LOOP_BOUNDS_COLOR))
            : (dragging && draggingTarget == EditorParts::SAMPLE_END ? lnf.SAMPLE_BOUNDS_SELECTED_COLOR : disabled(lnf.SAMPLE_BOUNDS_COLOR)));
        g.fillPath(sampleEndPath, juce::AffineTransform::translation(endPos + 3 * lnf.EDITOR_BOUNDS_WIDTH / 2.f, 0));
        
        // End icon
        if (isLooping.getValue())
        {
            g.fillRoundedRectangle(endPos - iconBounds.getWidth(), getHeight() - iconBounds.getHeight() - 4.f, iconBounds.getWidth() + 4, iconBounds.getHeight() + 4, 3.f);
            g.setColour(lnf.LOOP_ICON_COLOR);
            auto iconTranslation = juce::AffineTransform::translation(lnf.EDITOR_BOUNDS_WIDTH + endPos - iconBounds.getWidth(), getHeight() - iconBounds.getHeight() - 2);
            if (loopingHasEnd.getValue())
            {
                // Modified icon
                iconTranslation = iconTranslation.scaled(0.7f, 1.f, lnf.EDITOR_BOUNDS_WIDTH + endPos - iconBounds.getWidth(), 0);
                g.drawLine(Line<float>(lnf.EDITOR_BOUNDS_WIDTH + endPos, getHeight() - iconBounds.getHeight() - 3,
                    lnf.EDITOR_BOUNDS_WIDTH + endPos, getHeight() - 1.f), 1.7f);
            }
            g.strokePath(loopIcon, PathStrokeType(1.6f, PathStrokeType::JointStyle::curved), iconTranslation);
            g.fillPath(loopIconArrows, iconTranslation);
        }
        
        // Paint the loop bounds
        if (isLooping.getValue())
        {
            if (loopingHasStart.getValue())
            {
                float loopStartPos = sampleToPosition(loopStart.getValue());
                g.setColour(dragging && draggingTarget == EditorParts::LOOP_START ? lnf.SAMPLE_BOUNDS_SELECTED_COLOR : disabled(lnf.SAMPLE_BOUNDS_COLOR));
                g.fillPath(sampleStartPath, juce::AffineTransform::translation(loopStartPos + lnf.EDITOR_BOUNDS_WIDTH / 2.f, 0));
            }
            if (loopingHasEnd.getValue()) 
            {
                float loopEndPos = sampleToPosition(loopEnd.getValue());
                g.setColour(dragging && draggingTarget == EditorParts::LOOP_END ? lnf.SAMPLE_BOUNDS_SELECTED_COLOR : disabled(lnf.SAMPLE_BOUNDS_COLOR));
                g.fillPath(sampleEndPath, juce::AffineTransform::translation(loopEndPos + 3 * lnf.EDITOR_BOUNDS_WIDTH / 2.f, 0));
            }
        }
    }
}

void SampleEditorOverlay::resized()
{
    painterWidth = getWidth() - 2 * lnf.EDITOR_BOUNDS_WIDTH;
    sampleStartPath.clear();
    sampleStartPath.addLineSegment(juce::Line<int>(0, 0, 0, getHeight()).toFloat(), float(lnf.EDITOR_BOUNDS_WIDTH));
    sampleStartPath.addLineSegment(juce::Line<int>(0, 0, 8, 0).toFloat(), 4.f);
    sampleStartPath.addLineSegment(juce::Line<int>(0, getHeight(), 10, getHeight()).toFloat(), 4.f);

    sampleEndPath.clear();
    sampleEndPath.addLineSegment(juce::Line<int>(0, 0, 0, getHeight()).toFloat(), float(lnf.EDITOR_BOUNDS_WIDTH));
    sampleEndPath.addLineSegment(juce::Line<int>(-10, 0, 0, 0).toFloat(), 4);
    sampleEndPath.addLineSegment(juce::Line<int>(-10, getHeight(), 0, getHeight()).toFloat(), 4);
}

void SampleEditorOverlay::enablementChanged()
{
    repaint();
}

void SampleEditorOverlay::mouseMove(const MouseEvent& event)
{
    if (!sample || !isEnabled() || recordingMode)
    {
        setMouseCursor(MouseCursor::NormalCursor);
        return;
    }
    EditorParts editorPart = getClosestPartInRange(event.x, event.y);
    switch (editorPart)
    {
    case EditorParts::SAMPLE_START:
    case EditorParts::SAMPLE_END:
    case EditorParts::LOOP_START:
    case EditorParts::LOOP_END:
        setMouseCursor(MouseCursor::LeftRightResizeCursor);
        break;
    case EditorParts::LOOP_START_BUTTON:
    case EditorParts::LOOP_END_BUTTON:
        setMouseCursor(MouseCursor::NormalCursor);
        break;
    default:
        setMouseCursor(MouseCursor::NormalCursor);
    }
}

void SampleEditorOverlay::mouseDown(const juce::MouseEvent& event)
{
    if (!sample || !isEnabled() || recordingMode)
        return;
    EditorParts closest = getClosestPartInRange(event.getMouseDownX(), event.getMouseDownY());
    switch (closest)
    {
    case EditorParts::LOOP_START_BUTTON:
        loopingHasStart = !bool(loopingHasStart.getValue());
        break;
    case EditorParts::LOOP_END_BUTTON:
        loopingHasEnd = !bool(loopingHasEnd.getValue());
        break;
    case EditorParts::NONE:
        break;
    default:
        dragging = true;
        draggingTarget = closest;
        repaint();
        break;
    }
}

void SampleEditorOverlay::mouseUp(const juce::MouseEvent&)
{
    if (!sample || recordingMode)
        return;
    dragging = false;
    repaint();
}

void SampleEditorOverlay::mouseDrag(const juce::MouseEvent& event)
{
    if (!sample || !dragging || !isEnabled() || recordingMode)
        return;
    auto newSample = positionToSample(float(event.getMouseDownX() + event.getOffsetFromDragStart().getX() - lnf.EDITOR_BOUNDS_WIDTH));
    switch (draggingTarget)
    {
    case EditorParts::SAMPLE_START:
        sampleStart = juce::jlimit<int>(isLooping.getValue() && loopingHasStart.getValue() ? int(loopStart.getValue()) + 1 : int(viewStart.getValue()), 
            sampleEnd.getValue(), newSample);
        break;
    case EditorParts::SAMPLE_END:
        sampleEnd = juce::jlimit<int>(sampleStart.getValue(), 
            isLooping.getValue() && loopingHasEnd.getValue() ? int(loopEnd.getValue()) - 1 : int(viewEnd.getValue()), newSample);
        break;
    case EditorParts::LOOP_START:
        loopStart = juce::jlimit<int>(viewStart.getValue(), int(sampleStart.getValue()) - 1, newSample);
        break;
    case EditorParts::LOOP_END:
        loopEnd = juce::jlimit<int>(int(sampleEnd.getValue()) + 1, viewEnd.getValue(), newSample);
        break;
    }
}

EditorParts SampleEditorOverlay::getClosestPartInRange(int x, int y)
{
    auto startPos = sampleToPosition(int(sampleStart.getValue()));
    auto endPos = sampleToPosition(int(sampleEnd.getValue()));
    juce::Array<CompPart<EditorParts>> targets = {
        CompPart {EditorParts::SAMPLE_START, juce::Rectangle<float>(startPos + lnf.EDITOR_BOUNDS_WIDTH / 2.f, 0.f, 1.f, float(getHeight())), 1},
        CompPart {EditorParts::SAMPLE_END, juce::Rectangle<float>(endPos + 3 * lnf.EDITOR_BOUNDS_WIDTH / 2.f, 0.f, 1.f, float(getHeight())), 1},
    };
    if (isLooping.getValue())
    {
        auto icon = loopIcon.getBounds();
        targets.add(
            CompPart{ EditorParts::LOOP_START_BUTTON, icon.withPosition(startPos, getHeight() - icon.getHeight()), 2},
            CompPart{ EditorParts::LOOP_END_BUTTON, icon.withPosition(endPos - icon.getWidth(), getHeight() - icon.getHeight()), 2}
        );
        if (loopingHasStart.getValue())
        {
            targets.add(CompPart{ EditorParts::LOOP_START, juce::Rectangle<float>(sampleToPosition(int(loopStart.getValue())) + lnf.EDITOR_BOUNDS_WIDTH / 2.f, 0.f, 1.f, float(getHeight())), 1 });
        }
        if (loopingHasEnd.getValue())
        {
            targets.add(CompPart{ EditorParts::LOOP_END, juce::Rectangle<float>(sampleToPosition(int(loopEnd.getValue())) + 3 * lnf.EDITOR_BOUNDS_WIDTH / 2.f, 0.f, 1.f, float(getHeight())), 1 });
        }
    }
    return CompPart<EditorParts>::getClosestInRange(targets, x, y, lnf.DRAGGABLE_SNAP);
}

void SampleEditorOverlay::valueChanged(juce::Value&)
{
    repaint();
}


float SampleEditorOverlay::sampleToPosition(int sampleIndex)
{
    int start = viewStart.getValue();
    int end = viewEnd.getValue();
    return juce::jmap<float>(float(sampleIndex - start), 0.f, float(end - start), 0.f, float(painterWidth));
}

int SampleEditorOverlay::positionToSample(float position)
{
    int start = viewStart.getValue();
    int end = viewEnd.getValue();
    return start + int(juce::jmap<float>(position, 0.f, float(painterWidth), 0.f, float(end - start)));
}

void SampleEditorOverlay::setSample(juce::AudioBuffer<float>& sampleBuffer)
{
    sample = &sampleBuffer;
}

void SampleEditorOverlay::setRecordingMode(bool recording)
{
    recordingMode = recording;
    resized();
}

SampleEditor::SampleEditor(APVTS& apvts, juce::Array<CustomSamplerVoice*>& synthVoices) : apvts(apvts), overlay(apvts, synthVoices)
{
    apvts.state.addListener(this);

    label.setAlwaysOnTop(true);
    label.setColour(Label::ColourIds::textColourId, Colours::white);
    label.setJustificationType(Justification::centred);
    addAndMakeVisible(&label);

    addAndMakeVisible(&painter);
    overlay.toFront(true);
    addAndMakeVisible(&overlay);

    addMouseListener(this, true);
}

SampleEditor::~SampleEditor()
{
    apvts.state.removeListener(this);
}

void SampleEditor::paint(juce::Graphics& g)
{
    g.fillAll(lnf.BACKGROUND_COLOR);
    if (boundsSelecting && dragging)
    {
        g.setColour(lnf.SAMPLE_BOUNDS_SELECTED_COLOR);
        int x = jmin(startLoc, endLoc);
        int width = jmax(startLoc, endLoc) - x;
        g.fillRect(x, 0, width, getHeight());
    }
}

void SampleEditor::resized()
{
    auto bounds = getLocalBounds();
    
    label.setVisible(boundsSelecting);
    if (boundsSelecting)
    {
        label.setBounds(bounds);
    }

    overlay.setBounds(bounds);
    painter.setBounds(getPainterBounds());
}

Rectangle<int> SampleEditor::getPainterBounds() const
{
    auto bounds = getLocalBounds();
    bounds.removeFromLeft(lnf.EDITOR_BOUNDS_WIDTH);
    bounds.removeFromRight(lnf.EDITOR_BOUNDS_WIDTH);
    return bounds;
}

void SampleEditor::enablementChanged()
{
    overlay.setEnabled(isEnabled());
    painter.setEnabled(isEnabled());
}

void SampleEditor::valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property)
{
    if (property.toString() == PluginParameters::UI_VIEW_START || property.toString() == PluginParameters::UI_VIEW_END)
    {
        int start = treeWhosePropertyHasChanged.getProperty(PluginParameters::UI_VIEW_START);
        int stop = treeWhosePropertyHasChanged.getProperty(PluginParameters::UI_VIEW_END);
        painter.setSampleView(start, stop);
    }
}

void SampleEditor::repaintUI()
{
    overlay.repaint();
}

void SampleEditor::setSample(juce::AudioBuffer<float>& sample, bool resetUI)
{
    if (resetUI || recordingMode) 
    {
        painter.setSample(sample);
    }
    else
    {
        painter.setSample(sample, apvts.state.getProperty(PluginParameters::UI_VIEW_START), apvts.state.getProperty(PluginParameters::UI_VIEW_END));
    }
    overlay.setSample(sample);
}

void SampleEditor::setGain(float gain)
{
    painter.setGain(gain);
}

void SampleEditor::setRecordingMode(bool recording)
{
    recordingMode = recording;
    overlay.setRecordingMode(recordingMode);
}

void SampleEditor::sampleUpdated(int oldSize, int newSize)
{
    painter.appendToPath(oldSize, newSize - 1);
}

void SampleEditor::mouseDown(const juce::MouseEvent& event)
{
    auto e = event.getEventRelativeTo(this);
    if (boundsSelecting && !dragging)
    {
        dragging = true;
        
        auto bounds = getPainterBounds();
        startLoc = jlimit<int>(bounds.getX(), bounds.getRight(), e.x);
    }
}

void SampleEditor::mouseDrag(const juce::MouseEvent& event)
{
    auto e = event.getEventRelativeTo(this);
    if (boundsSelecting && dragging)
    {
        auto bounds = getPainterBounds();
        endLoc = jlimit<int>(bounds.getX(), bounds.getRight(), e.x);
        repaint();
    }
}

void SampleEditor::mouseUp(const juce::MouseEvent& event)
{
    auto e = event.getEventRelativeTo(this);
    if (!getLocalBounds().contains(e.getPosition()))
        return;
    if (boundsSelecting && dragging)
    {
        auto bounds = getPainterBounds();
        endLoc = jlimit<int>(bounds.getX(), bounds.getRight(), e.x);

        int leftLoc = jmin(startLoc, endLoc);
        int rightLoc = jmax(startLoc, endLoc);

        int startSample = jmap<int>(leftLoc, 
            bounds.getX(), bounds.getRight(),
            apvts.state.getProperty(PluginParameters::UI_VIEW_START), apvts.state.getProperty(PluginParameters::UI_VIEW_END));
        int endSample = jmap<int>(rightLoc, 
            bounds.getX(), bounds.getRight(),
            apvts.state.getProperty(PluginParameters::UI_VIEW_START), apvts.state.getProperty(PluginParameters::UI_VIEW_END));
        for (auto listener : boundSelectListeners)
        {
            listener->boundsSelected(startSample, endSample);
        }
        endBoundsSelectPrompt();
    }
}

void SampleEditor::boundsSelectPrompt(const String& text)
{
    boundsSelecting = true;
    dragging = false;
    label.setText(text, NotificationType::dontSendNotification);
    overlay.setEnabled(false);
    painter.setEnabled(false);
    resized();
}

void SampleEditor::endBoundsSelectPrompt()
{
    boundsSelecting = false;
    overlay.setEnabled(true);
    painter.setEnabled(true);
    resized();
}

void SampleEditor::addBoundsSelectListener(BoundsSelectListener* listener)
{
    boundSelectListeners.add(listener);
}

void SampleEditor::removeBoundsSelectListener(BoundsSelectListener* listener)
{
    boundSelectListeners.removeAllInstancesOf(listener);
}

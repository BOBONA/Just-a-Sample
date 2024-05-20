/*
  ==============================================================================

    SampleEditorOverlay.cpp
    Created: 19 Sep 2023 2:03:29pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>

#include "SampleEditor.h"

SampleEditorOverlay::SampleEditorOverlay(APVTS& apvts, const juce::Array<CustomSamplerVoice*>& synthVoices) : synthVoices(synthVoices)
{
    viewStart = apvts.state.getPropertyAsValue(PluginParameters::UI_VIEW_START, apvts.undoManager);
    viewEnd = apvts.state.getPropertyAsValue(PluginParameters::UI_VIEW_END, apvts.undoManager);
    sampleStart = apvts.state.getPropertyAsValue(PluginParameters::SAMPLE_START, apvts.undoManager);
    sampleEnd = apvts.state.getPropertyAsValue(PluginParameters::SAMPLE_END, apvts.undoManager);
    loopStart = apvts.state.getPropertyAsValue(PluginParameters::LOOP_START, apvts.undoManager);
    loopEnd = apvts.state.getPropertyAsValue(PluginParameters::LOOP_END, apvts.undoManager);
    isLooping = apvts.getParameterAsValue(PluginParameters::IS_LOOPING);
    loopingHasStart = apvts.getParameterAsValue(PluginParameters::LOOPING_HAS_START);
    loopingHasEnd = apvts.getParameterAsValue(PluginParameters::LOOPING_HAS_END);

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

//==============================================================================
void SampleEditorOverlay::valueChanged(juce::Value&)
{
    repaint();
}

void SampleEditorOverlay::paint(juce::Graphics& g)
{
    using namespace juce;
    if (sampleBuffer && sampleBuffer->getNumSamples() && !recordingMode)
    {
        // Draw voice positions
        double viewStartValue = viewStart.getValue();
        double viewEndValue = viewEnd.getValue();
        Path voicePositionsPath{};
        for (auto& voice : synthVoices)
        {
            if (voice->isPlaying())
            {
                auto location = voice->getPosition();
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
        float endPos = sampleToPosition(int(sampleEnd.getValue()) + 1);
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
                float loopEndPos = sampleToPosition(int(loopEnd.getValue()) + 1);
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

//==============================================================================
void SampleEditorOverlay::mouseMove(const juce::MouseEvent& event)
{
    if (!sampleBuffer || !isEnabled() || recordingMode)
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    }
    EditorParts editorPart = getClosestPartInRange(event.x, event.y);
    switch (editorPart)
    {
    case EditorParts::SAMPLE_START:
    case EditorParts::SAMPLE_END:
    case EditorParts::LOOP_START:
    case EditorParts::LOOP_END:
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        break;
    case EditorParts::LOOP_START_BUTTON:
    case EditorParts::LOOP_END_BUTTON:
        setMouseCursor(juce::MouseCursor::NormalCursor);
        break;
    case EditorParts::NONE:
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void SampleEditorOverlay::mouseDown(const juce::MouseEvent& event)
{
    if (!sampleBuffer || !isEnabled() || recordingMode)
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
    if (!sampleBuffer || recordingMode)
        return;
    dragging = false;
    repaint();
}

void SampleEditorOverlay::mouseDrag(const juce::MouseEvent& event)
{
    if (!sampleBuffer || !dragging || !isEnabled() || recordingMode)
        return;
    auto newSample = positionToSample(float(event.getMouseDownX() + event.getOffsetFromDragStart().getX() - lnf.EDITOR_BOUNDS_WIDTH));
    // A bit hacky, to use the same function definition we need to add/subtract MINIMUM_BOUNDS_DISTANCE, since there doesn't need to be a minimum distance from view bounds
    switch (draggingTarget)
    {
    case EditorParts::SAMPLE_START:
        sampleStart = limitBounds(sampleStart.getValue(), newSample,
            isLooping.getValue() && loopingHasStart.getValue() ? int(loopStart.getValue()) : int(viewStart.getValue()) - lnf.MINIMUM_BOUNDS_DISTANCE, sampleEnd.getValue());
        break;
    case EditorParts::SAMPLE_END:
        sampleEnd = limitBounds(sampleEnd.getValue(), newSample,
            sampleStart.getValue(), isLooping.getValue() && loopingHasEnd.getValue() ? int(loopEnd.getValue()) : int(viewEnd.getValue()) + lnf.MINIMUM_BOUNDS_DISTANCE);
        break;
    case EditorParts::LOOP_START:
        loopStart = limitBounds(loopStart.getValue(), newSample, int(viewStart.getValue()) - lnf.MINIMUM_BOUNDS_DISTANCE, int(sampleStart.getValue()));
        break;
    case EditorParts::LOOP_END:
        loopEnd = limitBounds(loopEnd.getValue(), newSample, int(sampleEnd.getValue()), int(viewEnd.getValue()) + lnf.MINIMUM_BOUNDS_DISTANCE);
        break;
    }
}

//==============================================================================
void SampleEditorOverlay::setSample(const juce::AudioBuffer<float>& sample)
{
    sampleBuffer = &sample;
}

void SampleEditorOverlay::setRecordingMode(bool recording)
{
    recordingMode = recording;
    resized();
}

//==============================================================================
int SampleEditorOverlay::limitBounds(int previousValue, int sample, int start, int end)
{
    if (sample <= previousValue)
        return juce::jmax(sample, juce::jmin(previousValue, start + lnf.MINIMUM_BOUNDS_DISTANCE));
    else if (sample >= previousValue)
        return juce::jmin(sample, juce::jmax(previousValue, end - lnf.MINIMUM_BOUNDS_DISTANCE));
    else
        return juce::jlimit<int>(start + lnf.MINIMUM_BOUNDS_DISTANCE, end - lnf.MINIMUM_BOUNDS_DISTANCE, sample);
}

EditorParts SampleEditorOverlay::getClosestPartInRange(int x, int y)
{
    auto startPos = sampleToPosition(int(sampleStart.getValue()));
    auto endPos = sampleToPosition(int(sampleEnd.getValue()) + 1);
    juce::Array targets = {
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
            targets.add(CompPart{ EditorParts::LOOP_END, juce::Rectangle<float>(sampleToPosition(int(loopEnd.getValue()) + 1) + 3 * lnf.EDITOR_BOUNDS_WIDTH / 2.f, 0.f, 1.f, float(getHeight())), 1 });
        }
    }
    return CompPart<EditorParts>::getClosestInRange(targets, x, y, lnf.DRAGGABLE_SNAP);
}

float SampleEditorOverlay::sampleToPosition(int sampleIndex)
{
    int start = viewStart.getValue();
    int end = viewEnd.getValue();
    return juce::jmap<float>(float(sampleIndex - start), 0.f, float(end - start + 1), 0.f, float(painterWidth));
}

int SampleEditorOverlay::positionToSample(float position)
{
    int start = viewStart.getValue();
    int end = viewEnd.getValue();
    return start + int(juce::jmap<float>(position, 0.f, float(painterWidth), 0.f, float(end - start + 1)));
}

/*
  ==============================================================================

    SampleEditor.cpp
    Created: 19 Sep 2023 2:03:29pm
    Author:  binya

  ==============================================================================
*/

SampleEditor::SampleEditor(APVTS& apvts, const juce::Array<CustomSamplerVoice*>& synthVoices) : apvts(apvts), overlay(apvts, synthVoices)
{
    apvts.state.addListener(this);
    apvts.addParameterListener(PluginParameters::MASTER_GAIN, this);

    painter.setGain(juce::Decibels::decibelsToGain(float(apvts.getParameterAsValue(PluginParameters::MASTER_GAIN).getValue())));
    addAndMakeVisible(&painter);

    overlay.toFront(true);
    addAndMakeVisible(&overlay);

    boundsSelector.toFront(true);
    addAndMakeVisible(&boundsSelector);
}

SampleEditor::~SampleEditor()
{
    apvts.state.removeListener(this);
    apvts.removeParameterListener(PluginParameters::MASTER_GAIN, this);
}

//==============================================================================
void SampleEditor::valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property)
{
    if (property.toString() == PluginParameters::UI_VIEW_START || property.toString() == PluginParameters::UI_VIEW_END)
    {
        int start = treeWhosePropertyHasChanged.getProperty(PluginParameters::UI_VIEW_START);
        int stop = treeWhosePropertyHasChanged.getProperty(PluginParameters::UI_VIEW_END);
        painter.setSampleView(start, stop);
    }
}

void SampleEditor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == PluginParameters::MASTER_GAIN)
    {
        painter.setGain(juce::Decibels::decibelsToGain(newValue));
    }
}

//==============================================================================
void SampleEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(lnf.BACKGROUND_COLOR);
}

void SampleEditor::resized()
{
    auto bounds = getLocalBounds();

    overlay.setBounds(bounds);
    painter.setBounds(getPainterBounds());
    boundsSelector.setBounds(getPainterBounds());
}

void SampleEditor::enablementChanged()
{
    overlay.setEnabled(isEnabled());
    painter.setEnabled(isEnabled());
}

juce::Rectangle<int> SampleEditor::getPainterBounds() const
{
    auto bounds = getLocalBounds();
    bounds.removeFromLeft(lnf.EDITOR_BOUNDS_WIDTH);
    bounds.removeFromRight(lnf.EDITOR_BOUNDS_WIDTH);
    return bounds;
}

//==============================================================================
void SampleEditor::setSample(const juce::AudioBuffer<float>& sample, bool initialLoad)
{
    if (!initialLoad || recordingMode) 
    {
        painter.setSample(sample);
    }
    else
    {
        painter.setSample(sample, apvts.state.getProperty(PluginParameters::UI_VIEW_START), apvts.state.getProperty(PluginParameters::UI_VIEW_END));
    }
    overlay.setSample(sample);
}

void SampleEditor::setRecordingMode(bool recording)
{
    recordingMode = recording;
    overlay.setRecordingMode(recordingMode);
}

bool SampleEditor::isRecordingMode() const
{
    return recordingMode;
}

void SampleEditor::sampleUpdated(int oldSize, int newSize)
{
    painter.appendToPath(oldSize, newSize - 1);
}

//==============================================================================
void SampleEditor::promptBoundsSelection(const juce::String& text, const std::function<void(int, int)>& callback)
{
    overlay.setEnabled(false);
    painter.setEnabled(false);
    boundsSelector.promptRangeSelect(text, [this, callback](int startPos, int endPos) -> void {
        return callback(overlay.positionToSample(startPos), overlay.positionToSample(endPos));
    });
}

void SampleEditor::cancelBoundsSelection()
{
    overlay.setEnabled(true);
    painter.setEnabled(true);
    boundsSelector.cancelRangeSelect();
}

bool SampleEditor::isInBoundsSelection() const
{
    return boundsSelector.isSelectingRange();
}

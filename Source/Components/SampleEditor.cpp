/*
  ==============================================================================

    SampleEditorOverlay.cpp
    Created: 19 Sep 2023 2:03:29pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>

#include "SampleEditor.h"

SampleEditorOverlay::SampleEditorOverlay(const APVTS& apvts, PluginParameters::State& pluginState, const juce::Array<CustomSamplerVoice*>& synthVoices) : synthVoices(synthVoices),
    viewStart(pluginState.viewStart),
    viewEnd(pluginState.viewEnd),
    sampleStart(pluginState.sampleStart),
    sampleEnd(pluginState.sampleEnd),
    loopStart(pluginState.loopStart),
    loopEnd(pluginState.loopEnd),
    isLooping(dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(PluginParameters::IS_LOOPING))),
    loopingHasStart(dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(PluginParameters::LOOPING_HAS_START))),
    loopingHasEnd(dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(PluginParameters::LOOPING_HAS_END))),
    isLoopingAttachment(*isLooping, [this](bool newValue) { repaint(); }, apvts.undoManager),
    loopingHasStartAttachment(*loopingHasStart, [this](bool newValue) { repaint(); }, apvts.undoManager),
    loopingHasEndAttachment(*loopingHasEnd, [this](bool newValue) { repaint(); }, apvts.undoManager)
{
    viewStart.addListener(this);
    viewEnd.addListener(this);
    sampleStart.addListener(this);
    sampleEnd.addListener(this);
    loopStart.addListener(this);
    loopEnd.addListener(this);
}

SampleEditorOverlay::~SampleEditorOverlay()
{
    viewStart.removeListener(this);
    viewEnd.removeListener(this);
    sampleStart.removeListener(this);
    sampleEnd.removeListener(this);
    loopStart.removeListener(this);
    loopEnd.removeListener(this);
}

//==============================================================================
void SampleEditorOverlay::valueChanged(ListenableValue<int>& source, int newValue)
{
    // This is necessary because on sample load (at least when recording ends), these callbacks happen on the wrong thread
    juce::MessageManager::callAsync([this] { repaint(); });
}

void SampleEditorOverlay::paint(juce::Graphics& g)
{
    if (!sampleBuffer || !sampleBuffer->getNumSamples() || recordingMode)
        return;

    using namespace juce;

    // Draw voice positions
    for (auto& voice : synthVoices)
    {
        if (voice->isPlaying())
        {
            auto location = voice->getPosition();
            auto pos = jmap<float>(location - viewStart, 0., viewEnd - viewStart, 0., double(getWidth()));

            Path voicePosition{};
            voicePosition.addLineSegment(Line<float>(pos, 0, pos, getHeight()), 1);

            g.setColour(Colors::WHITE.withAlpha(voice->getEnvelopeGain()));
            g.strokePath(voicePosition, PathStrokeType(1.f));
        }
    }

    float boundsWidth = getBoundsWidth();
    float boundsSeparation = 5 * boundsWidth;
    float handleStrokeWidth = Layout::handleWidth * getWidth();

    float startPos = sampleToPosition(sampleStart);
    float endPos = sampleToPosition(sampleEnd) + boundsWidth;
    float loopStartPos = sampleToPosition(loopStart);
    float loopEndPos = sampleToPosition(loopEnd) + boundsWidth;

    // Paint the backgrounds
    if (isLooping->get())
    {
        g.setColour(disabled(Colors::LOOP.withAlpha(0.07f)));
        g.fillRect(Rectangle(startPos, 0.f, endPos - startPos, float(getHeight())));
    }
  
    g.setColour(disabled(Colors::SLATE.withAlpha(0.15f)));
    g.fillRect(Rectangle(0.f, 0.f, isLooping->get() && loopingHasStart->get() ? loopStartPos : startPos, float(getHeight())));
    g.fillRect(Rectangle(isLooping->get() && loopingHasEnd->get() ? loopEndPos : endPos, 0.f, float(getWidth()) - endPos, float(getHeight())));

    // Paint the start bound
    Path startPosPath;
    startPosPath.addRectangle(startPos, 0.f, boundsWidth, float(getHeight()));
    (isLooping->get() ? loopBoundsShadow : boundsShadow).render(g, startPosPath);
    g.setColour(isLooping->get() ? 
         (dragging && draggingTarget == EditorParts::SAMPLE_START ? Colors::LOOP.withAlpha(0.5f) : disabled(Colors::LOOP))
        : (dragging && draggingTarget == EditorParts::SAMPLE_START ? Colors::SLATE.withAlpha(0.5f) : disabled(Colors::SLATE)));
    g.fillPath(startPosPath);

    if (isMouseOverOrDragging() && (!(isLooping->get() && loopingHasStart->get()) || startPos - loopStartPos > boundsSeparation))
        g.strokePath(handleLeft, PathStrokeType(handleStrokeWidth), AffineTransform::translation(startPos, 0.f));
    if (isMouseOverOrDragging() && endPos - startPos > boundsSeparation)
        g.strokePath(handleRight, PathStrokeType(handleStrokeWidth), AffineTransform::translation(startPos, 0.f));
    
    // Paint the end bound
    Path endPosPath;
    endPosPath.addRectangle(endPos, 0.f, boundsWidth, float(getHeight()));
    (isLooping->get() ? loopBoundsShadow : boundsShadow).render(g, endPosPath);
    g.setColour(isLooping->get() ?
        (dragging && draggingTarget == EditorParts::SAMPLE_END ? Colors::LOOP.withAlpha(0.5f) : disabled(Colors::LOOP))
        : (dragging && draggingTarget == EditorParts::SAMPLE_END ? Colors::SLATE.withAlpha(0.5f) : disabled(Colors::SLATE)));
    g.fillPath(endPosPath);

    if (isMouseOverOrDragging() && endPos - startPos > boundsSeparation)
        g.strokePath(handleLeft, PathStrokeType(handleStrokeWidth), AffineTransform::translation(endPos, 0.f));
    if (isMouseOverOrDragging() && (!(isLooping->get() && loopingHasEnd->get()) || loopEndPos - endPos > boundsSeparation))
        g.strokePath(handleRight, PathStrokeType(handleStrokeWidth), AffineTransform::translation(endPos, 0.f));
  
    // Paint the loop bounds
    if (isLooping->get())
    {
        if (loopingHasStart->get())
        {
            Path loopStartPath;
            loopStartPath.addRectangle(loopStartPos, 0.f, boundsWidth, float(getHeight()));
            boundsShadow.render(g, loopStartPath);
            g.setColour(dragging && draggingTarget == EditorParts::LOOP_START ? Colors::SLATE.withAlpha(0.5f) : disabled(Colors::SLATE));
            g.fillPath(loopStartPath);

            if (isMouseOverOrDragging())
                g.strokePath(handleLeft, PathStrokeType(handleStrokeWidth), AffineTransform::translation(loopStartPos, 0.f));
            if (isMouseOverOrDragging() && startPos - loopStartPos > boundsSeparation)
                g.strokePath(handleRight, PathStrokeType(handleStrokeWidth), AffineTransform::translation(loopStartPos, 0.f));
        }
        if (loopingHasEnd->get()) 
        {
            Path loopEndPath;
            loopEndPath.addRectangle(loopEndPos, 0.f, boundsWidth, float(getHeight()));
            boundsShadow.render(g, loopEndPath);
            g.setColour(dragging && draggingTarget == EditorParts::LOOP_END ? Colors::SLATE.withAlpha(0.5f) : disabled(Colors::SLATE));
            g.fillPath(loopEndPath);

            if (isMouseOverOrDragging() && loopEndPos - endPos > boundsSeparation)
                g.strokePath(handleLeft, PathStrokeType(handleStrokeWidth, PathStrokeType::curved, PathStrokeType::rounded), AffineTransform::translation(loopEndPos, 0.f));
            if (isMouseOverOrDragging())
                g.strokePath(handleRight, PathStrokeType(handleStrokeWidth, PathStrokeType::curved, PathStrokeType::rounded), AffineTransform::translation(loopEndPos, 0.f));
        }
    }

    Path innerShadowPath;
    innerShadowPath.addRectangle(getLocalBounds());
    innerShadow.render(g, innerShadowPath);
}

void SampleEditorOverlay::resized()
{
    int offset = int(0.0005f * getWidth());
    boundsShadow.setOffset({ offset, 0 }, 0);
    boundsShadow.setOffset({ -offset, 0 }, 1);
    loopBoundsShadow.setOffset({ offset, 0 }, 0);
    loopBoundsShadow.setOffset({ -offset, 0 }, 1);

    int innerShadowOffset = int(0.001f * getWidth());
    innerShadow.setOffset({ 0, innerShadowOffset }, 0);
    innerShadow.setOffset({ 0, -innerShadowOffset }, 1);

    float bounds = getBoundsWidth();
    float handleHeight = 0.04f * getWidth();

    handleLeft.clear();
    handleLeft.startNewSubPath(-bounds / 2.f, (getHeight() + handleHeight) / 2.f);
    handleLeft.lineTo(-bounds * 4.f / 3.f, getHeight() / 2.f);
    handleLeft.lineTo(-bounds / 2.f, (getHeight() - handleHeight) / 2.f);

    handleRight.clear();
    handleRight.startNewSubPath(bounds + bounds / 2.f, (getHeight() + handleHeight) / 2.f);
    handleRight.lineTo(bounds + bounds * 4.f / 3.f, getHeight() / 2.f);
    handleRight.lineTo(bounds + bounds / 2.f, (getHeight() - handleHeight) / 2.f);
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
    auto newSample = positionToSample(float(event.getMouseDownX() + event.getOffsetFromDragStart().getX() - getBoundsWidth()));

    // A bit hacky, to use the same function definition we need to add/subtract MINIMUM_BOUNDS_DISTANCE, since there doesn't need to be a minimum distance from view bounds
    switch (draggingTarget)
    {
    case EditorParts::SAMPLE_START:
        sampleStart = limitBounds(sampleStart, newSample,
            isLooping->get() && loopingHasStart->get() ? loopStart.load() : viewStart - lnf.MINIMUM_BOUNDS_DISTANCE, sampleEnd);
        break;
    case EditorParts::SAMPLE_END:
        sampleEnd = limitBounds(sampleEnd, newSample,
            sampleStart, isLooping->get() && loopingHasEnd->get() ? int(loopEnd) : viewEnd + lnf.MINIMUM_BOUNDS_DISTANCE);
        break;
    case EditorParts::LOOP_START:
        loopStart = limitBounds(loopStart, newSample, viewStart - lnf.MINIMUM_BOUNDS_DISTANCE, int(sampleStart));
        break;
    case EditorParts::LOOP_END:
        loopEnd = limitBounds(loopEnd, newSample, int(sampleEnd), viewEnd + lnf.MINIMUM_BOUNDS_DISTANCE);
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
int SampleEditorOverlay::limitBounds(int previousValue, int sample, int start, int end) const
{
    if (sample <= previousValue)
        return juce::jmax(sample, juce::jmin(previousValue, start + lnf.MINIMUM_BOUNDS_DISTANCE));
    else if (sample >= previousValue)
        return juce::jmin(sample, juce::jmax(previousValue, end - lnf.MINIMUM_BOUNDS_DISTANCE));
    else
        return juce::jlimit<int>(start + lnf.MINIMUM_BOUNDS_DISTANCE, end - lnf.MINIMUM_BOUNDS_DISTANCE, sample);
}

EditorParts SampleEditorOverlay::getClosestPartInRange(int x, int y) const
{
    auto startPos = sampleToPosition(sampleStart);
    auto endPos = sampleToPosition(sampleEnd);
    juce::Array targets = {
        CompPart {EditorParts::SAMPLE_START, juce::Rectangle(startPos + getBoundsWidth() / 2.f, 0.f, 1.f, float(getHeight())), 1},
        CompPart {EditorParts::SAMPLE_END, juce::Rectangle(endPos + 3 * getBoundsWidth() / 2.f, 0.f, 1.f, float(getHeight())), 1},
    };
    if (isLooping->get())
    {
        if (loopingHasStart->get())
            targets.add(CompPart{ EditorParts::LOOP_START, juce::Rectangle(sampleToPosition(loopStart) + getBoundsWidth() / 2.f, 0.f, 1.f, float(getHeight())), 1 });
        if (loopingHasEnd->get())
            targets.add(CompPart{ EditorParts::LOOP_END, juce::Rectangle(sampleToPosition(loopEnd + 1) + 3 * getBoundsWidth() / 2.f, 0.f, 1.f, float(getHeight())), 1 });
    }
    return CompPart<EditorParts>::getClosestInRange(targets, x, y, lnf.DRAGGABLE_SNAP);
}

float SampleEditorOverlay::getBoundsWidth() const
{
    return Layout::boundsWidth * getWidth();
}

float SampleEditorOverlay::sampleToPosition(int sampleIndex) const
{
    return juce::jmap<float>(float(sampleIndex - viewStart), 0.f, float(viewEnd - viewStart), 0.f, float(getWidth() - 2 * getBoundsWidth()));
}

int SampleEditorOverlay::positionToSample(float position) const
{
    return viewStart + int(std::round(juce::jmap<float>(position, 0.f, float(getWidth() - 2 * getBoundsWidth()), 0.f, float(viewEnd - viewStart))));
}

/*
  ==============================================================================

    SampleEditor.cpp
    Created: 19 Sep 2023 2:03:29pm
    Author:  binya

  ==============================================================================
*/

SampleEditor::SampleEditor(APVTS& apvts, PluginParameters::State& pluginState, const juce::Array<CustomSamplerVoice*>& synthVoices, const std::function<void(const juce::MouseWheelDetails& details, int centerSample)>& navScrollFunc) :
    apvts(apvts), pluginState(pluginState),
    gainAttachment(*apvts.getParameter(PluginParameters::MASTER_GAIN), [this](float newValue) { painter.setGain(juce::Decibels::decibelsToGain(newValue)); }, apvts.undoManager),
    overlay(apvts, pluginState, synthVoices), scrollFunc(navScrollFunc)
{
    pluginState.viewStart.addListener(this);
    pluginState.viewEnd.addListener(this);

    painter.setGain(juce::Decibels::decibelsToGain(float(apvts.getParameterAsValue(PluginParameters::MASTER_GAIN).getValue())));
    addAndMakeVisible(&painter);

    overlay.toFront(true);
    addAndMakeVisible(&overlay);

    boundsSelector.toFront(true);
    addAndMakeVisible(&boundsSelector);
}

SampleEditor::~SampleEditor()
{
    pluginState.viewStart.removeListener(this);
    pluginState.viewEnd.removeListener(this);
}

//==============================================================================
void SampleEditor::valueChanged(ListenableValue<int>& source, int newValue)
{
    if ((&source == &pluginState.viewStart || &source == &pluginState.viewEnd) && pluginState.viewStart < pluginState.viewEnd)
    {
        painter.setSampleView(pluginState.viewStart, pluginState.viewEnd);
    }
}

//==============================================================================
void SampleEditor::resized()
{
    auto bounds = getLocalBounds();

    overlay.setBounds(bounds);

    bounds.reduce(int(Layout::boundsWidth * bounds.getWidth()), 0);
    painter.setBounds(bounds);
    boundsSelector.setBounds(bounds);
}

void SampleEditor::enablementChanged()
{
    overlay.setEnabled(isEnabled());
    painter.setEnabled(isEnabled());
}

void SampleEditor::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    int sample = positionToSample(event.position.getX());
    scrollFunc(wheel, sample);
}

int SampleEditor::positionToSample(float position) const
{
    if (!sampleBuffer)
        return 0;

    return juce::jlimit<int>(0, sampleBuffer->getNumSamples(), overlay.positionToSample(position - int(Layout::boundsWidth * getWidth())));
}

//==============================================================================
void SampleEditor::setSample(const juce::AudioBuffer<float>& sample, bool initialLoad)
{
    sampleBuffer = &sample;
    if (!initialLoad || recordingMode) 
    {
        painter.setSample(sample);
    }
    else
    {
        painter.setSample(sample, pluginState.viewStart, pluginState.viewEnd);
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
        return callback(positionToSample(startPos), positionToSample(endPos));
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

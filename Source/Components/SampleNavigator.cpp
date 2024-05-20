/*
  ==============================================================================

    SampleNavigator.cpp
    Created: 19 Sep 2023 4:41:12pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>

#include "SampleNavigator.h"

SampleNavigator::SampleNavigator(APVTS& apvts, const juce::Array<CustomSamplerVoice*>& synthVoices) : apvts(apvts), synthVoices(synthVoices)
{
    apvts.addParameterListener(PluginParameters::MASTER_GAIN, this);

    viewStart = apvts.state.getPropertyAsValue(PluginParameters::UI_VIEW_START, apvts.undoManager);
    viewEnd = apvts.state.getPropertyAsValue(PluginParameters::UI_VIEW_END, apvts.undoManager);
    sampleStart = apvts.state.getPropertyAsValue(PluginParameters::SAMPLE_START, apvts.undoManager);
    sampleEnd = apvts.state.getPropertyAsValue(PluginParameters::SAMPLE_END, apvts.undoManager);
    loopStart = apvts.state.getPropertyAsValue(PluginParameters::LOOP_START, apvts.undoManager);
    loopEnd = apvts.state.getPropertyAsValue(PluginParameters::LOOP_END, apvts.undoManager);
    isLooping = apvts.getParameterAsValue(PluginParameters::IS_LOOPING);
    loopHasStart = apvts.getParameterAsValue(PluginParameters::LOOPING_HAS_START);
    loopHasEnd = apvts.getParameterAsValue(PluginParameters::LOOPING_HAS_END);
    viewStart.addListener(this);
    viewEnd.addListener(this);

    painter.setGain(juce::Decibels::decibelsToGain(float(apvts.getParameterAsValue(PluginParameters::MASTER_GAIN).getValue())));
    addAndMakeVisible(&painter);

    painter.setInterceptsMouseClicks(false, false);
}

SampleNavigator::~SampleNavigator()
{
    apvts.removeParameterListener(PluginParameters::MASTER_GAIN, this);
    viewStart.removeListener(this);
    viewEnd.removeListener(this);
}

void SampleNavigator::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == PluginParameters::MASTER_GAIN)
    {
        painter.setGain(juce::Decibels::decibelsToGain(newValue));
    }
}

void SampleNavigator::valueChanged(juce::Value& value)
{
    repaint();
}

//==============================================================================
void SampleNavigator::setSample(const juce::AudioBuffer<float>& sampleBuffer, bool initialLoad)
{
    painter.setSample(sampleBuffer);
    sample = &sampleBuffer;
    if (!initialLoad || recordingMode)
    {
        viewStart = 0;
        viewEnd = sampleBuffer.getNumSamples() - 1;
    }
    repaint();
}

void SampleNavigator::sampleUpdated(int oldSize, int newSize)
{
    painter.appendToPath(oldSize, newSize - 1);
}

void SampleNavigator::setRecordingMode(bool recording)
{
    recordingMode = recording;
}

//==============================================================================
void SampleNavigator::paint(juce::Graphics& g)
{
    g.fillAll(lnf.BACKGROUND_COLOR);
}

void SampleNavigator::paintOverChildren(juce::Graphics& g)
{
    using namespace juce;

    if (sample && sample->getNumSamples())
    {
        // Paints the voice positions
        if (!recordingMode)
        {
            Path path{};
            for (auto& voice : synthVoices)
            {
                if (voice->isPlaying())
                {
                    auto location = voice->getPosition();
                    auto pos = jmap<float>(float(location), 0.f, float(sample->getNumSamples()), 0.f, float(painter.getWidth()));
                    path.addLineSegment(Line<int>(pos, 0, pos, getHeight()).toFloat(), 1.f);
                }
            }
            g.setColour(lnf.VOICE_POSITION_COLOR);
            g.strokePath(path, PathStrokeType(1.f));
        }

        // Paints the start and stop
        float startPos = sampleToPosition(recordingMode ? 0 : int(viewStart.getValue()));
        float stopPos = sampleToPosition(recordingMode ? sample->getNumSamples() - 1 : int(viewEnd.getValue()));
        g.setColour(lnf.SAMPLE_BOUNDS_COLOR.withAlpha(0.2f));
        g.fillRect(startPos, 0.f, stopPos - startPos + 1.f, float(getHeight()));

        g.setColour(dragging && draggingTarget == Drag::SAMPLE_START ? lnf.SAMPLE_BOUNDS_SELECTED_COLOR : disabled(lnf.SAMPLE_BOUNDS_COLOR));
        g.fillPath(startSamplePath, AffineTransform::translation(startPos, 0.f));
        g.setColour(dragging && draggingTarget == Drag::SAMPLE_END ? lnf.SAMPLE_BOUNDS_SELECTED_COLOR : disabled(lnf.SAMPLE_BOUNDS_COLOR));
        g.fillPath(stopSamplePath, AffineTransform::translation(stopPos + 1, 0.f));
    }
}

void SampleNavigator::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromLeft(lnf.NAVIGATOR_BOUNDS_WIDTH);
    bounds.removeFromRight(lnf.NAVIGATOR_BOUNDS_WIDTH);
    painter.setBounds(bounds);

    startSamplePath.clear();
    startSamplePath.addLineSegment(juce::Line<int>(0, 0, 0, getHeight()).toFloat(), float(lnf.NAVIGATOR_BOUNDS_WIDTH));
    startSamplePath.addLineSegment(juce::Line<int>(0, 0, 4, 0).toFloat(), 2.f);
    startSamplePath.addLineSegment(juce::Line<int>(0, getHeight(), 4, getHeight()).toFloat(), 2.f);

    stopSamplePath.clear();
    stopSamplePath.addLineSegment(juce::Line<int>(0, 0, 0, getHeight()).toFloat(), float(lnf.NAVIGATOR_BOUNDS_WIDTH));
    stopSamplePath.addLineSegment(juce::Line<int>(-4, 0, 0, 0).toFloat(), 2.f);
    stopSamplePath.addLineSegment(juce::Line<int>(-4, getHeight(), 0, getHeight()).toFloat(), 2.f);
}

void SampleNavigator::enablementChanged()
{
    painter.setEnabled(isEnabled());
    painter.repaint();
}

//==============================================================================
void SampleNavigator::mouseDown(const juce::MouseEvent& event)
{
    if (!sample || !isEnabled() || recordingMode)
        return;

    dragOriginStartSample = viewStart.getValue();
    draggingTarget = getDraggingTarget(event.getMouseDownX(), event.getMouseDownY());
    dragging = draggingTarget != Drag::NONE;
    repaint();
}

void SampleNavigator::mouseUp(const juce::MouseEvent& event)
{
    if (!sample || recordingMode)
        return;

    dragging = false;
    repaint();
}

void SampleNavigator::mouseMove(const juce::MouseEvent& event)
{
    if (!sample || !isEnabled() || recordingMode)
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    }

    Drag currentTarget = getDraggingTarget(event.getMouseDownX(), event.getMouseDownY());
    switch (currentTarget)
    {
    case Drag::SAMPLE_START:
    case Drag::SAMPLE_END:
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        break;
    case Drag::SAMPLE_FULL:
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        break;
    case Drag::NONE:
        setMouseCursor(juce::MouseCursor::NormalCursor);
        break;
    }
}

void SampleNavigator::mouseDrag(const juce::MouseEvent& event)
{
    if (!sample || !dragging || !isEnabled() || recordingMode)
        return;

    auto newSample = positionToSample(float(event.getMouseDownX() + event.getOffsetFromDragStart().getX()));
    auto startPos = sampleToPosition(viewStart.getValue());
    auto endPos = sampleToPosition(viewEnd.getValue());

    // The goal is to keep the positions within their normal constraints
    switch (draggingTarget)
    {
    case Drag::SAMPLE_START:
    {
        int newValue = juce::jlimit(0, int(viewEnd.getValue()) - lnf.MINIMUM_VIEW, newSample);

        // "Stick" the start bound to the view
        if (loopStart.getValue() == viewStart.getValue())
            loopStart = newValue;
        else if (sampleStart.getValue() == viewStart.getValue())
            sampleStart = newValue;

        // Update the bounds
        viewStart = newValue;

        // Each of these keeps the positions within their bounds, and note that limit(min, max, value) = max(min, min(max, value))
        loopStart = juce::jmax<int>(viewStart.getValue(), juce::jmin<int>(int(sampleStart.getValue()) - lnf.MINIMUM_BOUNDS_DISTANCE, loopStart.getValue()));
        int lowerSampleBound = isLooping.getValue() && loopHasStart.getValue() ? int(loopStart.getValue()) + lnf.MINIMUM_BOUNDS_DISTANCE : int(viewStart.getValue());
        sampleStart = juce::jmax<int>(lowerSampleBound, juce::jmin<int>(int(sampleEnd.getValue()) - lnf.MINIMUM_BOUNDS_DISTANCE, sampleStart.getValue()));
        sampleEnd = juce::jmax<int>(int(sampleStart.getValue()) + lnf.MINIMUM_BOUNDS_DISTANCE, juce::jmin<int>(int(loopEnd.getValue()) - lnf.MINIMUM_BOUNDS_DISTANCE, sampleEnd.getValue()));
        loopEnd = juce::jmax<int>(int(sampleEnd.getValue()) + lnf.MINIMUM_BOUNDS_DISTANCE, juce::jmin<int>(viewEnd.getValue(), loopEnd.getValue()));

        break;
    }
    case Drag::SAMPLE_END:
    {
        // Mirrored logic from above
        int newValue = juce::jlimit(int(viewStart.getValue()) + lnf.MINIMUM_VIEW, sample->getNumSamples() - 1, newSample);

        if (loopEnd.getValue() == viewEnd.getValue())
            loopEnd = newValue;
        else if (sampleEnd.getValue() == viewEnd.getValue())
            sampleEnd = newValue;

        viewEnd = newValue;

        loopEnd = juce::jmin<int>(viewEnd.getValue(), juce::jmax<int>(int(sampleEnd.getValue()) + lnf.MINIMUM_BOUNDS_DISTANCE, loopEnd.getValue()));
        int upperSampleBound = isLooping.getValue() && loopHasEnd.getValue() ? int(loopEnd.getValue()) - lnf.MINIMUM_BOUNDS_DISTANCE : int(viewEnd.getValue());
        sampleEnd = juce::jmin<int>(upperSampleBound, juce::jmax<int>(int(sampleStart.getValue()) + lnf.MINIMUM_BOUNDS_DISTANCE, sampleEnd.getValue()));
        sampleStart = juce::jmin<int>(int(sampleEnd.getValue()) - lnf.MINIMUM_BOUNDS_DISTANCE, juce::jmax<int>(int(loopStart.getValue()) + lnf.MINIMUM_BOUNDS_DISTANCE, sampleStart.getValue()));
        loopStart = juce::jmin<int>(int(sampleStart.getValue()) - lnf.MINIMUM_BOUNDS_DISTANCE, juce::jmax<int>(viewStart.getValue(), loopStart.getValue()));

        break;
    }
    case Drag::SAMPLE_FULL:
    {
        auto originStart = dragOriginStartSample;
        auto originStop = dragOriginStartSample + int(viewEnd.getValue()) - int(viewStart.getValue());
        auto sampleChange = juce::jlimit<int>(-originStart, sample->getNumSamples() - 1 - originStop,
            positionToSample(float(event.getOffsetFromDragStart().getX())));
        sampleStart = dragOriginStartSample + int(sampleStart.getValue()) - int(viewStart.getValue()) + sampleChange;
        sampleEnd = dragOriginStartSample + int(sampleEnd.getValue()) - int(viewStart.getValue()) + sampleChange;
        loopStart = dragOriginStartSample + int(loopStart.getValue()) - int(viewStart.getValue()) + sampleChange;
        loopEnd = dragOriginStartSample + int(loopEnd.getValue()) - int(viewStart.getValue()) + sampleChange;
        viewStart = originStart + sampleChange;
        viewEnd = originStop + sampleChange;
        break;
    }
    case Drag::NONE:
        break;
    }
}

//==============================================================================
NavigatorParts SampleNavigator::getDraggingTarget(int x, int y) const
{
    auto bounds = getLocalBounds().toFloat();
    auto startPos = sampleToPosition(viewStart.getValue());
    auto stopPos = sampleToPosition(viewEnd.getValue());

    // A little trick to make the full sample always possible to drag
    auto offset = juce::jmax<float>(30 - (stopPos - startPos), 0) / 2;

    juce::Array<CompPart<Drag>> targets = {
        CompPart {Drag::SAMPLE_START, juce::Rectangle<float>{startPos - offset, bounds.getY(), 0, bounds.getHeight()}, 2},
        CompPart {Drag::SAMPLE_END, juce::Rectangle<float>{stopPos + offset, bounds.getY(), 0, bounds.getHeight()}, 2},
        CompPart {Drag::SAMPLE_FULL, juce::Rectangle<float>{startPos, bounds.getY(), stopPos - startPos, bounds.getHeight()}, 1}
    };
    return CompPart<Drag>::getClosestInRange(targets, x, y, lnf.DRAGGABLE_SNAP);
}

float SampleNavigator::sampleToPosition(int sampleIndex) const
{
    return juce::jmap<float>(float(sampleIndex), 0.f, float(sample->getNumSamples()), 0.f, float(painter.getWidth())) + lnf.NAVIGATOR_BOUNDS_WIDTH;
}

int SampleNavigator::positionToSample(float position) const
{
    return int(juce::jmap<float>(position - lnf.NAVIGATOR_BOUNDS_WIDTH, 0.f, float(painter.getWidth()), 0.f, float(sample->getNumSamples())));
}
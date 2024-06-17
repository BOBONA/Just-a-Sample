/*
  ==============================================================================

    SampleNavigator.cpp
    Created: 19 Sep 2023 4:41:12pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>

#include "SampleNavigator.h"

SampleNavigator::SampleNavigator(APVTS& apvts, PluginParameters::State& pluginState, const juce::Array<CustomSamplerVoice*>& synthVoices) :
    apvts(apvts), state(pluginState), synthVoices(synthVoices),
    isLooping(dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(PluginParameters::IS_LOOPING))),
    loopHasStart(dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(PluginParameters::LOOPING_HAS_START))),
    loopHasEnd(dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(PluginParameters::LOOPING_HAS_END)))
{
    apvts.addParameterListener(PluginParameters::MASTER_GAIN, this);

    state.viewStart.addListener(this);
    state.viewEnd.addListener(this);

    painter.setGain(juce::Decibels::decibelsToGain(float(apvts.getParameterAsValue(PluginParameters::MASTER_GAIN).getValue())));
    addAndMakeVisible(&painter);

    painter.setInterceptsMouseClicks(false, false);
}

SampleNavigator::~SampleNavigator()
{
    apvts.removeParameterListener(PluginParameters::MASTER_GAIN, this);
    state.viewStart.removeListener(this);
    state.viewEnd.removeListener(this);
}

void SampleNavigator::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == PluginParameters::MASTER_GAIN)
    {
        painter.setGain(juce::Decibels::decibelsToGain(newValue));
    }
}

void SampleNavigator::valueChanged(ListenableValue<int>& source, int newValue)
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
        state.viewStart = 0;
        state.viewEnd = sampleBuffer.getNumSamples() - 1;
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
            for (auto& voice : synthVoices)
            {
                if (voice->isPlaying())
                {
                    auto location = voice->getPosition();
                    auto pos = jmap<float>(float(location), 0.f, float(sample->getNumSamples()), 0.f, float(painter.getWidth()));

                    Path voicePath{};
                    voicePath.addLineSegment(Line<float>(pos, 0, pos, getHeight()), 1.f);
                    g.setColour(lnf.VOICE_POSITION_COLOR.withAlpha(voice->getEnvelopeGain()));
                    g.strokePath(voicePath, PathStrokeType(1.f));
                }
            }
        }

        // Paints the start and stop
        float startPos = sampleToPosition(recordingMode ? 0 : int(state.viewStart));
        float stopPos = sampleToPosition(recordingMode ? sample->getNumSamples() - 1 : int(state.viewEnd));
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

    dragOriginStartSample = state.viewStart;
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
    auto startPos = sampleToPosition(state.viewStart);
    auto endPos = sampleToPosition(state.viewEnd);

    // The goal is to keep the positions within their normal constraints
    switch (draggingTarget)
    {
    case Drag::SAMPLE_START:
    {
        int oldValue = state.viewStart;
        state.viewStart = juce::jlimit(0, state.viewEnd - lnf.MINIMUM_VIEW, newSample);

        // Each of these keeps the positions within their bounds, and note that limit(min, max, value) = max(min, min(max, value))
        if (state.loopStart == oldValue)
            state.loopStart = state.viewStart.load();  // "Stick" the start bound to the view
        else
            state.loopStart = juce::jmax<int>(state.viewStart, juce::jmin<int>(state.sampleStart - lnf.MINIMUM_BOUNDS_DISTANCE, state.loopStart));

        if (state.sampleStart == oldValue)
            state.sampleStart = state.viewStart.load();
        else
        {
            int lowerSampleBound = isLooping->get() && loopHasStart->get() ? state.loopStart + lnf.MINIMUM_BOUNDS_DISTANCE : int(state.viewStart);
            state.sampleStart = juce::jmax<int>(lowerSampleBound, juce::jmin<int>(state.sampleEnd - lnf.MINIMUM_BOUNDS_DISTANCE, state.sampleStart));
        }

        state.sampleEnd = juce::jmax<int>(state.sampleStart + lnf.MINIMUM_BOUNDS_DISTANCE, juce::jmin<int>(state.loopEnd - lnf.MINIMUM_BOUNDS_DISTANCE, state.sampleEnd));
        state.loopEnd = juce::jmax<int>(state.sampleEnd + lnf.MINIMUM_BOUNDS_DISTANCE, juce::jmin<int>(state.viewEnd, state.loopEnd));

        break;
    }
    case Drag::SAMPLE_END:
    {
        // Mirrored logic from above
        int oldValue = state.viewEnd;
        state.viewEnd = juce::jlimit(state.viewStart + lnf.MINIMUM_VIEW, sample->getNumSamples() - 1, newSample);

        if (state.loopEnd == oldValue)
            state.loopEnd = state.viewEnd.load();
        else
            state.loopEnd = juce::jmin<int>(state.viewEnd, juce::jmax<int>(state.sampleEnd + lnf.MINIMUM_BOUNDS_DISTANCE, state.loopEnd));

        if (state.sampleEnd == oldValue)
            state.sampleEnd = state.viewEnd.load();
        else
        {
            int upperSampleBound = isLooping->get() && loopHasEnd->get() ? state.loopEnd - lnf.MINIMUM_BOUNDS_DISTANCE : int(state.viewEnd);
            state.sampleEnd = juce::jmin<int>(upperSampleBound, juce::jmax<int>(state.sampleStart + lnf.MINIMUM_BOUNDS_DISTANCE, state.sampleEnd));

        }

        state.sampleStart = juce::jmin<int>(state.sampleEnd - lnf.MINIMUM_BOUNDS_DISTANCE, juce::jmax<int>(state.loopStart + lnf.MINIMUM_BOUNDS_DISTANCE, state.sampleStart));
        state.loopStart = juce::jmin<int>(state.sampleStart - lnf.MINIMUM_BOUNDS_DISTANCE, juce::jmax<int>(state.viewStart, state.loopStart));

        break;
    }
    case Drag::SAMPLE_FULL:
    {
        auto originStart = dragOriginStartSample;
        auto originStop = dragOriginStartSample + state.viewEnd - state.viewStart;
        auto sampleChange = juce::jlimit<int>(-originStart, sample->getNumSamples() - 1 - originStop,
            positionToSample(float(event.getOffsetFromDragStart().getX())));
        state.sampleStart = dragOriginStartSample + state.sampleStart - state.viewStart + sampleChange;
        state.sampleEnd = dragOriginStartSample + state.sampleEnd - state.viewStart + sampleChange;
        state.loopStart = dragOriginStartSample + state.loopStart - state.viewStart + sampleChange;
        state.loopEnd = dragOriginStartSample + state.loopEnd - state.viewStart + sampleChange;
        state.viewStart = originStart + sampleChange;
        state.viewEnd = originStop + sampleChange;
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
    auto startPos = sampleToPosition(state.viewStart);
    auto stopPos = sampleToPosition(state.viewEnd);

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
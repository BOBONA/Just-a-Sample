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

    draggingTarget = getDraggingTarget(event.getMouseDownX(), event.getMouseDownY());
    if (draggingTarget == Drag::SAMPLE_FULL)
        dragSelectOffset = event.getMouseDownX() - sampleToPosition(state.viewStart);
    dragging = draggingTarget != Drag::NONE;
    lastDragOffset = 0.f;

    if (draggingTarget == Drag::SAMPLE_START || draggingTarget == Drag::SAMPLE_END || draggingTarget == Drag::SAMPLE_FULL)
        juce::Desktop::getInstance().getMainMouseSource().enableUnboundedMouseMovement(true, false);

    repaint();
}

void SampleNavigator::mouseUp(const juce::MouseEvent& event)
{
    if (!sample || recordingMode)
        return;

    dragging = false;

    juce::Desktop::getInstance().getMainMouseSource().enableUnboundedMouseMovement(false);
    auto screenPos = getScreenBounds();
    if (draggingTarget == Drag::SAMPLE_START)
        juce::Desktop::getInstance().getMainMouseSource().setScreenPosition(juce::Point<float>(screenPos.getX() + sampleToPosition(state.viewStart) * screenPos.getWidth() / getWidth(), screenPos.getCentreY()));
    else if (draggingTarget == Drag::SAMPLE_END)
        juce::Desktop::getInstance().getMainMouseSource().setScreenPosition(juce::Point<float>(screenPos.getX() + sampleToPosition(state.viewEnd) * screenPos.getWidth() / getWidth(), screenPos.getCentreY()));
    else if (draggingTarget == Drag::SAMPLE_FULL)
        juce::Desktop::getInstance().getMainMouseSource().setScreenPosition(juce::Point<float>(screenPos.getX() + (sampleToPosition(state.viewStart) + dragSelectOffset) * screenPos.getWidth() / getWidth(), screenPos.getCentreY()));

    repaint();
}

void SampleNavigator::mouseMove(const juce::MouseEvent& event)
{
    if (!sample || !isEnabled() || recordingMode)
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    }

    if (dragging)  // Don't change while dragging
        return;

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

    float viewSize = state.viewEnd - state.viewStart + 1;

    // The goal is to keep the positions within their normal constraints
    switch (draggingTarget)
    {
    case Drag::SAMPLE_START:
    {
        float difference = event.getOffsetFromDragStart().getX() - lastDragOffset;
        moveStart(difference / 2);
        moveEnd(-difference / 2);

        break;
    }
    case Drag::SAMPLE_END:
    {
        float difference = event.getOffsetFromDragStart().getX() - lastDragOffset;
        moveEnd(difference / 2);
        moveStart(-difference / 2);

        break;
    }
    case Drag::SAMPLE_FULL:
    {
        float change = event.getOffsetFromDragStart().getX() - lastDragOffset;
        moveBoth(change);

        break;
    }
    case Drag::NONE:
        break;
    }

    lastDragOffset = event.getOffsetFromDragStart().getX();
}

void SampleNavigator::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (dragging)
        return;

    float changeY = -wheel.deltaY * lnf.MOUSE_SENSITIVITY;
    float changeX = wheel.deltaX * lnf.MOUSE_SENSITIVITY;

    if (wheel.isSmooth)
        changeY *= 0.5f;

    bool treatTrackpad = changeX != 0;
    bool modifier = juce::ModifierKeys::currentModifiers.isAnyModifierKeyDown();

    if (modifier || treatTrackpad)
    {
        moveBoth(treatTrackpad ? changeX : -changeY, false, !modifier);
    }

    if (!modifier || treatTrackpad)
    {
        float sample = positionToSample(event.position.getX());
        if (sample >= state.viewStart && sample <= state.viewEnd && changeY > 0)
        {
            float shortRatio = (state.viewEnd - sample) / (state.viewEnd - state.viewStart);
            moveStart(changeY * (1 - shortRatio), false);
            moveEnd(-changeY * shortRatio, false);
        }
        else
        {
            moveStart(changeY, false);
            moveEnd(-changeY, false);
        }
    }
}

void SampleNavigator::moveStart(float change, bool checkSecondary) const
{
    if (!change)
        return;

    // Check if the bounds should stick
    auto& start = isLooping->get() && loopHasStart->get() ? state.loopStart : state.sampleStart;
    bool stick = state.viewStart.load() == start.load();

    // Move the view
    state.viewStart = juce::jlimit<int>(0, state.viewEnd - lnf.MINIMUM_VIEW, state.viewStart + change * getDragSensitivity(checkSecondary));

    // Maintain bound constraints
    if (isLooping->get() && loopHasStart->get())
        state.loopStart = juce::jmax<int>(state.loopStart, state.viewStart);
    state.sampleStart = juce::jmax<int>(state.sampleStart, isLooping->get() && loopHasStart->get() ? state.loopStart + lnf.MINIMUM_BOUNDS_DISTANCE : float(state.viewStart));
    state.sampleEnd = juce::jmax<int>(state.sampleEnd, state.sampleStart + lnf.MINIMUM_BOUNDS_DISTANCE);
    if (isLooping->get() && loopHasEnd->get())
        state.loopEnd = juce::jmax<int>(state.loopEnd, state.sampleEnd + lnf.MINIMUM_BOUNDS_DISTANCE);

    // Stick functionality
    if (stick)
        start = state.viewStart.load();
}

void SampleNavigator::moveEnd(float change, bool checkSecondary) const
{
    if (!change)
        return;

    // Check if the bound should stick
    auto& end = isLooping->get() && loopHasEnd->get() ? state.loopEnd : state.sampleEnd;
    bool stick = state.viewEnd == end;

    // Move the view
    state.viewEnd = juce::jlimit<int>(state.viewStart + lnf.MINIMUM_VIEW, sample->getNumSamples() - 1, state.viewEnd + change * getDragSensitivity(checkSecondary));

    // Maintain bound constraints
    if (isLooping->get() && loopHasEnd->get())
        state.loopEnd = juce::jmin<int>(state.loopEnd, state.viewEnd);
    state.sampleEnd = juce::jmin<int>(state.sampleEnd, isLooping->get() && loopHasEnd->get() ? state.loopEnd - lnf.MINIMUM_BOUNDS_DISTANCE : float(state.viewEnd));
    state.sampleStart = juce::jmin<int>(state.sampleStart, state.sampleEnd - lnf.MINIMUM_BOUNDS_DISTANCE);
    if (isLooping->get() && loopHasStart->get())
        state.loopStart = juce::jmin<int>(state.loopStart, state.sampleStart - lnf.MINIMUM_BOUNDS_DISTANCE);

    // Stick functionality
    if (stick)
        end = state.viewEnd.load();
}

void SampleNavigator::moveBoth(float change, bool checkSecondary, bool useSecondary) const
{
    float sensitivity = getDragSensitivity(checkSecondary, useSecondary);
    float difference = juce::jlimit<float>(-state.viewStart, sample->getNumSamples() - 1 - state.viewEnd, change * sensitivity);

    state.viewStart = state.viewStart + difference;
    state.viewEnd = state.viewEnd + difference;
    state.loopStart = state.loopStart + difference;
    state.sampleStart = state.sampleStart + difference;
    state.sampleEnd = state.sampleEnd + difference;
    state.loopEnd = state.loopEnd + difference;
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

float SampleNavigator::getDragSensitivity(bool checkSecondary, bool useSecondary) const
{
    float viewSize = state.viewEnd - state.viewStart + 1;
    if ((checkSecondary && juce::ModifierKeys::currentModifiers.isAnyModifierKeyDown()) || (!checkSecondary && useSecondary))
        return sample->getNumSamples() / getWidth();
    else
        return -std::logf(viewSize / sample->getNumSamples() / juce::MathConstants<float>::euler) * viewSize / getWidth();
}

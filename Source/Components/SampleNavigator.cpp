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
    apvts(apvts), state(pluginState),
    gainAttachment(*apvts.getParameter(PluginParameters::MASTER_GAIN), [this](float newValue) { painter.setGain(juce::Decibels::decibelsToGain(newValue)); }, apvts.undoManager),
    synthVoices(synthVoices),
    isLooping(dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(PluginParameters::IS_LOOPING))),
    loopHasStart(dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(PluginParameters::LOOPING_HAS_START))),
    loopHasEnd(dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(PluginParameters::LOOPING_HAS_END))),
    loopAttachment(*isLooping, [this](bool newValue) { repaint(); }, apvts.undoManager),
    loopStartAttachment(*loopHasStart, [this](bool newValue) { repaint(); }, apvts.undoManager),
    loopEndAttachment(*loopHasEnd, [this](bool newValue) { repaint(); }, apvts.undoManager)
{
    state.viewStart.addListener(this);
    state.viewEnd.addListener(this);
    state.sampleStart.addListener(this);
    state.sampleEnd.addListener(this);
    state.loopStart.addListener(this);
    state.loopEnd.addListener(this);

    painter.setGain(juce::Decibels::decibelsToGain(float(apvts.getParameterAsValue(PluginParameters::MASTER_GAIN).getValue())));
    addAndMakeVisible(&painter);

    painter.setInterceptsMouseClicks(false, false);
}

SampleNavigator::~SampleNavigator()
{
    state.viewStart.removeListener(this);
    state.viewEnd.removeListener(this);
    state.sampleStart.removeListener(this);
    state.sampleEnd.removeListener(this);
    state.loopStart.removeListener(this);
    state.loopEnd.removeListener(this);
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
                    g.setColour(Colors::WHITE.withAlpha(voice->getEnvelopeGain()));
                    g.strokePath(voicePath, PathStrokeType(1.f));
                }
            }
        }

        float boundsThickness = getWidth() * Layout::navigatorBoundsWidth * 0.66f;
        float sampleStartPos = sampleToPosition(state.sampleStart);
        float sampleEndPos = sampleToPosition(state.sampleEnd);
        float loopStartPos = sampleToPosition(state.loopStart);
        float loopEndPos = sampleToPosition(state.loopEnd);
        g.setColour(disabled(isLooping->get() ? Colors::LOOP : Colors::SLATE));
        g.fillRect(sampleStartPos - boundsThickness, getHeight() * 0.1f, boundsThickness, getHeight() * 0.8f);
        g.setColour(disabled(isLooping->get() ? Colors::LOOP : Colors::SLATE));
        g.fillRect(sampleEndPos, getHeight() * 0.1f, boundsThickness, getHeight() * 0.8f);
        g.setColour(disabled(Colors::SLATE));
        if (isLooping->get() && loopHasStart->get())
            g.fillRect(loopStartPos - boundsThickness, getHeight() * 0.1f, boundsThickness, getHeight() * 0.8f);
        if (isLooping->get() && loopHasEnd->get())
            g.fillRect(loopEndPos, getHeight() * 0.1f, boundsThickness, getHeight() * 0.8f);

        // Paints the start and stop
        float startPos = sampleToPosition(recordingMode ? 0 : int(state.viewStart));
        float stopPos = sampleToPosition(recordingMode ? sample->getNumSamples() - 1 : int(state.viewEnd));
        g.setColour(disabled(Colors::SLATE.withAlpha(0.15f)));
        g.fillRect(startPos, 0.f, stopPos - startPos + 1.f, float(getHeight()));

        float lineThickness = getWidth() * Layout::navigatorBoundsWidth;
        g.setColour(disabled(Colors::SLATE));
        g.fillRect(startPos - lineThickness, 0.f, lineThickness, float(getHeight()));
        g.fillRect(stopPos, 0.f, lineThickness, float(getHeight()));
    }
}

void SampleNavigator::resized()
{
    auto bounds = getLocalBounds();
    float lineThickness = bounds.getWidth() * Layout::navigatorBoundsWidth;

    bounds.reduce(lineThickness, 0.f);
    painter.setBounds(bounds);
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
    float sensitivity = getDragSensitivity();
    switch (draggingTarget)
    {
    case Drag::SAMPLE_START:
    {
        float difference = event.getOffsetFromDragStart().getX() - lastDragOffset;
        moveStart(difference / 2, sensitivity);
        moveEnd(-difference / 2, sensitivity);

        break;
    }
    case Drag::SAMPLE_END:
    {
        float difference = event.getOffsetFromDragStart().getX() - lastDragOffset;
        moveEnd(difference / 2, sensitivity);
        moveStart(-difference / 2, sensitivity);

        break;
    }
    case Drag::SAMPLE_FULL:
    {
        float change = event.getOffsetFromDragStart().getX() - lastDragOffset;
        moveBoth(change, sensitivity);

        break;
    }
    case Drag::NONE:
        break;
    }

    lastDragOffset = event.getOffsetFromDragStart().getX();
}

void SampleNavigator::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (!sample || dragging || !isEnabled() || recordingMode)
        return;

    int sampleCenter = positionToSample(event.position.getX());
    scrollView(wheel, sampleCenter);
}

void SampleNavigator::scrollView(const juce::MouseWheelDetails& wheel, int sampleCenter, bool centerZoomOut) const
{
    float changeY = -wheel.deltaY * lnf.MOUSE_SENSITIVITY;
    float changeX = wheel.deltaX * lnf.MOUSE_SENSITIVITY;

    if (wheel.isSmooth)
        changeY *= 0.5f;

    bool treatTrackpad = changeX != 0;
    bool modifier = juce::ModifierKeys::currentModifiers.isAnyModifierKeyDown();

    if (modifier || treatTrackpad)
    {
        moveBoth(treatTrackpad ? changeX : -changeY, getDragSensitivity(false, !modifier));
    }

    if (!modifier || treatTrackpad)
    {
        float sensitivity = getDragSensitivity(false);
        float startRatio = 1.f;
        float endRatio = 1.f;
        if (sampleCenter >= state.viewStart && sampleCenter <= state.viewEnd && 
            ((centerZoomOut && state.viewStart > 0 && state.viewEnd < sample->getNumSamples() - 1) || changeY > 0))
        {
            endRatio = float(state.viewEnd - sampleCenter) / (state.viewEnd - state.viewStart);
            startRatio = 1.f - endRatio;
        }

        // The order must change depending on the direction of the wheel, since both maintain constraints and there's a minimum view size
        if (changeY > 0)
        {
            moveEnd(-changeY * endRatio, sensitivity);
            moveStart(changeY * startRatio, sensitivity);
        }
        else
        {
            moveStart(changeY * startRatio, sensitivity);
            moveEnd(-changeY * endRatio, sensitivity);
        }
    }
}

void SampleNavigator::fitView()
{
    if (!sample)
        return;

    state.viewStart = isLooping->get() && loopHasStart->get() ? state.loopStart.load() : state.sampleStart.load();
    state.viewEnd = isLooping->get() && loopHasEnd->get() ? state.loopEnd.load() : state.sampleEnd.load();
    repaint();
}

void SampleNavigator::moveStart(float change, float sensitivity) const
{
    if (!bool(change))
        return;

    int oldViewStart = state.viewStart;
    int viewEnd = state.viewEnd;

    auto viewStart = juce::jlimit<int>(0, state.viewEnd - lnf.MINIMUM_VIEW, int(std::floorf(state.viewStart + change * sensitivity)));
    state.viewStart = viewStart;

    if (!state.pinView)
        return;

    // To maintain constraints, we change the bounds in order depending on the direction of the change
    if (viewStart > oldViewStart)
    {
        state.loopEnd = std::round(long double(state.loopEnd - oldViewStart) / (viewEnd - oldViewStart) * (viewEnd - viewStart)) + viewStart;
        state.sampleEnd = std::round(long double(state.sampleEnd - oldViewStart) / (viewEnd - oldViewStart) * (viewEnd - viewStart)) + viewStart;
        state.sampleStart = std::round(long double(state.sampleStart - oldViewStart) / (viewEnd - oldViewStart) * (viewEnd - viewStart)) + viewStart;
        state.loopStart = std::round(long double(state.loopStart - oldViewStart) / (viewEnd - oldViewStart) * (viewEnd - viewStart)) + viewStart;
    }
    else
    {
        state.loopStart = std::round(long double(state.loopStart - oldViewStart) / (viewEnd - oldViewStart) * (viewEnd - viewStart)) + viewStart;
        state.sampleStart = std::round(long double(state.sampleStart - oldViewStart) / (viewEnd - oldViewStart) * (viewEnd - viewStart)) + viewStart;
        state.sampleEnd = std::round(long double(state.sampleEnd - oldViewStart) / (viewEnd - oldViewStart) * (viewEnd - viewStart)) + viewStart;
        state.loopEnd = std::round(long double(state.loopEnd - oldViewStart) / (viewEnd - oldViewStart) * (viewEnd - viewStart)) + viewStart;
    }
}

void SampleNavigator::moveEnd(float change, float sensitivity) const
{
    if (!bool(change))
        return;

    int oldViewEnd = state.viewEnd;
    int viewStart = state.viewStart;

    auto viewEnd = juce::jlimit<int>(state.viewStart + lnf.MINIMUM_VIEW, sample->getNumSamples() - 1, int(std::floorf(state.viewEnd + change * sensitivity)));
    state.viewEnd = viewEnd;

    if (!state.pinView)
        return;

    // To maintain constraints, we change the bounds in order depending on the direction of the change
    if (viewEnd < oldViewEnd)
    {
        state.loopStart = std::round(long double(state.loopStart - viewStart) / (oldViewEnd - viewStart) * (viewEnd - viewStart)) + viewStart;
        state.sampleStart = std::round(long double(state.sampleStart - viewStart) / (oldViewEnd - viewStart) * (viewEnd - viewStart)) + viewStart;
        state.sampleEnd = std::round(long double(state.sampleEnd - viewStart) / (oldViewEnd - viewStart) * (viewEnd - viewStart)) + viewStart;
        state.loopEnd = std::round(long double(state.loopEnd - viewStart) / (oldViewEnd - viewStart) * (viewEnd - viewStart)) + viewStart;
    }
    else
    {
        state.loopEnd = std::round(long double(state.loopEnd - viewStart) / (oldViewEnd - viewStart) * (viewEnd - viewStart)) + viewStart;
        state.sampleEnd = std::round(long double(state.sampleEnd - viewStart) / (oldViewEnd - viewStart) * (viewEnd - viewStart)) + viewStart;
        state.sampleStart = std::round(long double(state.sampleStart - viewStart) / (oldViewEnd - viewStart) * (viewEnd - viewStart)) + viewStart;
        state.loopStart = std::round(long double(state.loopStart - viewStart) / (oldViewEnd - viewStart) * (viewEnd - viewStart)) + viewStart;
    }
}

void SampleNavigator::moveBoth(float change, float sensitivity) const
{
    int difference = juce::jlimit<int>(-state.viewStart, sample->getNumSamples() - 1 - state.viewEnd, int(change * sensitivity));

    state.viewStart = state.viewStart + difference;
    state.viewEnd = state.viewEnd + difference;

    if (!state.pinView)
        return;

    // To maintain constraints, we change the bounds in order depending on the direction of the change
    if (difference > 0)
    {
        state.loopEnd = state.loopEnd + difference;
        state.sampleEnd = state.sampleEnd + difference;
        state.sampleStart = state.sampleStart + difference;
        state.loopStart = state.loopStart + difference;
    }
    else
    {
        state.loopStart = state.loopStart + difference;
        state.sampleStart = state.sampleStart + difference;
        state.sampleEnd = state.sampleEnd + difference;
        state.loopEnd = state.loopEnd + difference;
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

    juce::Array targets = {
        CompPart {Drag::SAMPLE_START, juce::Rectangle<float>{startPos - offset, bounds.getY(), 0, bounds.getHeight()}, 2},
        CompPart {Drag::SAMPLE_END, juce::Rectangle<float>{stopPos + offset, bounds.getY(), 0, bounds.getHeight()}, 2},
        CompPart {Drag::SAMPLE_FULL, juce::Rectangle<float>{startPos, bounds.getY(), stopPos - startPos, bounds.getHeight()}, 1}
    };
    return CompPart<Drag>::getClosestInRange(targets, x, y, lnf.DRAGGABLE_SNAP);
}

float SampleNavigator::sampleToPosition(int sampleIndex) const
{
    float boundsWidth = getWidth() * Layout::navigatorBoundsWidth;
    return juce::jmap<float>(float(sampleIndex), 0.f, float(sample->getNumSamples()), 0.f, float(painter.getWidth()) - boundsWidth) + boundsWidth;
}

int SampleNavigator::positionToSample(float position) const
{
    return int(juce::jmap<float>(position - getWidth() * Layout::navigatorBoundsWidth, 0.f, float(painter.getWidth()), 0.f, float(sample->getNumSamples())));
}

float SampleNavigator::getDragSensitivity(bool checkSecondary, bool useSecondary) const
{
    float viewSize = state.viewEnd - state.viewStart + 1;
    if ((checkSecondary && juce::ModifierKeys::currentModifiers.isAnyModifierKeyDown()) || (!checkSecondary && useSecondary))
        return sample->getNumSamples() / getWidth();
    else
        return -std::logf(viewSize / sample->getNumSamples() / juce::MathConstants<float>::euler) * viewSize / getWidth();
}

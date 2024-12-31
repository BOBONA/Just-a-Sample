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
    apvts(apvts), state(pluginState), dummyParam(apvts, PluginParameters::State::UI_DUMMY_PARAM),
    painter(1.f),
    gainAttachment(*apvts.getParameter(PluginParameters::SAMPLE_GAIN), [this](float newValue) { painter.setGain(juce::Decibels::decibelsToGain(newValue)); }, apvts.undoManager),
    synthVoices(synthVoices),
    isLooping(dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(PluginParameters::IS_LOOPING))),
    loopHasStart(dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(PluginParameters::LOOPING_HAS_START))),
    loopHasEnd(dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(PluginParameters::LOOPING_HAS_END))),
    loopAttachment(*isLooping, [this](bool newValue) { loopHasStartUpdate(newValue && loopHasStart->get()); loopHasEndUpdate(newValue && loopHasEnd->get()); }, apvts.undoManager),
    loopStartAttachment(*loopHasStart, [this](bool newValue) { loopHasStartUpdate(newValue); }, apvts.undoManager),
    loopEndAttachment(*loopHasEnd, [this](bool newValue) { loopHasEndUpdate(newValue); }, apvts.undoManager)
{
    state.viewStart.addListener(this);
    state.viewEnd.addListener(this);
    state.sampleStart.addListener(this);
    state.sampleEnd.addListener(this);
    state.loopStart.addListener(this);
    state.loopEnd.addListener(this);
    state.pinView.addListener(this);

    updatePinnedPositions();

    painter.setGain(juce::Decibels::decibelsToGain(float(apvts.getParameterAsValue(PluginParameters::SAMPLE_GAIN).getValue())));
    painter.setColour(Colors::painterColorId, Colors::DARKER_SLATE);
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
    state.pinView.removeListener(this);
}

void SampleNavigator::valueChanged(ListenableValue<int>& source, int /*newValue*/)
{
    auto& sourceA = dynamic_cast<ListenableAtomic<int>&>(source);
    if (!navigatorUpdate && (sourceA == state.loopStart || sourceA == state.loopEnd || sourceA == state.sampleStart || sourceA == state.sampleEnd))
        updatePinnedPositions();

    safeRepaint();
}

void SampleNavigator::valueChanged(ListenableValue<bool>& source, bool newValue)
{
    // When the pin is enabled, we move the bounds to within the viewport
    updatePinnedPositions();

    bool loopingHasStart = isLooping->get() && loopHasStart->get() && !isWaveformMode();
    bool loopingHasEnd = isLooping->get() && loopHasEnd->get() && !isWaveformMode();

    if (state.pinView && dynamic_cast<ListenableAtomic<bool>&>(source) == state.pinView && newValue && 
        ((loopingHasStart && state.loopStart < state.viewStart) || (loopingHasEnd && state.loopEnd > state.viewEnd) || 
            state.sampleStart < state.viewStart || state.sampleEnd > state.viewEnd))
    {
        int currentViewStart = state.viewStart;
        int currentViewEnd = state.viewEnd;

        int newViewStart = loopingHasStart ? state.loopStart.load() : state.sampleStart.load();
        int newViewEnd = loopingHasEnd ? state.loopEnd.load() : state.sampleEnd.load();

        state.viewStart = newViewStart;
        state.viewEnd = newViewEnd;
        updatePinnedPositions();
        state.viewStart = currentViewStart;
        state.viewEnd = currentViewEnd;
        moveBoundsToPinnedPositions(newViewStart > currentViewStart);
    }
}

//==============================================================================
void SampleNavigator::setSample(const juce::AudioBuffer<float>& sampleBuffer, float bufferSampleRate, bool resetView)
{
    painter.setSample(sampleBuffer);
    sample = &sampleBuffer;
    sampleRate = bufferSampleRate;

    if (resetView || recordingMode)
    {
        state.viewStart = 0;
        state.viewEnd = sampleBuffer.getNumSamples() - 1;
        updatePinnedPositions();
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
void SampleNavigator::paintOverChildren(juce::Graphics& g)
{
    using namespace juce;

    if (sample && sample->getNumSamples())
    {
        // Paints the voice positions
        if (!recordingMode && !isWaveformMode())
        {
            for (auto& voice : synthVoices)
            {
                if (voice->isPlaying())
                {
                    int location = int(std::ceil(voice->getPosition()));
                    auto pos = sampleToPosition(location);

                    Path voicePath{};
                    voicePath.addLineSegment(Line<float>(pos, 0.f, pos, float(getHeight())), 1.f);
                    g.setColour(Colors::WHITE.withAlpha(voice->getEnvelopeGain()));
                    g.strokePath(voicePath, PathStrokeType(Layout::playheadWidth * getWidth()));
                }
            }
        }

        float boundsThickness = getWidth() * Layout::navigatorBoundsWidth * 0.66f;
        float sampleStartPos = sampleToPosition(state.sampleStart);
        float sampleEndPos = sampleToPosition(state.sampleEnd);
        float loopStartPos = sampleToPosition(state.loopStart);
        float loopEndPos = sampleToPosition(state.loopEnd);

        auto boundColor = isWaveformMode() ? Colors::HIGHLIGHT : isLooping->get() ? Colors::LOOP : Colors::SLATE;

        g.setColour(disabled(boundColor));
        g.fillRect(sampleStartPos - boundsThickness, getHeight() * 0.1f, boundsThickness, getHeight() * 0.8f);
        g.setColour(disabled(boundColor));
        g.fillRect(sampleEndPos, getHeight() * 0.1f, boundsThickness, getHeight() * 0.8f);

        g.setColour(disabled(Colors::SLATE));
        if (isLooping->get() && loopHasStart->get() && !isWaveformMode())
            g.fillRect(loopStartPos - boundsThickness, getHeight() * 0.1f, boundsThickness, getHeight() * 0.8f);
        if (isLooping->get() && loopHasEnd->get() && !isWaveformMode())
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
    auto bounds = getLocalBounds().toFloat();
    float lineThickness = bounds.getWidth() * Layout::navigatorBoundsWidth;

    bounds.reduce(lineThickness, 0.f);
    painter.setBounds(bounds.toNearestInt());
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
    lastDragOffset = 0;

    if (draggingTarget == Drag::SAMPLE_START || draggingTarget == Drag::SAMPLE_END || draggingTarget == Drag::SAMPLE_FULL)
        juce::Desktop::getInstance().getMainMouseSource().enableUnboundedMouseMovement(true, false);

    repaint();
}

void SampleNavigator::mouseUp(const juce::MouseEvent&)
{
    if (!sample || recordingMode)
        return;

    if (dragging)
        dummyParam.sendUIUpdate();

    dragging = false;

    juce::Desktop::getInstance().getMainMouseSource().enableUnboundedMouseMovement(false);
    auto screenPos = getScreenBounds().toFloat();
    juce::Point<float> newMousePos;
    if (draggingTarget == Drag::SAMPLE_START)
        newMousePos = juce::Point(screenPos.getX() + sampleToPosition(state.viewStart) * screenPos.getWidth() / getWidth(), screenPos.getCentreY());
    else if (draggingTarget == Drag::SAMPLE_END)
        newMousePos = juce::Point(screenPos.getX() + sampleToPosition(state.viewEnd) * screenPos.getWidth() / getWidth(), screenPos.getCentreY());
    else if (draggingTarget == Drag::SAMPLE_FULL)
        newMousePos = juce::Point(screenPos.getX() + (sampleToPosition(state.viewStart) + dragSelectOffset) * screenPos.getWidth() / getWidth(), screenPos.getCentreY());
    juce::Desktop::getInstance().getMainMouseSource().setScreenPosition(newMousePos);

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

    // The goal is to keep the positions within their normal constraints
    float sensitivity = getDragSensitivity();
    switch (draggingTarget)
    {
    case Drag::SAMPLE_START:
    {
        int difference = event.getOffsetFromDragStart().getX() - lastDragOffset;
        moveStart(difference / 2.f, sensitivity);
        moveEnd(-difference / 2.f, sensitivity);

        break;
    }
    case Drag::SAMPLE_END:
    {
        int difference = event.getOffsetFromDragStart().getX() - lastDragOffset;
        moveEnd(difference / 2.f, sensitivity);
        moveStart(-difference / 2.f, sensitivity);

        break;
    }
    case Drag::SAMPLE_FULL:
    {
        int change = event.getOffsetFromDragStart().getX() - lastDragOffset;
        moveBoth(float(change), sensitivity);

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

void SampleNavigator::scrollView(const juce::MouseWheelDetails& wheel, int sampleCenter, bool centerZoomOut)
{
    if (!sample)
        return;

    float changeY = -wheel.deltaY * Feel::MOUSE_SENSITIVITY;
    float changeX = wheel.deltaX * Feel::MOUSE_SENSITIVITY;

    // MacOS sensitivity is a little different
    if (wheel.isSmooth && ((juce::SystemStats::getOperatingSystemType() & juce::SystemStats::MacOSX) == 0))
        changeY *= 0.25f;

    bool trackHorizontal = std::abs(wheel.deltaX) > std::abs(wheel.deltaY);
    bool modifier = juce::ModifierKeys::currentModifiers.isAnyModifierKeyDown();

    if (modifier || trackHorizontal)
    {
        moveBoth(trackHorizontal ? changeX : -changeY, getDragSensitivity(false, !modifier));
    }
    else if (!modifier)
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

    int viewStart = isLooping->get() && loopHasStart->get() && !isWaveformMode() ? state.loopStart.load() : state.sampleStart.load();
    int viewEnd = isLooping->get() && loopHasEnd->get() && !isWaveformMode() ? state.loopEnd.load() : state.sampleEnd.load();

    // Keep the view size larger than the minimum
    if (viewEnd - viewStart + 1 < Feel::MINIMUM_VIEW)
    {
        viewEnd = juce::jmin<int>(sample->getNumSamples() - 1, viewStart + Feel::MINIMUM_VIEW - 1);
        viewStart = juce::jmax<int>(0, viewEnd - Feel::MINIMUM_VIEW + 1);
    }

    state.viewStart = viewStart;
    state.viewEnd = viewEnd;

    updatePinnedPositions();

    repaint();
}

void SampleNavigator::moveStart(float change, float sensitivity)
{
    int oldViewStart = state.viewStart;

    auto viewStart = juce::jmax<int>(juce::jmin<int>(int(std::round(state.viewStart + change * sensitivity)), state.viewEnd - Feel::MINIMUM_VIEW), 0);
    state.viewStart = viewStart;

    if (state.pinView)
        moveBoundsToPinnedPositions(viewStart < oldViewStart);
}

void SampleNavigator::moveEnd(float change, float sensitivity)
{
    int oldViewEnd = state.viewEnd;

    auto viewEnd = juce::jmin<int>(juce::jmax<int>(int(std::round(state.viewEnd + change * sensitivity)), state.viewStart + Feel::MINIMUM_VIEW), sample->getNumSamples() - 1);
    state.viewEnd = viewEnd;

    if (state.pinView)
        moveBoundsToPinnedPositions(viewEnd < oldViewEnd);
}

void SampleNavigator::moveBoth(float change, float sensitivity)
{
    int difference = juce::jlimit<int>(-state.viewStart, sample->getNumSamples() - 1 - state.viewEnd, int(change * sensitivity));

    state.viewStart = state.viewStart + difference;
    state.viewEnd = state.viewEnd + difference;

    if (state.pinView)
        moveBoundsToPinnedPositions(difference < 0);
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
    return CompPart<Drag>::getClosestInRange(targets, x, y, Feel::DRAGGABLE_SNAP);
}

float SampleNavigator::sampleToPosition(int sampleIndex) const
{
    float boundsWidth = getWidth() * Layout::navigatorBoundsWidth;
    return juce::jmap<float>(float(sampleIndex), 0.f, float(sample->getNumSamples() - 1), 0.f, float(painter.getWidth())) + boundsWidth;
}

int SampleNavigator::positionToSample(float position) const
{
    return int(juce::jmap<float>(position - getWidth() * Layout::navigatorBoundsWidth, 0.f, float(painter.getWidth()), 0.f, float(sample->getNumSamples())));
}

float SampleNavigator::getDragSensitivity(bool checkSecondary, bool useSecondary) const
{
    int viewSize = state.viewEnd - state.viewStart + 1;
    if ((checkSecondary && juce::ModifierKeys::currentModifiers.isAnyModifierKeyDown()) || (!checkSecondary && useSecondary))
        return float(sample->getNumSamples()) / getWidth();
    else
        return -std::log(float(viewSize) / sample->getNumSamples() / juce::MathConstants<float>::euler) * viewSize / getWidth();
}

void SampleNavigator::loopHasStartUpdate(bool newValue)
{
    repaint();
    
    if (!bool(newValue))
        return;

    if (state.viewStart > state.loopStart && (state.loopStart == 0 || state.pinView))
        state.loopStart = state.viewStart.load();
}

void SampleNavigator::loopHasEndUpdate(bool newValue)
{
    repaint();

    if (!bool(newValue))
        return;

    if (state.viewEnd < state.loopEnd && (state.loopEnd == sample->getNumSamples() - 1 || state.pinView))
        state.loopEnd = state.viewEnd.load();
}

void SampleNavigator::updatePinnedPositions()
{
    int viewStart = state.viewStart;
    int viewEnd = state.viewEnd;

    pinnedSampleStart = juce::jlimit<float>(0.f, 1.f, float(state.sampleStart - viewStart) / (viewEnd - viewStart));
    pinnedSampleEnd = juce::jlimit<float>(0.f, 1.f, float(state.sampleEnd - viewStart) / (viewEnd - viewStart));
    pinnedLoopStart = juce::jlimit<float>(0.f, 1.f, float(state.loopStart - viewStart) / (viewEnd - viewStart));
    pinnedLoopEnd = juce::jlimit<float>(0.f, 1.f, float(state.loopEnd - viewStart) / (viewEnd - viewStart));
}

void SampleNavigator::moveBoundsToPinnedPositions(bool startToEnd)
{
    // To maintain constraints, we change the bounds in order depending on the direction of the change
    navigatorUpdate = true;
    if (startToEnd)
    {
        state.loopStart = juce::jmax(int(std::round(pinnedLoopStart * (state.viewEnd - state.viewStart))) + state.viewStart, 0);
        state.sampleStart = int(std::round(pinnedSampleStart * (state.viewEnd - state.viewStart))) + state.viewStart;
        state.sampleEnd = int(std::round(pinnedSampleEnd * (state.viewEnd - state.viewStart))) + state.viewStart;
        state.loopEnd = juce::jmin(int(std::round(pinnedLoopEnd * (state.viewEnd - state.viewStart))) + state.viewStart, sample->getNumSamples() - 1);
    }
    else
    {
        state.loopEnd = juce::jmin(int(std::round(pinnedLoopEnd * (state.viewEnd - state.viewStart))) + state.viewStart, sample->getNumSamples() - 1);
        state.sampleEnd = int(std::round(pinnedSampleEnd * (state.viewEnd - state.viewStart))) + state.viewStart;
        state.sampleStart = int(std::round(pinnedSampleStart * (state.viewEnd - state.viewStart))) + state.viewStart;
        state.loopStart = juce::jmax(int(std::round(pinnedLoopStart * (state.viewEnd - state.viewStart))) + state.viewStart, 0);
    }
    navigatorUpdate = false;
}

bool SampleNavigator::isWaveformMode() const
{
    return CustomSamplerVoice::isWavetableMode(sampleRate, state.sampleStart, state.sampleEnd);
}

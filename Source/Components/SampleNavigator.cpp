/*
  ==============================================================================

    SampleNavigator.cpp
    Created: 19 Sep 2023 4:41:12pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>

#include "SampleNavigator.h"

SampleNavigatorOverlay::SampleNavigatorOverlay(APVTS& apvts, juce::Array<CustomSamplerVoice*>& synthVoices) : synthVoices(synthVoices)
{
    viewStart = apvts.state.getPropertyAsValue(PluginParameters::UI_VIEW_START, apvts.undoManager);
    viewEnd = apvts.state.getPropertyAsValue(PluginParameters::UI_VIEW_END, apvts.undoManager);
    sampleStart = apvts.state.getPropertyAsValue(PluginParameters::SAMPLE_START, apvts.undoManager);
    sampleEnd = apvts.state.getPropertyAsValue(PluginParameters::SAMPLE_END, apvts.undoManager);
    loopStart = apvts.state.getPropertyAsValue(PluginParameters::LOOP_START, apvts.undoManager);
    loopEnd = apvts.state.getPropertyAsValue(PluginParameters::LOOP_END, apvts.undoManager);
    isLooping = apvts.getParameterAsValue(PluginParameters::IS_LOOPING);
    loopHasStart = apvts.state.getPropertyAsValue(PluginParameters::LOOPING_HAS_START, apvts.undoManager);
    loopHasEnd = apvts.state.getPropertyAsValue(PluginParameters::LOOPING_HAS_END, apvts.undoManager);
    viewStart.addListener(this);
    viewEnd.addListener(this);
}

SampleNavigatorOverlay::~SampleNavigatorOverlay()
{
    viewStart.removeListener(this);
    viewEnd.removeListener(this);
}

void SampleNavigatorOverlay::paint(juce::Graphics& g)
{
    using namespace juce;
    if (sample)
    {
        // paints the voice positions
        Path path{};
        for (auto i = 0; i < synthVoices.size(); i++)
        {
            if (synthVoices[i]->getCurrentlyPlayingSound())
            {
                auto location = synthVoices[i]->getEffectiveLocation();
                if (location > 0)
                {
                    auto pos = jmap<float>(float(location), 0.f, float(sample->getNumSamples()), 0.f, float(painterBounds.getWidth()));
                    path.addLineSegment(Line<float>(pos, 0.f, pos, float(getHeight())), 1.f);
                }
            }
        }
        g.setColour(lnf.VOICE_POSITION_COLOR);
        g.strokePath(path, PathStrokeType(1.f));
        // paints the start and stop
        float startPos = sampleToPosition(viewStart.getValue());
        float stopPos = sampleToPosition(viewEnd.getValue());
        g.setColour(lnf.SAMPLE_BOUNDS_COLOR.withAlpha(0.2f));
        g.fillRect(startPos + painterPadding, 0.f, stopPos - startPos + 1.f, float(getHeight()));

        g.setColour(dragging && draggingTarget == Drag::SAMPLE_START ? lnf.SAMPLE_BOUNDS_SELECTED_COLOR : disabled(lnf.SAMPLE_BOUNDS_COLOR));
        g.fillPath(startSamplePath, juce::AffineTransform::translation(startPos + painterPadding, 0.f));
        g.setColour(dragging && draggingTarget == Drag::SAMPLE_END ? lnf.SAMPLE_BOUNDS_SELECTED_COLOR : disabled(lnf.SAMPLE_BOUNDS_COLOR));
        g.fillPath(stopSamplePath, juce::AffineTransform::translation(stopPos + painterPadding + 1, 0.f));
    }
}

void SampleNavigatorOverlay::resized()
{
    startSamplePath.clear();
    startSamplePath.addLineSegment(juce::Line<int>(0, 0, 0, getHeight()).toFloat(), float(lnf.NAVIGATOR_BOUNDS_WIDTH));
    startSamplePath.addLineSegment(juce::Line<int>(0, 0, 4, 0).toFloat(), 2.f);
    startSamplePath.addLineSegment(juce::Line<int>(0, getHeight(), 4, getHeight()).toFloat(), 2.f);

    stopSamplePath.clear();
    stopSamplePath.addLineSegment(juce::Line<int>(0, 0, 0, getHeight()).toFloat(), float(lnf.NAVIGATOR_BOUNDS_WIDTH));
    stopSamplePath.addLineSegment(juce::Line<int>(-4, 0, 0, 0).toFloat(), 2.f);
    stopSamplePath.addLineSegment(juce::Line<int>(-4, getHeight(), 0, getHeight()).toFloat(), 2.f);
}

void SampleNavigatorOverlay::mouseMove(const MouseEvent& event)
{
    if (!sample || !isEnabled())
    {
        setMouseCursor(MouseCursor::NormalCursor);
        return;
    }
    Drag currentTarget = getDraggingTarget(event.getMouseDownX(), event.getMouseDownY());
    switch (currentTarget)
    {
    case Drag::SAMPLE_START:
    case Drag::SAMPLE_END:
        setMouseCursor(MouseCursor::LeftRightResizeCursor);
        break;
    case Drag::SAMPLE_FULL:
        setMouseCursor(MouseCursor::DraggingHandCursor);
        break;
    default:
        setMouseCursor(MouseCursor::NormalCursor);
    }
}

void SampleNavigatorOverlay::mouseDown(const juce::MouseEvent& event)
{
    if (sample || !isEnabled())
    {
        dragOriginStartSample = viewStart.getValue();
        draggingTarget = getDraggingTarget(event.getMouseDownX(), event.getMouseDownY());
        dragging = draggingTarget != Drag::NONE;
        repaint();
    }
}

void SampleNavigatorOverlay::mouseUp(const juce::MouseEvent&)
{
    if (sample)
    {
        dragging = false;
        repaint();
    }
}

void SampleNavigatorOverlay::mouseDrag(const juce::MouseEvent& event)
{
    if (sample && dragging && isEnabled())
    {
        auto newSample = positionToSample(float(event.getMouseDownX() + event.getOffsetFromDragStart().getX() - painterPadding));
        auto startPos = sampleToPosition(viewStart.getValue());
        auto endPos = sampleToPosition(viewEnd.getValue());
        // the goal is to keep the positions within view bounds
        switch (draggingTarget)
        {
        case Drag::SAMPLE_START:
        {
            auto newValue = juce::jlimit<int>(0, positionToSample(endPos - 20), newSample);
            auto effectiveMin = newValue;
            if (bool(isLooping.getValue()) && bool(loopHasStart.getValue()) && (int(loopStart.getValue()) < effectiveMin || viewStart == loopStart.getValue()))
            {
                loopStart = effectiveMin++;
            }
            if (int(sampleStart.getValue()) < effectiveMin || viewStart == sampleStart.getValue())
            {
                sampleStart = effectiveMin++;
            }
            if (int(sampleEnd.getValue()) < effectiveMin)
            {
                sampleEnd = effectiveMin++;
            }
            if (bool(isLooping.getValue()) && bool(loopHasEnd.getValue()) && int(loopEnd.getValue()) < effectiveMin)
            {
                loopEnd = effectiveMin++;
            }
            viewStart = newValue;
            break;
        }
        case Drag::SAMPLE_END:
        {
            auto newValue = juce::jlimit<int>(positionToSample(startPos + 20), sample->getNumSamples() - 1, newSample);
            auto effectiveMax = newValue;
            if (bool(isLooping.getValue()) && bool(loopHasEnd.getValue()) && (int(loopEnd.getValue()) > effectiveMax || viewEnd == loopEnd.getValue()))
            {
                loopEnd = effectiveMax--;
            }
            if (int(sampleEnd.getValue()) > effectiveMax || viewEnd == sampleEnd.getValue())
            {
                sampleEnd = effectiveMax--;
            }
            if (int(sampleStart.getValue()) > effectiveMax)
            {
                sampleStart = effectiveMax--;
            }
            if (bool(isLooping.getValue()) && bool(loopHasStart.getValue()) && int(loopStart.getValue()) > effectiveMax)
            {
                loopStart = effectiveMax--;
            }
            viewEnd = newValue;
            break;
        }
        case Drag::SAMPLE_FULL:
            auto originStart = dragOriginStartSample;
            auto originStop = dragOriginStartSample + int(viewEnd.getValue()) - int(viewStart.getValue());
            auto sampleChange = juce::jlimit<int>(-originStart, sample->getNumSamples() - 1 - originStop,
                positionToSample(float(event.getOffsetFromDragStart().getX())));
            sampleStart = dragOriginStartSample + int(sampleStart.getValue()) - int(viewStart.getValue()) + sampleChange;
            sampleEnd = dragOriginStartSample + int(sampleEnd.getValue()) - int(viewStart.getValue()) + sampleChange;
            if (isLooping.getValue() && loopHasStart.getValue())
            {
                loopStart = dragOriginStartSample + int(loopStart.getValue()) - int(viewStart.getValue()) + sampleChange;
            }
            if (isLooping.getValue() && loopHasEnd.getValue())
            {
                loopEnd = dragOriginStartSample + int(loopEnd.getValue()) - int(viewStart.getValue()) + sampleChange;
            }
            viewStart = originStart + sampleChange;
            viewEnd = originStop + sampleChange;
            break;
        }
    }
}

Drag SampleNavigatorOverlay::getDraggingTarget(int x, int)
{
    Drag target = Drag::NONE;
    auto startPos = sampleToPosition(viewStart.getValue()) + painterPadding;
    auto stopPos = sampleToPosition(viewEnd.getValue()) + painterPadding;
    auto startDif = std::abs(x - startPos);
    auto stopDif = std::abs(x - stopPos);
    if (startDif < lnf.DRAGGABLE_SNAP && stopDif < lnf.DRAGGABLE_SNAP)
    {
        if (startDif < stopDif)
        {
            target = Drag::SAMPLE_START;
        }
        else
        {
            target = Drag::SAMPLE_END;
        }
    }
    else if (startDif < lnf.DRAGGABLE_SNAP)
    {
        target = Drag::SAMPLE_START;
    }
    else if (stopDif < lnf.DRAGGABLE_SNAP)
    {
        target = Drag::SAMPLE_END;
    }
    else if (startPos < x && x < stopPos)
    {
        target = Drag::SAMPLE_FULL;
    }
    return target;
}

void SampleNavigatorOverlay::valueChanged(juce::Value&)
{
    repaint();
}

float SampleNavigatorOverlay::sampleToPosition(int sampleIndex)
{
    return juce::jmap<float>(float(sampleIndex), 0.f, float(sample->getNumSamples()), 0.f, float(painterBounds.getWidth()));
}

int SampleNavigatorOverlay::positionToSample(float position)
{
    return int(juce::jmap<float>(position, 0.f, float(painterBounds.getWidth()), 0.f, float(sample->getNumSamples())));
}

void SampleNavigatorOverlay::setSample(juce::AudioBuffer<float>& sampleBuffer, bool resetUI)
{
    sample = &sampleBuffer;
    if (resetUI)
    {
        viewStart = 0;
        viewEnd = sampleBuffer.getNumSamples() - 1;
    }
    repaint();
}

void SampleNavigatorOverlay::setPainterBounds(juce::Rectangle<int> bounds)
{
    painterBounds = bounds;
    painterPadding = (getWidth() - bounds.getWidth()) / 2;
}

//==============================================================================
SampleNavigator::SampleNavigator(APVTS& apvts, juce::Array<CustomSamplerVoice*>& synthVoices) : apvts(apvts), overlay(apvts, synthVoices)
{
    addAndMakeVisible(&painter);
    overlay.toFront(true);
    addAndMakeVisible(&overlay);
}

SampleNavigator::~SampleNavigator()
{
}

void SampleNavigator::paint (juce::Graphics&)
{
}

void SampleNavigator::resized()
{
    auto bounds = getLocalBounds();
    overlay.setBounds(bounds);
    bounds.removeFromLeft(lnf.NAVIGATOR_BOUNDS_WIDTH);
    bounds.removeFromRight(lnf.NAVIGATOR_BOUNDS_WIDTH);
    painter.setBounds(bounds);
    overlay.setPainterBounds(bounds);
}

void SampleNavigator::enablementChanged()
{
    overlay.setEnabled(isEnabled());
    overlay.repaint();
    painter.setEnabled(isEnabled());
    painter.repaint();
}

void SampleNavigator::repaintUI()
{
    overlay.repaint();
}

void SampleNavigator::setSample(juce::AudioBuffer<float>& sampleBuffer, bool resetUI)
{
    painter.setSample(sampleBuffer);
    overlay.setSample(sampleBuffer, resetUI);
}

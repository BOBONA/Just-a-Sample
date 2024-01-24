/*
  ==============================================================================

    FilterResponse.cpp
    Created: 2 Jan 2024 10:40:21pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>

#include "FilterResponse.h"

FilterResponse::FilterResponse(juce::AudioProcessorValueTreeState& apvts, int sampleRate) : apvts(apvts),
lowFreqAttachment(*apvts.getParameter(PluginParameters::EQ_LOW_FREQ), [&](float newValue) { lowFreq = newValue; shouldRepaint = true; }, apvts.undoManager),
highFreqAttachment(*apvts.getParameter(PluginParameters::EQ_HIGH_FREQ), [&](float newValue) { highFreq = newValue; shouldRepaint = true; }, apvts.undoManager)
{
    setBufferedToImage(true);
    eq.initialize(1, sampleRate);
    
    apvts.addParameterListener(PluginParameters::EQ_LOW_GAIN, this);
    apvts.addParameterListener(PluginParameters::EQ_MID_GAIN, this);
    apvts.addParameterListener(PluginParameters::EQ_HIGH_GAIN, this);

    lowFreq = apvts.getParameterAsValue(PluginParameters::EQ_LOW_FREQ).getValue();
    highFreq = apvts.getParameterAsValue(PluginParameters::EQ_HIGH_FREQ).getValue();
    lowGain = apvts.getParameterAsValue(PluginParameters::EQ_LOW_GAIN).getValue();
    midGain = apvts.getParameterAsValue(PluginParameters::EQ_MID_GAIN).getValue();
    highGain = apvts.getParameterAsValue(PluginParameters::EQ_HIGH_GAIN).getValue();

    startTimerHz(60);
}

FilterResponse::~FilterResponse()
{
    apvts.removeParameterListener(PluginParameters::EQ_LOW_GAIN, this);
    apvts.removeParameterListener(PluginParameters::EQ_MID_GAIN, this);
    apvts.removeParameterListener(PluginParameters::EQ_HIGH_GAIN, this);
}

void FilterResponse::setSampleRate(int sampleRate)
{
    eq.initialize(1, sampleRate);
}

void FilterResponse::paint(Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    Array<double> frequencies;
    for (int i = 0; i < bounds.getWidth(); i++)
    {
        frequencies.add(posToFreq(bounds, float(i)));
    }
    eq.updateParams(lowFreq, highFreq, lowGain, midGain, highGain);

    Array<double> magnitudes = eq.getMagnitudeForFrequencyArray(frequencies);

    Path path;
    path.startNewSubPath(bounds.getBottomLeft());
    for (int i = 0; i < bounds.getWidth(); i++)
    {
        float normalizedDecibel = jmap<float>(float(Decibels::gainToDecibels(magnitudes[i])), -13.f, 13.f, bounds.getHeight(), 0.f);
        path.lineTo(bounds.getX() + i, normalizedDecibel);
    }
    g.setColour(disabled(lnf.WAVEFORM_COLOR));
    g.strokePath(path, PathStrokeType{ 2, PathStrokeType::curved });

    int lowLoc = int(freqToPos(bounds, lowFreq));
    float curveLowPos = jmap<float>(float(Decibels::gainToDecibels(magnitudes[lowLoc])), -13.f, 13.f, bounds.getHeight(), 0.f);
    g.setColour(disabled((dragging && draggingTarget == FilterResponseParts::LOW_FREQ) ? lnf.SAMPLE_BOUNDS_SELECTED_COLOR : lnf.SAMPLE_BOUNDS_COLOR));
    g.drawVerticalLine(lowLoc, 1.f, curveLowPos - 4.f);
    g.drawVerticalLine(lowLoc, curveLowPos + 4.f, getHeight() - 1.f);

    int highLoc = int(freqToPos(bounds, highFreq));
    float curveHighPos = jmap<float>(float(Decibels::gainToDecibels(magnitudes[highLoc])), -13.f, 13.f, bounds.getHeight(), 0.f);
    g.setColour(disabled((dragging && draggingTarget == FilterResponseParts::HIGH_FREQ) ? lnf.SAMPLE_BOUNDS_SELECTED_COLOR : lnf.SAMPLE_BOUNDS_COLOR));
    g.drawVerticalLine(highLoc, 1.f, curveHighPos - 4.f);
    g.drawVerticalLine(highLoc, curveHighPos + 4.f, getHeight() - 1.f);
}

void FilterResponse::resized()
{
}

void FilterResponse::enablementChanged()
{
    repaint();
}

void FilterResponse::parameterChanged(const String& parameterID, float newValue)
{
    if (parameterID == PluginParameters::EQ_LOW_GAIN)
    {
        lowGain = newValue;
    }
    else if (parameterID == PluginParameters::EQ_MID_GAIN)
    {
        midGain = newValue;
    }
    else if (parameterID == PluginParameters::EQ_HIGH_GAIN)
    {
        highGain = newValue;
    }
    shouldRepaint = true;
}

void FilterResponse::timerCallback()
{
    if (shouldRepaint)
    {
        shouldRepaint = false;
        repaint();
    }
}

void FilterResponse::mouseMove(const MouseEvent& event)
{
    if (!isEnabled())
    {
        setMouseCursor(MouseCursor::NormalCursor);
        return;
    }
    FilterResponseParts part = getClosestPartInRange(event.x, event.y);
    switch (part)
    {
    case FilterResponseParts::LOW_FREQ:
    case FilterResponseParts::HIGH_FREQ:
        setMouseCursor(MouseCursor::LeftRightResizeCursor);
        break;
    default:
        setMouseCursor(MouseCursor::NormalCursor);
        break;
    }
}

void FilterResponse::mouseDown(const juce::MouseEvent& event)
{
    if (!isEnabled() || dragging)
        return;
    FilterResponseParts closest = getClosestPartInRange(event.getMouseDownX(), event.getMouseDownY());
    if (closest == FilterResponseParts::NONE)
        return;
    else if (closest == FilterResponseParts::LOW_FREQ)
        lowFreqAttachment.beginGesture();
    else if (closest == FilterResponseParts::HIGH_FREQ)
        highFreqAttachment.beginGesture();
    dragging = true;
    draggingTarget = closest;
    repaint();
}

void FilterResponse::mouseUp(const juce::MouseEvent&)
{
    if (!dragging)
        return;
    if (draggingTarget == FilterResponseParts::LOW_FREQ)
        lowFreqAttachment.endGesture();
    else if (draggingTarget == FilterResponseParts::HIGH_FREQ)
        highFreqAttachment.endGesture();
    dragging = false;
    repaint();
}

void FilterResponse::mouseDrag(const juce::MouseEvent& event)
{
    if (!dragging || !isEnabled())
        return;
    auto bounds = getLocalBounds().toFloat();
    auto newFreq = posToFreq(bounds, float(event.getMouseDownX() + event.getOffsetFromDragStart().getX()));
    auto freqStartBound = draggingTarget == FilterResponseParts::LOW_FREQ ? PluginParameters::EQ_LOW_FREQ_RANGE.getStart() : PluginParameters::EQ_HIGH_FREQ_RANGE.getStart();
    auto freqEndBound = draggingTarget == FilterResponseParts::LOW_FREQ ? PluginParameters::EQ_LOW_FREQ_RANGE.getEnd() : PluginParameters::EQ_HIGH_FREQ_RANGE.getEnd();
    newFreq = jlimit<float>(freqStartBound, freqEndBound, newFreq);
    switch (draggingTarget)
    {
    case FilterResponseParts::LOW_FREQ:
        lowFreqAttachment.setValueAsPartOfGesture(newFreq);
        break;
    case FilterResponseParts::HIGH_FREQ:
        highFreqAttachment.setValueAsPartOfGesture(newFreq);
        break;
    }
}

FilterResponseParts FilterResponse::getClosestPartInRange(int x, int y) const
{
    auto bounds = getLocalBounds().toFloat();
    juce::Array<CompPart<FilterResponseParts>> targets = {
        CompPart {FilterResponseParts::LOW_FREQ, Rectangle<float>(freqToPos(bounds, lowFreq), bounds.getY(), 0, bounds.getHeight()), 1},
        CompPart {FilterResponseParts::HIGH_FREQ, Rectangle<float>(freqToPos(bounds, highFreq), bounds.getY(), 0, bounds.getHeight()), 1},
    };
    return CompPart<FilterResponseParts>::getClosestInRange(targets, x, y, lnf.DRAGGABLE_SNAP);
}

float FilterResponse::freqToPos(Rectangle<float> bounds, float freq) const
{
    return (bounds.getWidth() - 1) * logf(freq / startFreq) / logf(float(endFreq) / startFreq);
}

float FilterResponse::posToFreq(Rectangle<float> bounds, float pos) const
{
    return startFreq * powf(float(endFreq) / startFreq, float(pos) / (bounds.getWidth() - 1.f));
}

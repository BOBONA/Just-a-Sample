/*
  ==============================================================================

    FilterResponse.cpp
    Created: 2 Jan 2024 10:40:21pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>

#include "FilterResponse.h"

FilterResponse::FilterResponse(APVTS& apvts, int sampleRate) : apvts(apvts),
    lowFreqAttachment(*apvts.getParameter(PluginParameters::EQ_LOW_FREQ), [&](float newValue) { lowFreq = newValue; repaint(); }, apvts.undoManager),
    highFreqAttachment(*apvts.getParameter(PluginParameters::EQ_HIGH_FREQ), [&](float newValue) { highFreq = newValue; repaint(); }, apvts.undoManager),
    lowGainAttachment(*apvts.getParameter(PluginParameters::EQ_LOW_GAIN), [&](float newValue) { lowGain = newValue; repaint(); }, apvts.undoManager),
    midGainAttachment(*apvts.getParameter(PluginParameters::EQ_MID_GAIN), [&](float newValue) { midGain = newValue; repaint(); }, apvts.undoManager),
    highGainAttachment(*apvts.getParameter(PluginParameters::EQ_HIGH_GAIN), [&](float newValue) { highGain = newValue; repaint(); }, apvts.undoManager)
{
    eq.initialize(1, sampleRate);

    lowFreqAttachment.sendInitialUpdate();
    highFreqAttachment.sendInitialUpdate();
    lowGainAttachment.sendInitialUpdate();
    midGainAttachment.sendInitialUpdate();
    highGainAttachment.sendInitialUpdate();
}

void FilterResponse::paint(juce::Graphics& g)
{
    using namespace juce;

    auto bounds = getLocalBounds().toFloat();

    Array<double> frequencies;
    for (int i = 0; i < bounds.getWidth(); i++)
        frequencies.add(posToFreq(bounds, float(i)));
    eq.updateParams(lowFreq, highFreq, lowGain, midGain, highGain);

    Array<double> magnitudes = eq.getMagnitudeForFrequencyArray(frequencies);
    auto gainRange = PluginParameters::EQ_GAIN_RANGE;

    Path path;
    path.startNewSubPath(bounds.getBottomLeft());
    for (int i = 0; i < bounds.getWidth(); i++)
    {
        float normalizedDecibel = jmap<float>(float(Decibels::gainToDecibels(magnitudes[i])), gainRange.start - 1.f, gainRange.end + 1.f, bounds.getHeight(), 0.f);
        if (i == 0)
            path.startNewSubPath(0, normalizedDecibel);
        else
            path.lineTo(bounds.getX() + i, normalizedDecibel);
    }

    auto borderWidth = getWidth() * Layout::fxDisplayStrokeWidth * 1.5f;
    g.setColour(Colors::DARK);
    g.strokePath(path, PathStrokeType{ borderWidth, PathStrokeType::curved });

    auto boundsPad = getWidth() * Layout::fxDisplayStrokeWidth * 6.f;

    int lowLoc = int(freqToPos(bounds, lowFreq));
    float curveLowPos = jmap<float>(float(Decibels::gainToDecibels(magnitudes[lowLoc])), gainRange.start, gainRange.end, bounds.getHeight() * 0.98f, 0.02f);
    g.setColour(dragging && draggingTarget == LOW_FREQ ? Colors::SLATE.withAlpha(0.5f) : Colors::SLATE);
    if (float height = curveLowPos - boundsPad; height > 0.f)
        g.fillRect(lowLoc - borderWidth / 2.f, 0.f, borderWidth, height);
    if (float height = getHeight() - curveLowPos - boundsPad; height > 0.f)
        g.fillRect(lowLoc - borderWidth / 2.f, curveLowPos + boundsPad, borderWidth, height);

    int highLoc = int(freqToPos(bounds, highFreq));
    float curveHighPos = jmap<float>(float(Decibels::gainToDecibels(magnitudes[highLoc])), gainRange.start, gainRange.end, bounds.getHeight() * 0.98f, 0.02f);
    g.setColour(dragging && draggingTarget == HIGH_FREQ ? Colors::SLATE.withAlpha(0.5f) : Colors::SLATE);
    if (float height = curveHighPos - boundsPad; height > 0.f)
        g.fillRect(highLoc - borderWidth / 2.f, 0.f, borderWidth, height);
    if (float height = getHeight() - curveHighPos - boundsPad; height > 0.f)
        g.fillRect(highLoc - borderWidth / 2.f, curveHighPos + boundsPad, borderWidth, height);

    if (!isEnabled())
        g.fillAll(Colors::BACKGROUND.withAlpha(0.5f));
}

void FilterResponse::enablementChanged()
{
    repaint();
}

void FilterResponse::mouseMove(const juce::MouseEvent& event)
{
    if (!isEnabled())
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    }

    FilterResponseParts part = getClosestPartInRange(event.x, event.y);
    switch (part)
    {
    case LOW_FREQ:
    case HIGH_FREQ:
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        break;
    case NONE:
        setMouseCursor(juce::MouseCursor::NormalCursor);
        break;
    }
}

void FilterResponse::mouseDown(const juce::MouseEvent& event)
{
    if (!isEnabled() || dragging)
        return;

    FilterResponseParts closest = getClosestPartInRange(event.getMouseDownX(), event.getMouseDownY());
    if (closest == NONE)
        return;

    if (closest == LOW_FREQ)
        lowFreqAttachment.beginGesture();
    else if (closest == HIGH_FREQ)
        highFreqAttachment.beginGesture();
    dragging = true;
    draggingTarget = closest;

    repaint();
}

void FilterResponse::mouseUp(const juce::MouseEvent&)
{
    if (!dragging)
        return;
    if (draggingTarget == LOW_FREQ)
        lowFreqAttachment.endGesture();
    else if (draggingTarget == HIGH_FREQ)
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
    auto freqStartBound = draggingTarget == LOW_FREQ ? PluginParameters::EQ_LOW_FREQ_RANGE.getStart() : PluginParameters::EQ_HIGH_FREQ_RANGE.getStart();
    auto freqEndBound = draggingTarget == LOW_FREQ ? PluginParameters::EQ_LOW_FREQ_RANGE.getEnd() : PluginParameters::EQ_HIGH_FREQ_RANGE.getEnd();
    newFreq = juce::jlimit<float>(freqStartBound, freqEndBound, newFreq);

    switch (draggingTarget)
    {
    case LOW_FREQ:
        lowFreqAttachment.setValueAsPartOfGesture(newFreq);
        break;
    case HIGH_FREQ:
        highFreqAttachment.setValueAsPartOfGesture(newFreq);
        break;
    }
}

void FilterResponse::mouseDoubleClick(const juce::MouseEvent& event)
{
    auto part = getClosestPartInRange(event.getMouseDownX(), event.getMouseDownY());

    switch (part)
    {
    case LOW_FREQ:
        lowFreqAttachment.setValueAsCompleteGesture(PluginParameters::EQ_LOW_FREQ_DEFAULT);
        break;
    case HIGH_FREQ:
        highFreqAttachment.setValueAsCompleteGesture(PluginParameters::EQ_HIGH_FREQ_DEFAULT);
        break;
    }
}

FilterResponseParts FilterResponse::getClosestPartInRange(int x, int y) const
{
    auto bounds = getLocalBounds().toFloat();
    juce::Array targets = {
        CompPart {LOW_FREQ, juce::Rectangle<float>(freqToPos(bounds, lowFreq), bounds.getY(), 0, bounds.getHeight()), 1},
        CompPart {HIGH_FREQ, juce::Rectangle<float>(freqToPos(bounds, highFreq), bounds.getY(), 0, bounds.getHeight()), 1},
    };
    return CompPart<FilterResponseParts>::getClosestInRange(targets, x, y, Feel::DRAGGABLE_SNAP);
}

float FilterResponse::freqToPos(juce::Rectangle<float> bounds, float freq) const
{
    return (bounds.getWidth() - 1) * logf(freq / startFreq) / logf(float(endFreq) / startFreq);
}

float FilterResponse::posToFreq(juce::Rectangle<float> bounds, float pos) const
{
    return startFreq * powf(float(endFreq) / startFreq, float(pos) / (bounds.getWidth() - 1.f));
}

juce::String FilterResponse::getCustomHelpText()
{
    auto part = dragging ? draggingTarget : getClosestPartInRange(getMouseXYRelative().getX(), getMouseXYRelative().getY());
    switch (part)
    {
    case LOW_FREQ: return PluginParameters::EQ_LOW_FREQ + ": " + juce::String(int(lowFreq)) + " hz";
    case HIGH_FREQ: return PluginParameters::EQ_HIGH_FREQ + ": " + juce::String(int(highFreq)) + " hz";
    default: return "Drag to adjust filter cutoffs";
    }
}

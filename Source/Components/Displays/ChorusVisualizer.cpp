/*
  ==============================================================================

    ChorusVisualizer.cpp
    Created: 30 Jan 2024 6:12:49pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>
#include "ChorusVisualizer.h"

ChorusVisualizer::ChorusVisualizer(APVTS& apvts, int sampleRate) :
    apvts(apvts), sampleRate(sampleRate),
    rateAttachment(*apvts.getParameter(PluginParameters::CHORUS_RATE), [this](float newValue) { rate = newValue; repaint(); }, apvts.undoManager),
    depthAttachment(*apvts.getParameter(PluginParameters::CHORUS_DEPTH), [this](float newValue) { depth = newValue; repaint(); }, apvts.undoManager),
    centerDelayAttachment(*apvts.getParameter(PluginParameters::CHORUS_CENTER_DELAY), [this](float newValue) { centerDelay = newValue; repaint(); }, apvts.undoManager)
{
    rateAttachment.sendInitialUpdate();
    depthAttachment.sendInitialUpdate();
    centerDelayAttachment.sendInitialUpdate();
}

void ChorusVisualizer::paint(juce::Graphics& g)
{
    juce::Path path;
    constexpr int pointsPerPixel = 10;

    for (int i = 0; i < getWidth() * pointsPerPixel; i++)
    {
        float pos = float(i) / pointsPerPixel;

        float lfo = std::sinf(WINDOW_LENGTH * rate * pos / getWidth() * 2 * juce::MathConstants<float>::pi);
        float skewedDepth = logf(depth) + b;
        float delay = centerDelay * 0.98f + skewedDepth * lfo * multiplier;
        float loc = juce::jmap<float>(delay, -oscRange, oscRange, float(getHeight()), 0.f);

        if (i == 0)
            path.startNewSubPath(pos, loc);
        else
            path.lineTo(pos, loc);
    }

    auto strokeWidth = getWidth() * Layout::fxDisplayStrokeWidth;
    
    g.setColour(Colors::DARK.withAlpha(0.2f));
    g.drawRect(0.f, (getHeight() - strokeWidth) / 2.f, float(getWidth()), strokeWidth);
    g.setColour(Colors::DARK);
    g.strokePath(path, juce::PathStrokeType(strokeWidth));

    if (!isEnabled())
        g.fillAll(Colors::BACKGROUND.withAlpha(0.5f));
}

void ChorusVisualizer::enablementChanged()
{
    repaint();
}

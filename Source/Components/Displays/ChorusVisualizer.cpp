/*
  ==============================================================================

    ChorusVisualizer.cpp
    Created: 30 Jan 2024 6:12:49pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>
#include "ChorusVisualizer.h"

ChorusVisualizer::ChorusVisualizer(AudioProcessorValueTreeState& apvts, int sampleRate) : 
    apvts(apvts), sampleRate(sampleRate),
    rate(apvts.getParameterAsValue(PluginParameters::CHORUS_RATE).getValue()),
    depth(jlimit<float>(PluginParameters::CHORUS_DEPTH_RANGE.start, PluginParameters::CHORUS_DEPTH_RANGE.end, apvts.getParameterAsValue(PluginParameters::CHORUS_DEPTH).getValue())),
    feedback(apvts.getParameterAsValue(PluginParameters::CHORUS_FEEDBACK).getValue()),
    centerDelay(apvts.getParameterAsValue(PluginParameters::CHORUS_CENTER_DELAY).getValue()),
    mix(apvts.getParameterAsValue(PluginParameters::CHORUS_MIX).getValue())
{
    setBufferedToImage(true);
    apvts.addParameterListener(PluginParameters::CHORUS_RATE, this);
    apvts.addParameterListener(PluginParameters::CHORUS_DEPTH, this);
    apvts.addParameterListener(PluginParameters::CHORUS_FEEDBACK, this);
    apvts.addParameterListener(PluginParameters::CHORUS_CENTER_DELAY, this);
    apvts.addParameterListener(PluginParameters::CHORUS_MIX, this);

    startTimerHz(60);
}

ChorusVisualizer::~ChorusVisualizer()
{
    apvts.removeParameterListener(PluginParameters::CHORUS_RATE, this);
    apvts.removeParameterListener(PluginParameters::CHORUS_DEPTH, this);
    apvts.removeParameterListener(PluginParameters::CHORUS_FEEDBACK, this);
    apvts.removeParameterListener(PluginParameters::CHORUS_CENTER_DELAY, this);
    apvts.removeParameterListener(PluginParameters::CHORUS_MIX, this);
}

void ChorusVisualizer::paint(juce::Graphics& g)
{
    Path path;
    for (float i = 0; i < getWidth(); i++)
    {
        float lfo = std::sinf(WINDOW_LENGTH * rate * i / getWidth() * 2 * MathConstants<float>::pi);
        float skewedDepth = logf(depth) + b;
        float delay = centerDelay / 5.f + skewedDepth * lfo * multiplier;
        float loc = jmap<float>(delay, -oscRange, oscRange, float(getHeight()), 0.f);
        if (i == 0)
            path.startNewSubPath(i, loc);
        else
            path.lineTo(i, loc);
    }

    g.setColour(disabled(lnf.WAVEFORM_COLOR));
    g.strokePath(path, PathStrokeType(.5f));
    g.drawHorizontalLine(getHeight() / 2, 0, float(getWidth()));
}

void ChorusVisualizer::resized()
{
}

void ChorusVisualizer::enablementChanged()
{
    repaint();
}

void ChorusVisualizer::timerCallback()
{
    if (shouldRepaint)
    {
        repaint();
        shouldRepaint = false;
    }
}

void ChorusVisualizer::parameterChanged(const String& parameterID, float newValue)
{
    if (parameterID == PluginParameters::CHORUS_RATE)
        rate = newValue;
    else if (parameterID == PluginParameters::CHORUS_DEPTH)
        depth = newValue;
    else if (parameterID == PluginParameters::CHORUS_FEEDBACK)
        feedback = newValue;
    else if (parameterID == PluginParameters::CHORUS_CENTER_DELAY)
        centerDelay = newValue;
    else if (parameterID == PluginParameters::CHORUS_MIX)
        mix = newValue;
    shouldRepaint = true;
}
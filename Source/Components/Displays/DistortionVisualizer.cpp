/*
  ==============================================================================

    DistortionVisualizer.cpp
    Created: 23 Jan 2024 9:12:52pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>
#include "DistortionVisualizer.h"

DistortionVisualizer::DistortionVisualizer(APVTS& apvts, int sampleRate) : inputBuffer(1, WINDOW_LENGTH), apvts(apvts)
{
    distortion.initialize(1, sampleRate);
    
    apvts.addParameterListener(PluginParameters::DISTORTION_DENSITY, this);
    apvts.addParameterListener(PluginParameters::DISTORTION_HIGHPASS, this);
    apvts.addParameterListener(PluginParameters::DISTORTION_MIX, this);

    distortionDensity = *apvts.getRawParameterValue(PluginParameters::DISTORTION_DENSITY);
    distortionHighpass = *apvts.getRawParameterValue(PluginParameters::DISTORTION_HIGHPASS);
    distortionMix = *apvts.getRawParameterValue(PluginParameters::DISTORTION_MIX);

    startTimerHz(60);
}

DistortionVisualizer::~DistortionVisualizer()
{
    apvts.removeParameterListener(PluginParameters::DISTORTION_DENSITY, this);
    apvts.removeParameterListener(PluginParameters::DISTORTION_HIGHPASS, this);
    apvts.removeParameterListener(PluginParameters::DISTORTION_MIX, this);
}

void DistortionVisualizer::paint(juce::Graphics& g)
{
    // Fill the input buffer with a sine wave
    for (int i = 0; i < WINDOW_LENGTH; i++)
    {
        inputBuffer.setSample(0, i, sinf(juce::MathConstants<float>::twoPi * i * SINE_HZ / WINDOW_LENGTH) + cosf(2.5f * juce::MathConstants<float>::pi * i * SINE_HZ / WINDOW_LENGTH));
    }
    distortion.updateParams(distortionDensity, 0.f, distortionMix);
    distortion.process(inputBuffer, inputBuffer.getNumSamples());
    auto range = inputBuffer.findMinMax(0, 0, inputBuffer.getNumSamples()).getLength() / 2.f;

    // Draw the resulting waveform
    juce::Path path;
    for (int i = 0; i < getWidth(); i++)
    {
        float sample = inputBuffer.getSample(0, i * WINDOW_LENGTH / getWidth());
        float y = juce::jmap<float>(sample, -range, range, float(getHeight()), 0.f);
        if (i == 0)
            path.startNewSubPath(0, y);
        else
            path.lineTo(float(i), y);
    }
    g.setColour(disabled(lnf.WAVEFORM_COLOR));
    g.strokePath(path, juce::PathStrokeType(1.f));
}

void DistortionVisualizer::resized()
{
}

void DistortionVisualizer::timerCallback()
{
    if (shouldRepaint)
    {
        repaint();
        shouldRepaint = false;
    }
}

void DistortionVisualizer::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == PluginParameters::DISTORTION_DENSITY)
        distortionDensity = newValue;
    else if (parameterID == PluginParameters::DISTORTION_HIGHPASS)
        distortionHighpass = newValue;
    else if (parameterID == PluginParameters::DISTORTION_MIX)
        distortionMix = newValue;
    shouldRepaint = true;
}

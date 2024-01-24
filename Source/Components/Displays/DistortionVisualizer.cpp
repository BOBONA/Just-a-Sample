/*
  ==============================================================================

    DistortionVisualizer.cpp
    Created: 23 Jan 2024 9:12:52pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>
#include "DistortionVisualizer.h"

DistortionVisualizer::DistortionVisualizer(AudioProcessorValueTreeState& apvts, int sampleRate) : apvts(apvts), inputBuffer(1, WINDOW_LENGTH)
{
    distortion.initialize(1, sampleRate);
    
    apvts.addParameterListener(PluginParameters::DISTORTION_DENSITY, this);
    apvts.addParameterListener(PluginParameters::DISTORTION_HIGHPASS, this);
    apvts.addParameterListener(PluginParameters::DISTORTION_MIX, this);

    distortionDensity = apvts.getParameterAsValue(PluginParameters::DISTORTION_DENSITY).getValue();
    distortionHighpass = apvts.getParameterAsValue(PluginParameters::DISTORTION_HIGHPASS).getValue();
    distortionMix = apvts.getParameterAsValue(PluginParameters::DISTORTION_MIX).getValue();

    startTimerHz(60);
}

DistortionVisualizer::~DistortionVisualizer()
{
    apvts.removeParameterListener(PluginParameters::DISTORTION_DENSITY, this);
    apvts.removeParameterListener(PluginParameters::DISTORTION_HIGHPASS, this);
    apvts.removeParameterListener(PluginParameters::DISTORTION_MIX, this);
}

void DistortionVisualizer::paint(Graphics& g)
{
    // fill input buffer with sine wave
    for (int i = 0; i < WINDOW_LENGTH; i++)
    {
        inputBuffer.setSample(0, i, sinf(MathConstants<float>::twoPi * i * SINE_HZ / WINDOW_LENGTH) + cosf(2.5f * MathConstants<float>::pi * i * SINE_HZ / WINDOW_LENGTH));
    }
    distortion.updateParams(distortionDensity, 1.f, distortionMix);
    distortion.process(inputBuffer, inputBuffer.getNumSamples());
    auto range = inputBuffer.findMinMax(0, 0, inputBuffer.getNumSamples()).getLength() / 2.f;

    // draw resulting waveform
    Path path;
    for (int i = 0; i < getWidth(); i++)
    {
        float sample = inputBuffer.getSample(0, i * WINDOW_LENGTH / getWidth());
        float y = jmap<float>(sample, -range, range, float(getHeight()), 0.f);
        if (i == 0)
            path.startNewSubPath(0, y);
        else
            path.lineTo(float(i), y);
    }
    g.setColour(disabled(lnf.WAVEFORM_COLOR));
    g.strokePath(path, PathStrokeType(1.f));
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

void DistortionVisualizer::parameterChanged(const String& parameterID, float newValue)
{
    if (parameterID == PluginParameters::DISTORTION_DENSITY)
        distortionDensity = newValue;
    else if (parameterID == PluginParameters::DISTORTION_HIGHPASS)
        distortionHighpass = newValue;
    else if (parameterID == PluginParameters::DISTORTION_MIX)
        distortionMix = newValue;
    shouldRepaint = true;
}

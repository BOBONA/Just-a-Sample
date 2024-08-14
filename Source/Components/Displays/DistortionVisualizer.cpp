/*
  ==============================================================================

    DistortionVisualizer.cpp
    Created: 23 Jan 2024 9:12:52pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>
#include "DistortionVisualizer.h"

DistortionVisualizer::DistortionVisualizer(APVTS& apvts, int sampleRate) : apvts(apvts), inputBuffer(1, WINDOW_LENGTH),
    densityAttachment(*apvts.getParameter(PluginParameters::DISTORTION_DENSITY), [this](float newValue) { distortionDensity = newValue; repaint(); }, apvts.undoManager),
    mixAttachment(*apvts.getParameter(PluginParameters::DISTORTION_MIX), [this](float newValue) { distortionMix = newValue; repaint(); }, apvts.undoManager)
{
    densityAttachment.sendInitialUpdate();
    mixAttachment.sendInitialUpdate();

    distortion.initialize(1, sampleRate);
}

void DistortionVisualizer::paint(juce::Graphics& g)
{
    // Fill the input buffer with a pre-chosen wave
    for (int i = 0; i < WINDOW_LENGTH; i++)
    {
        inputBuffer.setSample(0, i, sinf(juce::MathConstants<float>::twoPi * i * SINE_HZ / WINDOW_LENGTH) + cosf(2.5f * juce::MathConstants<float>::pi * i * SINE_HZ / WINDOW_LENGTH));
    }
    distortion.updateParams(distortionDensity, 0.f, distortionMix);
    distortion.process(inputBuffer, inputBuffer.getNumSamples());
    auto range = inputBuffer.findMinMax(0, 0, inputBuffer.getNumSamples()).getLength() / 2.f;

    // Draw the resulting waveform
    juce::Path path;
    int numPointsPerPixel = 10;
    for (int i = 0; i < getWidth() * numPointsPerPixel; i++)
    {
        float pos = float(i) / numPointsPerPixel;

        float sample = inputBuffer.getSample(0, int(pos * WINDOW_LENGTH / getWidth()));
        float y = juce::jmap<float>(sample, -range, range, float(getHeight()) * 0.98f, getHeight() * 0.02f);
        if (i == 0)
            path.startNewSubPath(0, y);
        else
            path.lineTo(float(pos), y);
    }

    auto strokeWidth = getWidth() * Layout::fxDisplayStrokeWidth;
    g.setColour(Colors::DARK);
    g.strokePath(path, juce::PathStrokeType(strokeWidth));

    if (!isEnabled())
        g.fillAll(Colors::BACKGROUND.withAlpha(0.5f));
}

void DistortionVisualizer::enablementChanged()
{
    repaint();
}

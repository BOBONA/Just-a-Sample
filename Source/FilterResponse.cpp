/*
  ==============================================================================

    FilterResponse.cpp
    Created: 2 Jan 2024 10:40:21pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>
#include "FilterResponse.h"

FilterResponse::FilterResponse(juce::AudioProcessorValueTreeState& apvts, int sampleRate) : samplerSound(apvts, _b, sampleRate)
{
    setBufferedToImage(true);
    eq.initialize(1, sampleRate);
}

FilterResponse::~FilterResponse()
{
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
        frequencies.add(startFreq * pow(double(endFreq) / startFreq, double(i) / (bounds.getWidth() - 1)));
    }
    eq.updateParams(samplerSound);

    Array<double> magnitudes = eq.getMagnitudeForFrequencyArray(frequencies);

    Path path;
    path.startNewSubPath(bounds.getBottomLeft());
    for (int i = 0; i < bounds.getWidth(); i++)
    {
        float normalizedDecibel = jmap<float>(Decibels::gainToDecibels(magnitudes[i]), -13, 13, bounds.getHeight(), 0);
        path.lineTo(bounds.getX() + i, normalizedDecibel);
    }
    g.setColour(lnf.WAVEFORM_COLOR);
    g.strokePath(path, PathStrokeType{ 2, PathStrokeType::curved });
}

void FilterResponse::resized()
{
    repaint();
}

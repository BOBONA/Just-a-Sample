/*
  ==============================================================================

    SamplePainter.cpp
    Created: 19 Sep 2023 3:02:14pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>
#include "SamplePainter.h"

//==============================================================================
SamplePainter::SamplePainter(juce::Array<int>& voicePositions) : voicePositions(voicePositions)
{
    
}

SamplePainter::~SamplePainter()
{
}

void SamplePainter::paint (juce::Graphics& g)
{
    using namespace juce;
    if (sample)
    {
        // paints the sample
        auto bounds = getLocalBounds();
        Path path{};
        float scale = sample->getNumSamples() / getWidth();
        for (auto i = 0; i < getWidth(); i++)
        {
            auto s = jmap<float>(sample->getSample(0, i * scale), 0, 1, 0, getHeight());
            path.addLineSegment(Line<float>(i, (getHeight() - s) / 2, i, (getHeight() + s) / 2), 1);
        }
        g.setColour(Colours::black);
        g.strokePath(path, PathStrokeType(1.f));

        // paints the voice positions
        path.clear();
        for (auto i = 0; i < voicePositions.size(); i++)
        {
            auto pos = jmap<float>(voicePositions[i], 0, sample->getNumSamples(), 0, getWidth());
            path.addLineSegment(Line<float>(pos, 0, pos, getHeight()), 1);
        }
        g.setColour(Colours::lightgrey);
        g.strokePath(path, PathStrokeType(1.f));
    }
}

void SamplePainter::resized()
{
}

void SamplePainter::setSample(juce::AudioBuffer<float>& sample)
{
    this->sample = &sample;
    repaint();
}

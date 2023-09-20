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
SamplePainter::SamplePainter() : lnf(dynamic_cast<CustomLookAndFeel&>(getLookAndFeel()))
{
    setBufferedToImage(true);
}

SamplePainter::~SamplePainter()
{
}

void SamplePainter::paint(juce::Graphics& g)
{
    if (sample)
    {
        g.setColour(lnf.WAVEFORM_COLOR);
        g.strokePath(path, juce::PathStrokeType(1.f));
    }
}

void SamplePainter::resized()
{
    updatePath();
}

void SamplePainter::updatePath()
{
    using namespace juce;
    if (sample)
    {
        path.clear();
        float scale = sample->getNumSamples() / getWidth();
        for (auto i = 0; i < getWidth(); i++)
        {
            auto s = jmap<float>(sample->getSample(0, i * scale), 0, 1, 0, getHeight());
            path.addLineSegment(Line<float>(i, (getHeight() - s) / 2, i, (getHeight() + s) / 2), 1);
        }
        repaint();
    }
}

void SamplePainter::setSample(juce::AudioBuffer<float>& sample)
{
    this->sample = &sample;
    updatePath();
}

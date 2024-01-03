/*
  ==============================================================================

    SamplePainter.cpp
    Created: 19 Sep 2023 3:02:14pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>
#include "SamplePainter.h"

SamplePainter::SamplePainter()
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
        g.setColour(disabled(lnf.WAVEFORM_COLOR));
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
        float startF = jlimit(0, sample->getNumSamples(), start);
        float stopF = jlimit(start, sample->getNumSamples(), stop);
        float scale = (float(stopF) - startF + 1) / getWidth();
        for (auto i = 0; i < getWidth(); i++)
        {
            auto level = sample->getSample(0, startF + i * scale);
            if (scale > 1)
            {
                level = FloatVectorOperations::findMinAndMax(sample->getReadPointer(0, startF + i * scale), int(scale)).getLength() / 2;
            }
            auto s = jmap<float>(level, 0, 1, 0, getHeight());
            path.addLineSegment(Line<float>(i, (getHeight() - s) / 2, i, (getHeight() + s) / 2), 1);
        }
        repaint();
    }
}

void SamplePainter::setSample(juce::AudioBuffer<float>& sample)
{
    setSample(sample, 0, sample.getNumSamples() - 1);
}

void SamplePainter::setSample(juce::AudioBuffer<float>& sample, int start, int stop)
{
    this->sample = &sample;
    this->start = start;
    this->stop = stop;
    updatePath();
}

void SamplePainter::setSampleView(int start, int stop)
{
    this->start = start;
    this->stop = stop;
    updatePath();
}

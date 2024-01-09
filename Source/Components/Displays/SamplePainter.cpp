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
        int startF = jlimit<int>(0, sample->getNumSamples(), start);
        int stopF = jlimit<int>(start, sample->getNumSamples(), stop);
        float scale = (float(stopF) - startF + 1) / getWidth();
        for (auto i = 0; i < getWidth(); i++)
        {
            auto level = sample->getSample(0, startF + int(i * scale));
            if (scale > 1)
            {
                level = FloatVectorOperations::findMaximum(sample->getReadPointer(0, startF + int(i * scale)), int(scale));
            }
            auto s = jmap<float>(level, 0.f, 1.f, 0.f, float(getHeight()));
            path.addLineSegment(Line<float>(float(i), (getHeight() - s) / 2.f, float(i), (getHeight() + s) / 2.f), 1.f);
        }
        repaint();
    }
}

void SamplePainter::setSample(juce::AudioBuffer<float>& sampleBuffer)
{
    setSample(sampleBuffer, 0, sampleBuffer.getNumSamples() - 1);
}

void SamplePainter::setSample(juce::AudioBuffer<float>& sampleBuffer, int startSample, int stopSample)
{
    sample = &sampleBuffer;
    start = startSample;
    stop = stopSample;
    updatePath();
}

void SamplePainter::setSampleView(int startSample, int stopSample)
{
    start = startSample;
    stop = stopSample;
    updatePath();
}

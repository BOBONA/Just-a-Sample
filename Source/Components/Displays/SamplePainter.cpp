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

void SamplePainter::enablementChanged()
{
    repaint();
}

void SamplePainter::updatePath()
{
    if (sample && sample->getNumChannels() && sample->getNumSamples())
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
            level *= gain;
            auto s = jmap<float>(level, 0.f, 1.f, 0.f, float(getHeight()));
            path.addLineSegment(Line<float>(float(i), (getHeight() - s) / 2.f, float(i), (getHeight() + s) / 2.f), 1.f);
        }
        repaint();
    }
}

void SamplePainter::appendToPath(int startSample, int endSample)
{
    if (sample && sample->getNumChannels() && sample->getNumSamples())
    {
        int startF = jlimit<int>(0, sample->getNumSamples(), start);
        int stopF = jlimit<int>(start, sample->getNumSamples(), stop);
        float scale = (float(stopF) - startF + 1) / getWidth();
        
        int startPixel = int((startSample - startF) / scale);
        int endPixel = int((endSample - startF) / scale);
        for (auto i = startPixel; i <= endPixel; i++)
        {
            auto level = sample->getSample(0, startF + int(i * scale));
            if (scale > 1)
            {
                level = FloatVectorOperations::findMaximum(sample->getReadPointer(0, startF + int(i * scale)), int(scale));
            }
            level *= gain;
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

void SamplePainter::setGain(float newGain)
{
    auto bounds = path.getBounds();
    float scaleFactor = newGain / gain;

    AffineTransform transform;
    transform = transform.translated(0.f, -bounds.getCentreY()); 
    transform = transform.scaled(1.f, scaleFactor);
    transform = transform.translated(0.f, bounds.getCentreY());
    path.applyTransform(transform);

    gain = newGain;
    repaint();
}

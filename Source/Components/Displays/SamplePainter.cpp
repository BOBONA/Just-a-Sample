/*
  ==============================================================================

    SamplePainter.cpp
    Created: 19 Sep 2023 3:02:14pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>

#include "SamplePainter.h"

SamplePainter::SamplePainter(bool useEfficientCache) : useEfficientCache(useEfficientCache)
{
    setBufferedToImage(true);
}

SamplePainter::~SamplePainter()
{
}

void SamplePainter::paint(Graphics& g)
{
    if (sample && sample->getNumChannels() && sample->getNumSamples())
    {
        g.setColour(disabled(lnf.WAVEFORM_COLOR));
        if (!useEfficientCache || stop - start <= cacheThreshold)
        {
            Path path;
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
            g.strokePath(path, PathStrokeType(1.f));
        }
        else
        {
            Path path;
            int startF = int(std::floorf(jlimit<int>(0, sample->getNumSamples(), start) / float(cacheAmount)));
            int stopF = int(std::ceilf(jlimit<int>(start, sample->getNumSamples(), stop) / float(cacheAmount)));
            float scale = (float(stopF) - startF + 1) / getWidth();
            for (auto i = 0; i < getWidth(); i++)
            {
                auto level = cacheData.getSample(0, startF + int(i * scale));
                if (scale > 1)
                {
                    level = FloatVectorOperations::findMaximum(cacheData.getReadPointer(0, startF + int(i * scale)), int(scale));
                }
                level *= gain;
                auto s = jmap<float>(level, 0.f, 1.f, 0.f, float(getHeight()));
                path.addLineSegment(Line<float>(float(i), (getHeight() - s) / 2.f, float(i), (getHeight() + s) / 2.f), 1.f);
            }
            g.strokePath(path, PathStrokeType(1.f));
        }
    }
}

void SamplePainter::enablementChanged()
{
    repaint();
}

void SamplePainter::appendToPath(int startSample, int endSample)
{
    if (sample && useEfficientCache && sample->getNumSamples() > cacheThreshold)
    {
        for (int i = startSample; i < endSample; i += cacheAmount)
        {
            auto level = FloatVectorOperations::findMaximum(sample->getReadPointer(0, i), cacheAmount);
            cacheData.setSample(0, i / cacheAmount, level);
        }
    }
    repaint();
}

void SamplePainter::setSample(const AudioBuffer<float>& sampleBuffer)
{
    sample = &sampleBuffer;
    start = 0;
    stop = sampleBuffer.getNumSamples() - 1;
    if (sample && useEfficientCache && sample->getNumSamples() > cacheThreshold)
    {
        cacheData.setSize(1, int(std::ceilf(float(sample->getNumSamples()) / cacheAmount)));
        cacheData.clear();
        for (int i = 0; i < sample->getNumSamples(); i += cacheAmount)
        {
            auto level = FloatVectorOperations::findMaximum(sample->getReadPointer(0, i), cacheAmount);
            cacheData.setSample(0, i / cacheAmount, level);
        }
    }
    repaint();
}

void SamplePainter::setSample(const AudioBuffer<float>& sampleBuffer, int startSample, int stopSample)
{
    setSample(sampleBuffer);
    start = startSample;
    stop = stopSample;
    repaint();
}

void SamplePainter::setSampleView(int startSample, int stopSample)
{
    start = startSample;
    stop = stopSample;
    repaint();
}

void SamplePainter::setGain(float newGain)
{
    gain = newGain;
    repaint();
}

/*
  ==============================================================================

    SamplePainter.cpp
    Created: 19 Sep 2023 3:02:14pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>

#include "SamplePainter.h"

SamplePainter::SamplePainter(float resolution, bool useEfficientCache) :
    resolution(resolution), sampleData(2, 0), useEfficientCache(useEfficientCache)
{
    setBufferedToImage(true);
}

void SamplePainter::paint(juce::Graphics& g)
{
    using namespace juce;

    if (sample && sample->getNumChannels() && sample->getNumSamples())
    {
        g.setColour(disabled(lnf.WAVEFORM_COLOR));
        g.drawHorizontalLine(getHeight() / 2, 0, getWidth());

        bool useCache = useEfficientCache && viewEnd - viewStart + 1 > cacheThreshold;

        int start = useCache ? viewStart / cacheAmount : viewStart;
        int end = useCache ? viewEnd / cacheAmount : viewEnd;

        // Here's two methods for sampling the data:

        // 1. Keep points constant (smooth but has flickering)
        /*
        int numPoints = getWidth() * resolution;
        float intervalWidth = (end - start + 1) / numPoints;
        */

        // 2. Change number of points while zooming (no flickering but less smooth)
        // We use some math to keep the display smooth as we zoom in. Power of 2 would work, but it looks more jarring as the level of detail changes, so we do a smaller power.
        float sampleDiv = (end - start + 1) / (getWidth() * resolution);
        float base = 1.3f;
        float nextPower = std::pow(base, std::ceil(std::log(sampleDiv) / std::log(base)));

        float intervalWidth = nextPower / base;
        int numPoints = (end - start + 1) / intervalWidth;

        if (viewEnd - viewStart > numPoints)  // Regular display
        {
            sampleData.setSize(sampleData.getNumChannels(), numPoints, false, false, true);

            // Retrieve min and max data
            int startX = int(start / intervalWidth) * intervalWidth;  // Round down to nearest intervalWidth
            for (auto i = 0; i < numPoints; i++)
            {
                int index = startX + intervalWidth * i;
                if (!useCache)
                {
                    float min = 0;
                    float max = 0;
                    for (auto ch = 0; ch < sample->getNumChannels(); ch++)
                    {
                        auto range = FloatVectorOperations::findMinAndMax(sample->getReadPointer(ch, index), int(ceilf(intervalWidth)));
                        min += range.getStart();
                        max += range.getEnd();
                    }
                    min = min / sample->getNumChannels() * gain;
                    max = max / sample->getNumChannels() * gain;
                    sampleData.setSample(0, i, min);
                    sampleData.setSample(1, i, max);
                }
                else
                {
                    auto level = FloatVectorOperations::findMaximum(cacheData.getReadPointer(0, index), int(ceilf(intervalWidth)));
                    sampleData.setSample(0, i, -level * gain);
                    sampleData.setSample(1, i, level * gain);
                }
            }

            // Create the path
            Path path;
            path.startNewSubPath(0, getHeight() / 2.f);
            for (auto i = 0; i < numPoints; i++)
            {
                float xPos = i * getWidth() / float(numPoints);
                float yPos = jmap<float>(sampleData.getSample(1, i), -1, 1, getHeight(), 0);
                path.lineTo(xPos, yPos);
            }
            for (auto i = numPoints - 1; i >= 0; i--)
            {
                float xPos = i * getWidth() / float(numPoints);
                float yPos = jmap<float>(sampleData.getSample(0, i), -1, 1, getHeight(), 0);
                path.lineTo(xPos, yPos);
            }
            path.closeSubPath();
            g.fillPath(path);
        }
        else  // Sample by sample display
        {
            Path path;
            Path circles;
            for (auto i = 0; i < end - start + 1; i++)
            {
                float level = 0;
                for (auto ch = 0; ch < sample->getNumChannels(); ch++)
                    level += sample->getSample(ch, start + i);
                level = level / sample->getNumChannels() * gain;

                float xPos = getWidth() * i / float(end - start);
                float yPos = jmap<float>(level, -1, 1, getHeight(), 0);

                if (!i)
                    path.startNewSubPath(0, yPos);
                path.lineTo(xPos, yPos);

                if (viewEnd - viewStart + 1 < getWidth() / 5)  // Only draw the circles when we are more zoomed in
                    circles.addEllipse(xPos - 2, yPos - 2, 4.f, 4.f);
            }
            g.strokePath(path, PathStrokeType(1.f));
            g.fillPath(circles);
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
            float level = 0;
            for (int ch = 0; ch < sample->getNumChannels(); ch++)
                level += juce::FloatVectorOperations::findMaximum(sample->getReadPointer(ch, i), cacheAmount);
            level /= sample->getNumChannels();
            cacheData.setSample(0, i / cacheAmount, level);
        }
    }
    repaint();
}

void SamplePainter::setSample(const juce::AudioBuffer<float>& sampleBuffer)
{
    sample = &sampleBuffer;
    viewStart = 0;
    viewEnd = sampleBuffer.getNumSamples() - 1;
    if (sample && useEfficientCache && sample->getNumSamples() > cacheThreshold)
    {
        cacheData.setSize(1, int(std::ceilf(float(sample->getNumSamples()) / cacheAmount)));
        for (int i = 0; i < sample->getNumSamples(); i += cacheAmount)
        {
            float level = 0;
            for (int ch = 0; ch < sample->getNumChannels(); ch++)
                level += juce::FloatVectorOperations::findMaximum(sample->getReadPointer(ch, i), cacheAmount);
            level /= sample->getNumChannels();
            cacheData.setSample(0, i / cacheAmount, level);
        }
}
    repaint();
}

void SamplePainter::setSample(const juce::AudioBuffer<float>& sampleBuffer, int startSample, int stopSample)
{
    setSample(sampleBuffer);
    viewStart = startSample;
    viewEnd = stopSample;
    repaint();
}

void SamplePainter::setSampleView(int startSample, int stopSample)
{
    viewStart = startSample;
    viewEnd = stopSample;
    repaint();
}

void SamplePainter::setGain(float newGain)
{
    gain = newGain;
    repaint();
}

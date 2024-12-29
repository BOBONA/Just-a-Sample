/*
  ==============================================================================

    SamplePainter.cpp
    Created: 19 Sep 2023 3:02:14pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>

#include "SamplePainter.h"

SamplePainter::SamplePainter(float resolution) : resolution(resolution), sampleData(2, 0)
{
    setBufferedToImage(true);
}

void SamplePainter::paint(juce::Graphics& g)
{
    if (!sample || !sample->getNumChannels() || sample->getNumSamples() <= 1 || viewEnd <= viewStart || viewStart >= sample->getNumSamples() || viewEnd >= sample->getNumSamples())
        return;

    using namespace juce;

    g.setColour(findColour(Colors::painterColorId, true));
    g.drawHorizontalLine(getHeight() / 2.f, 0, getWidth());

    int start = viewStart;
    int end = viewEnd;
    int viewSize = end - start + 1;

    // We adjust numPoints in factors of 2 to keep the view smooth looking
    float sampleDiv = viewSize / (getWidth() * resolution);
    float base = 2.f;
    float nextPower = std::powf(base, std::ceilf(std::logf(sampleDiv) / std::logf(base)));

    float intervalWidth = nextPower / base;
    int numPoints = int(viewSize / intervalWidth);

    // While we could do a more general solution with a variable amount of caches, let's just use two fixed caches
    int cacheRatio = intervalWidth >= cache2Amount ? cache2Amount : intervalWidth >= cache1Amount ? cache1Amount : 1.f;
    auto& cacheData = cacheRatio == cache1Amount ? cache1Data : cache2Data;

    if (intervalWidth > 2.f)  // Regular display
    {
        // Sample the data
        sampleData.setSize(sampleData.getNumChannels(), numPoints, false, false, true);

        // To keep sampling more consistent, we round down to intervalWidth
        float startX = std::floorf(start / intervalWidth) * intervalWidth / cacheRatio;

        for (auto i = 0; i < numPoints; i++)
        {
            int index = int(startX + intervalWidth / cacheRatio * i);
            int numValues = int(startX + intervalWidth / cacheRatio * (i + 1)) - index;

            float min{ 0.f };
            float max{ 0.f };
            if (cacheRatio > 1.f)
            {
                min = FloatVectorOperations::findMinimum(cacheData.getReadPointer(0, index), numValues);
                max = FloatVectorOperations::findMaximum(cacheData.getReadPointer(1, index), numValues);
            }
            else
            {
                for (auto ch = 0; ch < sample->getNumChannels(); ch++)
                {
                    auto range = FloatVectorOperations::findMinAndMax(sample->getReadPointer(ch, index), numValues);
                    min += range.getStart();
                    max += range.getEnd();
                }
                min = min / sample->getNumChannels();
                max = max / sample->getNumChannels();
            }
            sampleData.setSample(0, i, min * gain);
            sampleData.setSample(1, i, max * gain);
        }

        // Create the path
        float offset = float(startX * cacheRatio - viewStart) / viewSize * getWidth();

        Path path;
        for (auto i = 0; i < numPoints; i++)
        {
            float x = offset + jmap<float>(i, 0.f, numPoints - 2.f, 0.f, float(getWidth()));
            float yMax = jmap<float>(sampleData.getSample(1, i), -1.f, 1.f, float(getHeight()), 0.f);
            float yMin = jmap<float>(sampleData.getSample(0, i), -1.f, 1.f, float(getHeight()), 0.f);
            
            if (!i)
                path.startNewSubPath(x, yMax);
            else
                path.lineTo(x, yMax);
            path.lineTo(x, yMin);
        }
        g.strokePath(path, PathStrokeType(2.f, PathStrokeType::beveled));
    }
    else  // Sample by sample display
    {
        Path path;
        Path circles;
        for (auto i = 0; i < viewSize; i++)
        {
            float level = 0;
            for (auto ch = 0; ch < sample->getNumChannels(); ch++)
                level += sample->getSample(ch, start + i);
            level = level / sample->getNumChannels() * gain;

            float xPos = float(getWidth() * i) / (end - start);
            float yPos = jmap<float>(level, -1, 1, getHeight(), 0);

            if (!i)
                path.startNewSubPath(0, yPos);
            path.lineTo(xPos, yPos);

            if (viewSize < getWidth() / 5.f)  // Only draw the circles when we are further zoomed in
                circles.addEllipse(xPos - 2.5f, yPos - 2.5f, 5.f, 5.f);
        }
        g.strokePath(path, PathStrokeType(2.f, PathStrokeType::curved));
        g.fillPath(circles);
    }
}

void SamplePainter::enablementChanged()
{
    repaint();
}

void SamplePainter::updateCaches(int start, int end)
{
    if (!sample)
        return;

    cache1Data.setSize(2, int(1 + std::ceilf(float(sample->getNumSamples()) / cache1Amount)), true, false, false);
    for (int i = start; i < end; i += cache1Amount)
    {
        float min = 0;
        float max = 0;
        for (int ch = 0; ch < sample->getNumChannels(); ch++)
        {
            auto range = juce::FloatVectorOperations::findMinAndMax(sample->getReadPointer(ch, i), juce::jmin(cache1Amount, sample->getNumSamples() - i));
            min += range.getStart();
            max += range.getEnd();
        }
        min /= sample->getNumChannels();
        max /= sample->getNumChannels();
        cache1Data.setSample(0, i / cache1Amount, min);
        cache1Data.setSample(1, i / cache1Amount, max);
    }

    int cacheRatio = int(cache2Amount / cache1Amount);
    cache2Data.setSize(2, int(std::ceilf(float(cache1Data.getNumSamples() + 1) / cacheRatio)), true, false, false);
    for (int i = start / cache1Amount; i < end / cache1Amount; i += cacheRatio)
    {
        float min = juce::FloatVectorOperations::findMinimum(cache1Data.getReadPointer(0, i), juce::jmin(cacheRatio, cache1Data.getNumSamples() - i));
        float max = juce::FloatVectorOperations::findMaximum(cache1Data.getReadPointer(1, i), juce::jmin(cacheRatio, cache1Data.getNumSamples() - i));
        cache2Data.setSample(0, i / cacheRatio, min);
        cache2Data.setSample(1, i / cacheRatio, max);
    }
}

void SamplePainter::appendToPath(int startSample, int endSample)
{
    if (!sample)
        return;

    updateCaches(startSample, endSample);

    repaint();
}

void SamplePainter::setSample(const juce::AudioBuffer<float>& sampleBuffer)
{
    sample = &sampleBuffer;
    viewStart = 0;
    viewEnd = sampleBuffer.getNumSamples() - 1;

    if (!sample)
        return;

    updateCaches(0, sample->getNumSamples());
    
    repaint();
}

void SamplePainter::setSample(const juce::AudioBuffer<float>& sampleBuffer, int viewStartSample, int viewEndSample)
{
    setSample(sampleBuffer);
    viewStart = viewStartSample;
    viewEnd = viewEndSample;
    repaint();
}

void SamplePainter::setSampleView(int viewStartSample, int viewEndSample)
{
    viewStart = viewStartSample;
    viewEnd = viewEndSample;
    repaint();
}

void SamplePainter::setGain(float newGain)
{
    gain = newGain;
    repaint();
}

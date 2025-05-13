/*
  ==============================================================================

    SamplePainter.cpp
    Created: 19 Sep 2023 3:02:14pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>

#include <algorithm>

#include "SamplePainter.h"

SamplePainter::SamplePainter(ListenableAtomic<int>& primaryVisibleChannel, float resolutionScale, UIDummyParam* uiDummyParam) :
    resolutionScale(resolutionScale), sampleData(2, 0),
    primaryChannel(primaryVisibleChannel), dummyParam(uiDummyParam)
{
    primaryChannel.addListener(this);

    setBufferedToImage(true);
}

SamplePainter::~SamplePainter()
{
    primaryChannel.removeListener(this);
}

void SamplePainter::paint(juce::Graphics& g)
{
    if (!sample || !sample->getNumChannels() || sample->getNumSamples() <= 1 || viewEnd <= viewStart || viewStart >= sample->getNumSamples() || 
        viewEnd >= sample->getNumSamples() || sample->getNumSamples() != sampleSize || numPoints == 0)

        return;

    using namespace juce;

    g.setColour(findColour(Colors::painterColorId, true));

    // Draw a horizontal line at 0
    float dividerHeight = getHeight() * 0.0035f;
    g.fillRect(0.f, (getHeight() - dividerHeight) / 2.f, float(getWidth()), dividerHeight);

    int start = viewStart;
    int end = viewEnd;
    int viewSize = end - start + 1;

    // While we could do a more general solution with a variable amount of caches, let's just use two fixed caches
    int cacheRatio = intervalWidth >= cache2Amount ? cache2Amount : intervalWidth >= cache1Amount ? cache1Amount : 1.f;
    auto& cacheData = (mono && sample->getNumChannels() > 1)
        ? (cacheRatio == cache1Amount ? cache1DataMono : cache2DataMono)
        : (cacheRatio == cache1Amount ? cache1Data : cache2Data);

    auto strokeWidth = getWidth() / resolution * 1.25f;

    // Regular display
    if (!isSampleBySample())  
    {
        // Sample the data
        sampleData.setSize(sampleData.getNumChannels(), numPoints, false, false, true);

        // To keep sampling more consistent, we round down to intervalWidth
        float startX = std::floor(start / intervalWidth) * intervalWidth / cacheRatio;

        for (auto i = 0; i < numPoints; i++)
        {
            int index = int(startX + intervalWidth / cacheRatio * i);
            int numValues = int(startX + intervalWidth / cacheRatio * (i + 1)) - index;

            float min;
            float max;
            if (cacheRatio > 1.f)
            {
                min = FloatVectorOperations::findMinimum(cacheData.getReadPointer(0, index), numValues);
                max = FloatVectorOperations::findMaximum(cacheData.getReadPointer(1, index), numValues);
            }
            else if (mono)
            {
                downsample(*sample, true, index, numValues, min, max);
            }
            else
            {
                downsample(*sample, false, index, numValues, min, max);
            }
            sampleData.setSample(0, i, min * gain);
            sampleData.setSample(1, i, max * gain);
        }

        // Create the path
        float offset = float(startX * cacheRatio - viewStart) / viewSize * getWidth();

        Path path;
        path.preallocateSpace(numPoints * 2 * 3);
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
        g.strokePath(path, PathStrokeType(strokeWidth, PathStrokeType::beveled));
    }

    // Sample by sample display
    else  
    {
        // If mono is enabled, we average the channels and exit this loop on the first iteration
        // Otherwise, we draw each channel separately with a different opacity

        // Find the primary channel to display
        int primary = primaryChannel;
        if (selectingChannel >= 0)
            primary = selectingChannel;

        Path path;
        path.preallocateSpace(3 * (viewSize + 1));

        Path circles;
        auto circRadius = 0.003125f * getWidth();
        bool drawCircles = viewSize <= SAMPLE_BY_SAMPLE_THRESHOLD;
        if (drawCircles)
            path.preallocateSpace(4 * viewSize);

        for (auto ch = 0; ch < sample->getNumChannels(); ch++)
        {
            for (auto i = 0; i < viewSize; i++)
            {
                float level = sample->getSample(ch, start + i);

                // If mono is enabled, we average the channels
                if (mono)
                {
                    level = 0;
                    for (auto ch2 = 0; ch2 < sample->getNumChannels(); ch2++)
                        level += sample->getSample(ch2, start + i);
                    level /= sample->getNumChannels();
                }

                level *= gain;

                float xPos = float(getWidth() * i) / (end - start);
                float yPos = jmap<float>(level, -1, 1, getHeight(), 0);

                if (!i)
                    path.startNewSubPath(0, yPos);
                path.lineTo(xPos, yPos);

                // Only draw the circles when we are further zoomed in
                if (drawCircles)
                    circles.addEllipse(xPos - circRadius, yPos - circRadius, circRadius * 2.f, circRadius * 2.f);
            }

            int channelOrderNum = (primary - ch + sample->getNumChannels()) % sample->getNumChannels();
            float opacity = std::pow(0.6f, channelOrderNum);
            if (mono)
                opacity = 1.f;

            g.setColour(findColour(Colors::painterColorId, true).withMultipliedAlpha(opacity));
            g.strokePath(path, PathStrokeType(strokeWidth, PathStrokeType::curved));
            g.fillPath(circles);

            if (mono)
                break;

            path.clear();
            circles.clear();
        }

        path.clear();
        circles.clear();
    }
}

void SamplePainter::resized()
{
    recalculateIntervals();
}

void SamplePainter::enablementChanged()
{
    repaint();
}

juce::String SamplePainter::getCustomHelpText()
{
    auto mousePos = getMouseXYRelative();
    int hoveringChannel = getChannel(mousePos.x, mousePos.y);

    if (hoveringChannel == -1 || sample->getNumChannels() == 1)
        return "";

    if (sample->getNumChannels() == 2)
        return "Focus " + juce::String(hoveringChannel == 0 ? "left" : "right") + " channel";

    return "Focus channel " + juce::String(hoveringChannel + 1);
}

bool SamplePainter::isSampleBySample() const
{
    return intervalWidth <= 5.f;
}

int SamplePainter::getChannel(int x, int y) const
{
    if (!sample || !sample->getNumChannels() || !isSampleBySample() || mono)
        return -1;

    int closestCh = -1;
    float bestYDist = getHeight() / 6.f;

    int start = viewStart;
    int end = viewEnd;
    int viewSize = end - start + 1;

    // Estimate closest sample index from x
    float normX = float(x) / getWidth();
    int i = juce::jlimit(0, viewSize - 1, int(std::round(normX * (end - start))));

    for (int ch = 0; ch < sample->getNumChannels(); ++ch)
    {
        float level = sample->getSample(ch, start + i) * gain;
        float yPos = juce::jmap<float>(level, -1, 1, getHeight(), 0);
        float yDist = std::abs(y - yPos);

        if (yDist < bestYDist)
        {
            bestYDist = yDist;
            closestCh = ch;
        }
    }

    return closestCh;
}

void SamplePainter::mouseDown(const juce::MouseEvent& event)
{
    if (!isSampleBySample() || mono)
        return;

    selectingChannel = getChannel(event.x, event.y);

    repaint();
}

void SamplePainter::mouseUp(const juce::MouseEvent& event)
{
    if (!isSampleBySample() || mono)
        return;

    int hoveringChannel = getChannel(event.x, event.y);

    if (selectingChannel >= 0 && hoveringChannel == selectingChannel)
    {
        primaryChannel = selectingChannel;
        if (dummyParam)
            dummyParam->sendUIUpdate();
    }

    selectingChannel = -1;

    repaint();
}

void SamplePainter::valueChanged(ListenableValue<int>& source, int newValue)
{
    repaint();
}

void SamplePainter::downsample(const juce::AudioBuffer<float>& buffer, bool average, int start, int numSamples, float& outMin, float& outMax)
{
    // Average channels for mono downsample
    if (average)
    {
        downsampleBuffer.setSize(1, numSamples, false, false, true);
        downsampleBuffer.clear();
        for (int ch = 0; ch < buffer.getNumChannels(); ch++)
            downsampleBuffer.addFrom(0, 0, buffer, ch, start, numSamples);
        auto range = juce::FloatVectorOperations::findMinAndMax(downsampleBuffer.getReadPointer(0), numSamples);

        outMin = range.getStart() / buffer.getNumChannels();
        outMax = range.getEnd() / buffer.getNumChannels();
    }

    // Downsample across all channels
    else
    {
        float min = std::numeric_limits<float>::max();
        float max = std::numeric_limits<float>::lowest();
        for (int ch = 0; ch < buffer.getNumChannels(); ch++)
        {
            auto range = juce::FloatVectorOperations::findMinAndMax(buffer.getReadPointer(ch, start), numSamples);
            min = juce::jmin(range.getStart(), min);
            max = juce::jmax(range.getEnd(), max);
        }

        outMin = min;
        outMax = max;
    }
}

void SamplePainter::updateCaches(int start, int end)
{
    if (!sample || start < 0)
        return;

    float min;
    float max;

    int cacheRatio = int(cache2Amount / cache1Amount);

    const int cache1Size = int(1 + std::ceil(float(sample->getNumSamples()) / cache1Amount));
    int effectiveStart = (start / cache1Amount) * cache1Amount;  // Round down to the nearest cache1Amount
    cache1Data.setSize(2, cache1Size, true, false, false);
    for (int i = effectiveStart; i < end; i += cache1Amount)
    {
        int numSamples = juce::jmin(cache1Amount, sample->getNumSamples() - i);
        downsample(*sample, false, i, numSamples, min, max);

        cache1Data.setSample(0, i / cache1Amount, min);
        cache1Data.setSample(1, i / cache1Amount, max);
    }

    const int cache2Size = int(std::ceil(float(cache1Data.getNumSamples() + 1) / cacheRatio));
    int effectiveCache2Start = (effectiveStart / cache1Amount / cacheRatio) * cacheRatio;  // Round down to the nearest cacheRatio
    cache2Data.setSize(2, cache2Size, true, false, false);
    for (int i = effectiveCache2Start; i < end / cache1Amount; i += cacheRatio)
    {
        int numSamples = juce::jmin(cacheRatio, cache1Data.getNumSamples() - i);
        min = juce::FloatVectorOperations::findMinimum(cache1Data.getReadPointer(0, i), numSamples);
        max = juce::FloatVectorOperations::findMaximum(cache1Data.getReadPointer(1, i), numSamples);

        cache2Data.setSample(0, i / cacheRatio, min);
        cache2Data.setSample(1, i / cacheRatio, max);
    }

    if (sample->getNumChannels() > 1)
    {
        cache1DataMono.setSize(2, cache1Size, true, false, false);
        for (int i = effectiveStart; i < end; i += cache1Amount)
        {
            int numSamples = juce::jmin(cache1Amount, sample->getNumSamples() - i);
            downsample(*sample, true, i, numSamples, min, max);

            cache1DataMono.setSample(0, i / cache1Amount, min);
            cache1DataMono.setSample(1, i / cache1Amount, max);
        }

        cache2DataMono.setSize(2, cache2Size, true, false, false);
        for (int i = effectiveCache2Start; i < end / cache1Amount; i += cacheRatio)
        {
            int numSamples = juce::jmin(cacheRatio, cache1DataMono.getNumSamples() - i);
            min = juce::FloatVectorOperations::findMinimum(cache1DataMono.getReadPointer(0, i), numSamples);
            max = juce::FloatVectorOperations::findMaximum(cache1DataMono.getReadPointer(1, i), numSamples);

            cache2DataMono.setSample(0, i / cacheRatio, min);
            cache2DataMono.setSample(1, i / cacheRatio, max);
        }
    }
}

void SamplePainter::appendToPath(int startSample, int endSample)
{
    if (!sample)
        return;

    updateCaches(startSample, endSample);
    recalculateIntervals();
    repaint();
}

void SamplePainter::setSample(const juce::AudioBuffer<float>& sampleBuffer)
{
    sample = &sampleBuffer;
    sampleSize = sampleBuffer.getNumSamples();

    viewStart = 0;
    viewEnd = sampleBuffer.getNumSamples() - 1;

    if (!sample)
        return;

    updateCaches(0, sample->getNumSamples());
    recalculateIntervals();
    repaint();
}

void SamplePainter::setSample(const juce::AudioBuffer<float>& sampleBuffer, int viewStartSample, int viewEndSample)
{
    setSample(sampleBuffer);
    viewStart = viewStartSample;
    viewEnd = viewEndSample;

    recalculateIntervals();
    repaint();
}

void SamplePainter::setSampleView(int viewStartSample, int viewEndSample)
{
    viewStart = viewStartSample;
    viewEnd = viewEndSample;

    recalculateIntervals();
    repaint();
}

void SamplePainter::recalculateIntervals()
{
    int viewSize = viewEnd - viewStart + 1;

    // Set resolution as a constant factor of the screen width
    int width = getWidth();
    if (const juce::Displays::Display* screen = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        width = screen->userArea.getWidth();

    resolution = width * resolutionScale;

    // We adjust numPoints in factors of 2 to keep the view smooth looking
    float sampleDiv = viewSize / resolution;
    float base = 2.f;
    float nextPower = std::pow(base, std::ceil(std::log(sampleDiv) / std::log(base)));

    intervalWidth = nextPower / base;
    numPoints = int(viewSize / intervalWidth);
}

void SamplePainter::setGain(float newGain)
{
    gain = newGain;

    repaint();
}

void SamplePainter::setMono(bool isMono)
{
    mono = isMono;

    if (!isMono)
        selectingChannel = -1;

    repaint();
}

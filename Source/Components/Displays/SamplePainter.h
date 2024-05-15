/*
  ==============================================================================

    SamplePainter.h
    Created: 19 Sep 2023 3:02:14pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "../ComponentUtils.h"

class SamplePainter : public CustomComponent
{
public:
    SamplePainter(bool useEfficientCache = true);
    ~SamplePainter() override;

    void paint(Graphics&) override;
    void enablementChanged() override;

    /** This adds (does not remove) to the path along that given start and end samples */
    void appendToPath(int startSample, int endSample);

    void setSample(const AudioBuffer<float>& sampleBuffer);
    void setSample(const AudioBuffer<float>& sampleBuffer, int startSample, int stopSample);
    void setSampleView(int startSample, int stopSample);

    /** Change the gain of the sample and repaint */
    void setGain(float newGain);

private:
    const AudioBuffer<float>* sample{ nullptr };
    int start{ 0 }, stop{ 0 };
    float gain{ 1.f };

    const int cacheAmount{ 250 }; // Number of samples per pixel in cache
    const int cacheThreshold{ 1000000 }; // Number of samples needed before the painter uses the cache
    bool useEfficientCache{ false };
    AudioBuffer<float> cacheData;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplePainter)
};

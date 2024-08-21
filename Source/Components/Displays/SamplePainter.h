/*
  ==============================================================================

    SamplePainter.h
    Created: 19 Sep 2023 3:02:14pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "../../Utilities/ComponentUtils.h"

/** A custom component that paints a waveform of a sample. An optional cache is used to help render large
    waveforms with minimal overhead. Additionally, a method is provided to append to the path for real-time recording.
*/
class SamplePainter final : public CustomComponent
{
public:
    explicit SamplePainter(float resolution = 1.f);
    ~SamplePainter() override = default;

    /** This adds (does not remove) to the path along the given start and end samples */
    void appendToPath(int startSample, int endSample);

    void setSample(const juce::AudioBuffer<float>& sampleBuffer);
    void setSample(const juce::AudioBuffer<float>& sampleBuffer, int startSample, int stopSample);
    void setSampleView(int startSample, int stopSample);

    /** Change the gain of the sample and repaint */
    void setGain(float newGain);

private:
    void paint(juce::Graphics& g) override;
    void enablementChanged() override;

    void updateCaches(int start, int end);

    //==============================================================================
    const juce::AudioBuffer<float>* sample{ nullptr };
    int viewStart{ 0 }, viewEnd{ 0 };
    float gain{ 1.f };

    float resolution{ 1. };

    // Intermediate buffer
    juce::AudioBuffer<float> sampleData;

    /** The cache is a down-sampled version of the sample that is used to speed up rendering */
    juce::AudioBuffer<float> cache1Data, cache2Data;
    static constexpr int cache1Amount{ 100 }, cache2Amount{ 5000 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplePainter)
};

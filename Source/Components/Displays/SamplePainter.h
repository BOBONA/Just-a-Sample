/*
  ==============================================================================

    SamplePainter.h
    Created: 19 Sep 2023 3:02:14pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "../../Sampler/CustomSamplerVoice.h"
#include "../../Utilities/ComponentUtils.h"

/** A custom component that paints a waveform of a sample. A cache is used to help render large
    waveforms with minimal overhead. Additionally, a method is provided to append to the path for real-time recording.
*/
class SamplePainter final : public CustomComponent, public ValueListener<int>
{
public:
    explicit SamplePainter(ListenableAtomic<int>& primaryVisibleChannel, float resolutionScale = 0.25f, UIDummyParam* dummyParam = nullptr);
    ~SamplePainter() override;

    /** This adds (does not remove) to the path along the given start and end samples */
    void appendToPath(int startSample, int endSample);

    void setSample(const juce::AudioBuffer<float>& sampleBuffer);
    void setSample(const juce::AudioBuffer<float>& sampleBuffer, int viewStartSample, int viewEndSample);
    void setSampleView(int viewStartSample, int viewEndSample);

    /** Change gain and repaint */
    void setGain(float newGain);

    /** Change mono display and repaint */
    void setMono(bool isMono);

private:
    void paint(juce::Graphics& g) override;
    void resized() override;
    void enablementChanged() override;
    void colourChanged() override;
    juce::String getCustomHelpText() override;

    /** Whether sample by sample display is active */
    bool isSampleBySample() const;

    /** Get the channel number of the sample at the given x and y coordinates (within a snap amount) */
    int getChannel(int x, int y) const;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    void valueChanged(ListenableValue<int>& source, int newValue) override;

    /** Downsample a range of a buffer. If average is true, the values are averaged, otherwise the max value is taken (across a
        a single channel if channel is set, or across all channels if channel is -1). 
    */
    void downsample(const juce::AudioBuffer<float>& buffer, bool average, int start, int numSamples, float& outMin, float& outMax);
    void updateCaches(int start, int end);
    void recalculateIntervals();

    //==============================================================================
    const juce::AudioBuffer<float>* sample{ nullptr };
    int sampleSize{ 0 };

    int viewStart{ 0 }, viewEnd{ 0 };
    float gain{ 1.f };
    bool mono{ false };

    float resolutionScale{ 1.f };
    float resolution{ 0.f };
    int numPoints{ 0 };
    float intervalWidth{ 0.f };

    /** An intermediate buffer */
    juce::AudioBuffer<float> sampleData;
    juce::AudioBuffer<float> downsampleBuffer;

    /** The cache is a down-sampled version of the sample that is used to speed up rendering */
    juce::AudioBuffer<float> cache1Data, cache2Data;
    juce::AudioBuffer<float> cache1DataMono, cache2DataMono;
    static constexpr int cache1Amount{ 100 }, cache2Amount{ 5000 };

    const int SAMPLE_BY_SAMPLE_THRESHOLD{ 150 };

    /** Which channel has full opacity */
    ListenableAtomic<int>& primaryChannel;
    UIDummyParam* dummyParam{ nullptr };

    /** Set when a mouse down has occurred on a channel */
    int selectingChannel{ -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplePainter)
};

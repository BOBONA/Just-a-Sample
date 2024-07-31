/*
  ==============================================================================

    ReverbResponse.h
    Created: 17 Jan 2024 11:16:38pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "../../Utilities/readerwriterqueue/readerwriterqueue.h"
#include "../../Utilities/ComponentUtils.h"
#include "../../Sampler/Effects/TriReverb.h"

struct ReverbResponseChange
{
    enum ChangeType
    {
        VALUE,
        DECREASE_SIZE
    };

    ReverbResponseChange(ChangeType type, int index, float newValue=0) :
        type(type),
        index(index),
        newValue(newValue)
    {
    }

    ChangeType type;
    int index; // use as size when type is DECREASE_SIZE
    float newValue;
};

class ReverbResponseThread final : public juce::Thread
{
public:
    explicit ReverbResponseThread(int sampleRate);
    ~ReverbResponseThread() override;

    void run() override;

    void initializeImpulse();
    void calculateRMS(int windowWidth);

    moodycamel::ReaderWriterQueue<ReverbResponseChange, 16384> responseChangeQueue;
    std::atomic<float> size, damping, predelay, lows, highs, mix;

private:
    const float SAMPLE_RATE_RATIO{ 1.f }; // decreasing the response sample rate is much faster but gives inaccurate responses for the filters
    const int DISPLAY_TIME{ 10 };
    const float IMPULSE_TIME{ 0.05f };
    const float CHIRP_START{ 50.f };
    const float CHIRP_END{ 18000.f };
    const int EMPTY_RATIO{ 10 }; // how many times to process the empty buffer

    //==============================================================================
    int sampleRate;
    std::atomic<int> width{ 0 };
    std::atomic_flag updateRMS;

    TriReverb reverb;
    juce::AudioBuffer<float> impulse;
    juce::AudioBuffer<float> empty; // empty buffer for the reverb processing
};

class ReverbResponse final : public CustomComponent, public juce::Timer, public APVTS::Listener
{
public:
    ReverbResponse(APVTS& apvts, int sampleRate);
    ~ReverbResponse() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void enablementChanged() override;

    void timerCallback() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

private:
    APVTS& apvts;
    int sampleRate;

    ReverbResponseThread responseThread;
    juce::Array<float> rmsRecordings;
    int rmsRecordingsEffectiveSize{ 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbResponse)
};
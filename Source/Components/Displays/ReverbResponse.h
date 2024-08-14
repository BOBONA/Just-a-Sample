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
#include "../../Sampler/Effects/Reverb.h"

struct ReverbResponseChange
{
    enum ChangeType
    {
        NEW_SAMPLES,
        CLEAR
    };

    ChangeType type;
    juce::AudioBuffer<float> samples{};
};

class ReverbResponseThread final : public juce::Thread
{
public:
    explicit ReverbResponseThread(const APVTS& apvts, int sampleRate);
    ~ReverbResponseThread() override;

    void run() override;

    moodycamel::ReaderWriterQueue<ReverbResponseChange, 16384>& getResponseChangeQueue() { return responseChangeQueue; }

    //==============================================================================
    static constexpr int DISPLAY_TIME{ 10 };
    static constexpr float SAMPLE_RATE_RATIO{ 10.f };

private:
    static constexpr float IMPULSE_TIME{ 0.05f };
    static constexpr float CHIRP_START{ 50.f };
    static constexpr float CHIRP_END{ 18000.f };

    void initializeImpulse();

    //==============================================================================
    int sampleRate;

    Reverb reverb;
    std::atomic<float> size{ 0.f }, damping{ 0.f }, delay{ 0.f }, mix{ 0.f };
    std::atomic<bool> reverbChanged{ false };
    juce::ParameterAttachment sizeAttachment, dampingAttachment, delayAttachment, mixAttachment;

    juce::AudioBuffer<float> impulse;

    moodycamel::ReaderWriterQueue<ReverbResponseChange, 16384> responseChangeQueue;
};

class ReverbResponse final : public CustomComponent, public juce::Timer
{
public:
    ReverbResponse(APVTS& apvts, int sampleRate);

    void paint (juce::Graphics&) override;
    void enablementChanged() override;

    void timerCallback() override;

private:
    APVTS& apvts;
    int sampleRate;

    float lows{ 0.f }, highs{ 0.f }, mix{ 0.f };
    juce::ParameterAttachment lowsAttachment, highsAttachment, mixAttachment;

    ReverbResponseThread responseThread;
    std::queue<ReverbResponseChange> responseChanges;
    int numSamples{ 0 };
    juce::AudioBuffer<float> response;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbResponse)
};
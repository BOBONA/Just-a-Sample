/*
  ==============================================================================

    FilterResponse.h
    Created: 2 Jan 2024 10:40:21pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "../../Sampler/Effects/BandEQ.h"
#include "../ComponentUtils.h"

enum FilterResponseParts
{
    NONE,
    LOW_FREQ,
    HIGH_FREQ
};

class FilterResponse final : public CustomComponent, public APVTS::Listener, public juce::Timer
{
public:
    FilterResponse(APVTS& apvts, int sampleRate);
    ~FilterResponse() override;

private:
    void paint(juce::Graphics&) override;
    void resized() override;
    void enablementChanged() override;

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void timerCallback() override;

    void mouseMove(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    FilterResponseParts getClosestPartInRange(int x, int y) const;

    float freqToPos(juce::Rectangle<float> bounds, float freq) const;
    float posToFreq(juce::Rectangle<float> bounds, float pos) const;

    //==============================================================================
    constexpr static int startFreq{ 20 };
    constexpr static int endFreq{ 17500 };

    //==============================================================================
    APVTS& apvts;
    juce::ParameterAttachment lowFreqAttachment, highFreqAttachment;

    // AFAIK these member variables are necessary to maintain an up-to-date GUI
    float lowFreq{ 0 }, highFreq{ 0 }, lowGain{ 0 }, midGain{ 0 }, highGain{ 0 };
    bool shouldRepaint{ false };
    BandEQ eq;

    bool dragging{ false };
    FilterResponseParts draggingTarget{ FilterResponseParts::NONE };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterResponse)
};

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
#include "../../Utilities/ComponentUtils.h"

enum FilterResponseParts
{
    NONE,
    LOW_FREQ,
    HIGH_FREQ
};

class FilterResponse final : public CustomComponent
{
public:
    FilterResponse(APVTS& apvts, int sampleRate);

private:
    void paint(juce::Graphics&) override;
    void enablementChanged() override;

    void mouseMove(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

    FilterResponseParts getClosestPartInRange(int x, int y) const;

    float freqToPos(juce::Rectangle<float> bounds, float freq) const;
    float posToFreq(juce::Rectangle<float> bounds, float pos) const;

    juce::String getCustomHelpText() override;

    //==============================================================================
    constexpr static int startFreq{ 20 };
    constexpr static int endFreq{ 17500 };

    //==============================================================================
    APVTS& apvts;
    BandEQ eq;

    float lowFreq{ 0 }, highFreq{ 0 }, lowGain{ 0 }, midGain{ 0 }, highGain{ 0 };
    juce::ParameterAttachment lowFreqAttachment, highFreqAttachment, lowGainAttachment, midGainAttachment, highGainAttachment;

    bool dragging{ false };
    FilterResponseParts draggingTarget{ NONE };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterResponse)
};

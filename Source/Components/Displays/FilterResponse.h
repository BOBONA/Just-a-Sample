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

class FilterResponse : public CustomComponent, public AudioProcessorValueTreeState::Listener, public Timer
{
public:
    FilterResponse(AudioProcessorValueTreeState& apvts, int sampleRate);
    ~FilterResponse() override;

    void setSampleRate(int sampleRate);

    void paint(juce::Graphics&) override;
    void resized() override;
    void enablementChanged() override;

    void parameterChanged(const String& parameterID, float newValue) override;
    void timerCallback() override;

    void mouseMove(const MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    FilterResponseParts getClosestPartInRange(int x, int y) const;

    float freqToPos(Rectangle<float> bounds, float freq) const;
    float posToFreq(Rectangle<float> bounds, float pos) const;

private:
    const static int startFreq{ 20 };
    const static int endFreq{ 17500 };

    AudioProcessorValueTreeState& apvts;
    ParameterAttachment lowFreqAttachment, highFreqAttachment;

    float lowFreq{ 0 }, highFreq{ 0 }, lowGain{ 0 }, midGain{ 0 }, highGain{ 0 }; // afaik this is necessary to get up to date GUI
    bool shouldRepaint{ false };
    BandEQ eq;

    bool dragging{ false };
    FilterResponseParts draggingTarget{ FilterResponseParts::NONE };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterResponse)
};

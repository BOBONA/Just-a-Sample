/*
  ==============================================================================

    ChorusVisualizer.h
    Created: 30 Jan 2024 6:12:49pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "../../utilities/readerwriterqueue/readerwriterqueue.h"
#include "../../Sampler/Effects/Chorus.h"
#include "../ComponentUtils.h"

class ChorusVisualizer : public CustomComponent, public juce::Timer, public APVTS::Listener
{
public:
    ChorusVisualizer(APVTS& apvts, int sampleRate);
    ~ChorusVisualizer() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void enablementChanged() override;

    void timerCallback() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

private:
    const float WINDOW_LENGTH{ 1.5f };
    const float b{ 5.f };
    const float multiplier{ 20.f };
    const float oscRange{ b * multiplier + PluginParameters::CHORUS_CENTER_DELAY_RANGE.getEnd() };

    APVTS& apvts;
    int sampleRate;

    bool shouldRepaint{ false };
    float rate, depth, centerDelay, feedback, mix;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChorusVisualizer)
};

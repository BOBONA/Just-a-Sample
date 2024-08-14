/*
  ==============================================================================

    ChorusVisualizer.h
    Created: 30 Jan 2024 6:12:49pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "../../PluginParameters.h"
#include "../../Utilities/ComponentUtils.h"

/** A simple chorus visualizer, which displays a sine wave representing the delay line. */
class ChorusVisualizer final : public CustomComponent
{
public:
    ChorusVisualizer(APVTS& apvts, int sampleRate);

private:
    void paint(juce::Graphics&) override;
    void enablementChanged() override;

    //==============================================================================
    static constexpr float WINDOW_LENGTH{ 1.5f };
    static constexpr float b{ 5.f };
    static constexpr float multiplier{ 25.f };
    static constexpr float oscRange{ b * multiplier + PluginParameters::CHORUS_CENTER_DELAY_RANGE.getEnd() };

    //==============================================================================
    APVTS& apvts;
    int sampleRate;

    bool shouldRepaint{ false };
    float rate{ 0.f }, depth{ 0.f }, centerDelay{ 0.f }; /*feedback{ 0.f }, mix{ 0.f }*/

    juce::ParameterAttachment rateAttachment, depthAttachment, centerDelayAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChorusVisualizer)
};

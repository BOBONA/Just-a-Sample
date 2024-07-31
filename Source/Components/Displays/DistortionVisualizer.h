/*
  ==============================================================================

    DistortionVisualizer.h
    Created: 23 Jan 2024 9:12:52pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "../../Sampler/Effects/Distortion.h"
#include "../../Utilities/ComponentUtils.h"

/** Visualizes the distortion effect by displaying its result on a sine wave. */
class DistortionVisualizer final : public CustomComponent, public juce::Timer, public APVTS::Listener
{
public:
    DistortionVisualizer(APVTS& apvts, int sampleRate);
    ~DistortionVisualizer() override;

private:
    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    //==============================================================================
    const int WINDOW_LENGTH{ 30000 };
    const float SINE_HZ{ 13.f };

    //==============================================================================
    juce::AudioBuffer<float> inputBuffer;

    APVTS& apvts;

    float distortionDensity{ 0.f }, distortionHighpass{ 0.f }, distortionMix{ 0.f };
    bool shouldRepaint{ false };
    Distortion distortion;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DistortionVisualizer)
};

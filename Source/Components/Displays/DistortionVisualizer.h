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
#include "../ComponentUtils.h"

class DistortionVisualizer : public CustomComponent, public Timer, public AudioProcessorValueTreeState::Listener
{
public:
    DistortionVisualizer(AudioProcessorValueTreeState& apvts, int sampleRate);
    ~DistortionVisualizer() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void parameterChanged(const String& parameterID, float newValue) override;

private:
    const int WINDOW_LENGTH{ 30000 };
    const float SINE_HZ{ 13.f };

    AudioBuffer<float> inputBuffer;

    AudioProcessorValueTreeState& apvts;

    float distortionDensity{ 0.f }, distortionHighpass{ 0.f }, distortionMix{ 0.f };
    bool shouldRepaint{ false };
    Distortion distortion;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DistortionVisualizer)
};

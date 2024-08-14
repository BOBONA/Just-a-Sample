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
class DistortionVisualizer final : public CustomComponent
{
public:
    DistortionVisualizer(APVTS& apvts, int sampleRate);

private:
    void paint(juce::Graphics&) override;
    void enablementChanged() override;

    //==============================================================================
    static constexpr int WINDOW_LENGTH{ 30000 };
    static constexpr float SINE_HZ{ 13.f };

    //==============================================================================
    APVTS& apvts;

    juce::AudioBuffer<float> inputBuffer;

    Distortion distortion;
    float distortionDensity{ 0.f }, distortionMix{ 0.f }; /*distortionHighpass{ 0.f }*/
    juce::ParameterAttachment densityAttachment, mixAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DistortionVisualizer)
};

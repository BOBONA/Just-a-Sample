/*
  ==============================================================================

    LookAndFeel.h
    Created: 19 Sep 2023 4:48:33pm
    Author:  binya

  ==============================================================================
*/
#pragma once

#include <JuceHeader.h>

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    const juce::Colour BACKGROUND_COLOR = juce::Colours::lightslategrey;
    const juce::Colour WAVEFORM_COLOR = juce::Colours::black;
    const juce::Colour VOICE_POSITION_COLOR = juce::Colours::lightgrey.withAlpha(0.5f);
};
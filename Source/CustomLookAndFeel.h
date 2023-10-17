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
    const juce::Colour SAMPLE_BOUNDS_COLOR = juce::Colours::white;
    const juce::Colour SAMPLE_BOUNDS_SELECTED_COLOR = juce::Colours::lightgrey.withAlpha(0.5f);

    const int DRAGGABLE_SNAP = 5;
    const int NAVIGATOR_BOUNDS_WIDTH = 1;
    const int EDITOR_BOUNDS_WIDTH = 2;

};

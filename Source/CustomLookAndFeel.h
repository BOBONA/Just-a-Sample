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
    // general colors
    const juce::Colour BACKGROUND_COLOR = juce::Colours::lightslategrey;
    // waveforms
    const juce::Colour WAVEFORM_COLOR = juce::Colours::black;
    const juce::Colour PITCH_PROCESSED_WAVEFORM_COLOR = juce::Colours::white.withAlpha(0.5f);
    const float PITCH_PROCESSED_WAVEFORM_THICKNESS = 0.8f;
    // sample editor
    const juce::Colour VOICE_POSITION_COLOR = juce::Colours::lightgrey.withAlpha(0.5f);
    const juce::Colour SAMPLE_BOUNDS_COLOR = juce::Colours::white;
    const juce::Colour SAMPLE_BOUNDS_SELECTED_COLOR = SAMPLE_BOUNDS_COLOR.withAlpha(0.5f);
    const juce::Colour LOOP_ICON_COLOR = juce::Colours::darkgrey;
    const juce::Colour LOOP_BOUNDS_COLOR = juce::Colour::fromRGB(255, 231, 166);
    const juce::Colour LOOP_BOUNDS_SELECTED_COLOR = LOOP_BOUNDS_COLOR.withAlpha(0.5f);

    const int DRAGGABLE_SNAP = 5;
    const int NAVIGATOR_BOUNDS_WIDTH = 1;
    const int EDITOR_BOUNDS_WIDTH = 2;

    const float DEFAULT_LOOP_START_END_PORTION = 0.1f; // for where to place the start or end portion when none valid was found before
};

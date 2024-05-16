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
using Colour = juce::Colour;

public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;

    // general colors
    const Colour BACKGROUND_COLOR = juce::Colours::lightslategrey;
    const Colour DISABLED = juce::Colours::darkgrey;
    const Colour TITLE_TEXT = juce::Colours::white;

    // waveforms
    const Colour WAVEFORM_COLOR = juce::Colours::black;
    const Colour PITCH_PROCESSED_WAVEFORM_COLOR = juce::Colours::white.withAlpha(0.5f);
    const float PITCH_PROCESSED_WAVEFORM_THICKNESS = 0.8f;

    // sample editor
    const Colour VOICE_POSITION_COLOR = juce::Colours::lightgrey.withAlpha(0.5f);
    const Colour SAMPLE_BOUNDS_COLOR = juce::Colours::white;
    const Colour SAMPLE_BOUNDS_SELECTED_COLOR = SAMPLE_BOUNDS_COLOR.withAlpha(0.5f);
    const Colour LOOP_ICON_COLOR = juce::Colours::darkgrey;
    const Colour LOOP_BOUNDS_COLOR = Colour::fromRGB(255, 231, 166);
    const Colour LOOP_BOUNDS_SELECTED_COLOR = LOOP_BOUNDS_COLOR.withAlpha(0.5f);

    const int DRAGGABLE_SNAP = 10;
    const int NAVIGATOR_BOUNDS_WIDTH = 1;
    const int EDITOR_BOUNDS_WIDTH = 2;

    const int MINIMUM_BOUNDS_DISTANCE = 10;  // This needs to be at some minimum value
    const int MINIMUM_VIEW = 6 * MINIMUM_BOUNDS_DISTANCE;  // Minimum view size in samples
    const float DEFAULT_LOOP_START_END_PORTION = 0.1f;  // What percent the loop bounds should be placed before/after the sample bounds
};

/*
  ==============================================================================

    LookAndFeel.h
    Created: 19 Sep 2023 4:48:33pm
    Author:  binya

  ==============================================================================
*/
#pragma once
#include <JuceHeader.h>

using namespace juce;

class CustomLookAndFeel : public LookAndFeel_V4
{
public:
    void drawRotarySlider(Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, Slider& slider) override;

    // general colors
    const Colour BACKGROUND_COLOR = Colours::lightslategrey;
    const Colour DISABLED = Colours::darkgrey;
    const Colour TITLE_TEXT = Colours::white;

    // waveforms
    const Colour WAVEFORM_COLOR = Colours::black;
    const Colour PITCH_PROCESSED_WAVEFORM_COLOR = Colours::white.withAlpha(0.5f);
    const float PITCH_PROCESSED_WAVEFORM_THICKNESS = 0.8f;

    // sample editor
    const Colour VOICE_POSITION_COLOR = Colours::lightgrey.withAlpha(0.5f);
    const Colour SAMPLE_BOUNDS_COLOR = Colours::white;
    const Colour SAMPLE_BOUNDS_SELECTED_COLOR = SAMPLE_BOUNDS_COLOR.withAlpha(0.5f);
    const Colour LOOP_ICON_COLOR = Colours::darkgrey;
    const Colour LOOP_BOUNDS_COLOR = Colour::fromRGB(255, 231, 166);
    const Colour LOOP_BOUNDS_SELECTED_COLOR = LOOP_BOUNDS_COLOR.withAlpha(0.5f);

    const int DRAGGABLE_SNAP = 5;
    const int NAVIGATOR_BOUNDS_WIDTH = 1;
    const int EDITOR_BOUNDS_WIDTH = 2;

    const float DEFAULT_LOOP_START_END_PORTION = 0.1f; // for where to place the start or end portion when none valid was found before
};

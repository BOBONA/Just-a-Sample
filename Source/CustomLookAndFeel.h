/*
  ==============================================================================

    LookAndFeel.h
    Created: 19 Sep 2023 4:48:33pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

/** This struct contains the plugin's colors. */
struct Colors
{
    inline static const juce::Colour BACKGROUND{ 0xFFFDF6C3 };
    inline static const juce::Colour FOREGROUND{ 0xFFEAFFF7 };

    inline static const juce::Colour DARK{ 0xFF171614 };
    inline static const juce::Colour HIGHLIGHT{ 0xFFFF595E };
    inline static const juce::Colour LOOP{ 0xFFFFDA22 };

    inline static const juce::Colour SLATE{ 0xFF6E7894 };
    inline static const juce::Colour DARKER_SLATE{ 0xFF403D37 };
    inline static const juce::Colour WHITE{ 0xFFFFFFFF };
};

/** This struct contains the plugin's layout constants. */
struct Layout
{
    // Toolbar values should all be scaled according to the window width
    static constexpr int figmaWidth{ 1988 };

    static constexpr int controlsHeight{ 159 };
    static constexpr int controlsPaddingX{ 16 };
    static constexpr int moduleGap{ 36 };

    static constexpr int moduleLabelHeight{ 42 };
    static constexpr int moduleLabelPadding{ 3 };
    static constexpr int moduleLabelGap{ 3 };
    static constexpr int moduleControlsGap{ 22 };

    static constexpr int tuningWidth{ 320 };  // As ratios of the window width
    static constexpr int attackWidth{ 204 };
    static constexpr int releaseWidth{ 204 };
    static constexpr int playbackWidth{ 534 };
    static constexpr int loopWidth{ 217 };
    static constexpr int masterWidth{ 295 };

    static constexpr int standardRotarySize{ 87 };  // Not including padding
    static constexpr float rotaryHeightRatio{ 0.965f };  // The height of the rotary slider as a ratio of its width
    static constexpr float rotaryPadding{ 0.09f };
    static constexpr float rotaryTextSize{ 0.40f };
};

//==============================================================================
/** Some utilities */
struct ComponentProps
{
    inline static const juce::String& LABEL_UNIT{ "props_unit" };
    inline static const juce::String& GREATER_UNIT{ "greater_unit" };
    inline static const juce::String& LABEL_ICON{ "label_icon" };
};

class ReferenceCountedPath final : public juce::ReferenceCountedObject
{
public:
    explicit ReferenceCountedPath(juce::Path path) : path(std::move(path)) {}

    juce::Path path;
};

const juce::Font& getInriaSans();
const juce::Font& getInriaSansBold();
const juce::Font& getInterBold();
juce::Path getOutlineFromSVG(const char* data);

//==============================================================================
class CustomLookAndFeel : public juce::LookAndFeel_V4
{
using Colour = juce::Colour;

public:
    juce::Slider::SliderLayout getSliderLayout(juce::Slider& slider) override;
    juce::Label* createSliderTextBox(juce::Slider& slider) override;
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;

    void drawLabel(juce::Graphics&, juce::Label&) override;

    // general colors
    const Colour BACKGROUND_COLOR = juce::Colours::lightslategrey;
    const Colour DISABLED = juce::Colours::darkgrey;
    const Colour TITLE_TEXT = juce::Colours::white;

    // waveforms
    const Colour WAVEFORM_COLOR = juce::Colours::black;
    const Colour PITCH_PROCESSED_WAVEFORM_COLOR = juce::Colours::white.withAlpha(0.5f);
    const float PITCH_PROCESSED_WAVEFORM_THICKNESS = 0.8f;

    // sample editor
    const Colour VOICE_POSITION_COLOR = juce::Colours::lightgrey.withAlpha(0.8f);
    const Colour SAMPLE_BOUNDS_COLOR = juce::Colours::white;
    const Colour SAMPLE_BOUNDS_SELECTED_COLOR = SAMPLE_BOUNDS_COLOR.withAlpha(0.5f);
    const Colour LOOP_ICON_COLOR = juce::Colours::darkgrey;
    const Colour LOOP_BOUNDS_COLOR = Colour::fromRGB(255, 231, 166);
    const Colour LOOP_BOUNDS_SELECTED_COLOR = LOOP_BOUNDS_COLOR.withAlpha(0.5f);

    const int MOUSE_SENSITIVITY = 30;
    const int DRAGGABLE_SNAP = 10;
    const int NAVIGATOR_BOUNDS_WIDTH = 1;
    const int EDITOR_BOUNDS_WIDTH = 2;

    const int MINIMUM_BOUNDS_DISTANCE = 10;  // This needs to be at some minimum value
    const int MINIMUM_VIEW = 6 * MINIMUM_BOUNDS_DISTANCE;  // Minimum view size in samples
    const float DEFAULT_LOOP_START_END_PORTION = 0.1f;  // What percent the loop bounds should be placed before/after the sample bounds
};


/** Here's a custom look and feel for the envelope sliders. A template was not necessary here, but I thought I'd try it. */
enum class EnvelopeSlider
{
    attack,
    release
};

template <EnvelopeSlider Direction>
class EnvelopeSliderLookAndFeel final : public CustomLookAndFeel
{
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;
};

template class EnvelopeSliderLookAndFeel<EnvelopeSlider::attack>;
template class EnvelopeSliderLookAndFeel<EnvelopeSlider::release>;

//==============================================================================
class VolumeSliderLookAndFeel final : public CustomLookAndFeel
{
    juce::Slider::SliderLayout getSliderLayout(juce::Slider& slider) override;
    juce::Label* createSliderTextBox(juce::Slider& slider) override;
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle, juce::Slider&) override;
};
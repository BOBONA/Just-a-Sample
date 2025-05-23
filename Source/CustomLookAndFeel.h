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
    /** Background for the editor */
    juce::Colour background;

    /** Foreground for the controls */
    juce::Colour foreground;

    /** Dark color */
    juce::Colour dark;

    /** Highlight color for controls */
    juce::Colour highlight;

    /** Loop color */
    juce::Colour loop;

    /** Lighter slate for some buttons */
    juce::Colour slate;

    /** Darker slate for waveform and some buttons */
    juce::Colour darkerSlate;

    /** Background neutral color (outside editor) */
    juce::Colour light;

    /** Prompt background color */
    juce::Colour prompt;

    static constexpr int backgroundColorId{ -1 };
    static constexpr int painterColorId{ -2 };
};

static const Colors defaultTheme
{
    .background = juce::Colour{0xFFFDF6C3},
    .foreground = juce::Colour{0xFFEAFFF7},
    .dark = juce::Colour{0xFF171614},
    .highlight = juce::Colour{0xFFFF595E},
    .loop = juce::Colour{0xFFFFDA22},
    .slate = juce::Colour{0xFF6E7894},
    .darkerSlate = juce::Colour{0xFF403D37},
    .light = juce::Colour{0xFFFFFFFF},
    .prompt = juce::Colour{0xFF171614}.withAlpha(0.3f)
};

static const Colors darkTheme
{
    .background = juce::Colour{0xFF282625},
    .foreground = juce::Colour{0xFF1D1C1A},
    .dark = juce::Colour{0xFFE8DCCB},
    .highlight = juce::Colour{0xFFB65050},
    .loop = juce::Colour{0xFFB89C2F},
    .slate = juce::Colour{0xFF596B9C},
    .darkerSlate = juce::Colour{0xFFE8DCCB},
    .light = juce::Colour{0xFF171614},
    .prompt = juce::Colour{0xFF171614}.withAlpha(0.5f)
};

/** This struct contains layout constants. */
struct Layout
{
    // Toolbar values should all be scaled according to the window width
    static constexpr int figmaWidth{ 1999 };

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

    static constexpr float boundsWidth{ 0.0045f };
    static constexpr float handleWidth{ boundsWidth / 2.f };
    static constexpr float playheadWidth{ handleWidth * 0.66f };

    static constexpr juce::Point<int> sampleControlsMargin{ 46, 25 };
    static constexpr int sampleControlsHeight{ 45 };
    static constexpr int fileControlsWidth{ 1179 };
    static constexpr int waveformModeWidth{ 238 };
    static constexpr int playbackControlsWidth{ 137 };
    static constexpr float expandWidth{ 1.185f };  // Ratio of combobox height

    static constexpr int sampleNavigatorHeight{ 93 };
    static constexpr float navigatorBoundsWidth{ 0.002f };
    static constexpr juce::Point<int> navigatorControlsSize{ 124, 52 };

    static constexpr int fxChainHeight{ 384 };
    static constexpr float fxChainDivider{ 0.001f };
    static constexpr float fxModuleHeader{ 73.f };
    static constexpr float fxDisplayStrokeWidth{ 0.004f };

    static constexpr int footerHeight{ 64 };
};

/** This struct contains certain UX constants */
struct Feel
{
    static constexpr int MOUSE_SENSITIVITY{ 60 };
    static constexpr int DRAGGABLE_SNAP{ 10 };

    static constexpr int MINIMUM_BOUNDS_DISTANCE{ 10 };  // This needs to be at some minimum value
    static constexpr int MINIMUM_VIEW{ 6 * MINIMUM_BOUNDS_DISTANCE };  // Minimum view size in samples
};

//==============================================================================
/** Some utilities */
struct ComponentProps
{
    inline static const juce::String& ROTARY_UNIT{ "props_unit" };  // The rotary unit
    inline static const juce::String& ROTARY_GREATER_UNIT{ "greater_unit" };  // A larger unit (e.g. s instead of ms)
    inline static const juce::String& ROTARY_ICON{ "label_icon" };  // An icon to display in place of the unit

    inline static const juce::String& ROTARY_PARAMETER_NAME{ "parameter_name" };  // For use in a custom rotaries help text

    inline static const juce::String& LABEL_ELLIPSES{ "label_ellipses" };  // Use ellipses for the label
};

class ReferenceCountedPath final : public juce::ReferenceCountedObject
{
public:
    explicit ReferenceCountedPath(juce::Path path) : path(std::move(path)) {}

    juce::Path path;
};

const juce::Font& getInriaSans();
const juce::Font& getInriaSansBold();
const juce::Font& getInter();
const juce::Font& getInterBold();
juce::Path getOutlineFromSVG(const char* data);

//==============================================================================
class CustomLookAndFeel : public juce::LookAndFeel_V4
{
using Colour = juce::Colour;

public:
    CustomLookAndFeel();
    void setTheme(const Colors& theme);

    juce::Slider::SliderLayout getSliderLayout(juce::Slider& slider) override;
    juce::Label* createSliderTextBox(juce::Slider& slider) override;
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;

    void drawLabel(juce::Graphics&, juce::Label&) override;

    juce::Button* createFilenameComponentBrowseButton(const juce::String& text) override;
    void layoutFilenameComponent(juce::FilenameComponent&, juce::ComboBox* filenameBox, juce::Button* browseButton) override;
    
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    juce::Label* createComboBoxTextBox(juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label& label) override;
    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    void drawComboBoxTextWhenNothingSelected(juce::Graphics&, juce::ComboBox&, juce::Label&) override;
    juce::PopupMenu::Options getOptionsForComboBoxPopupMenu(juce::ComboBox&, juce::Label&) override;
    
    juce::Font getPopupMenuFont() override;
    void drawPopupMenuBackground(juce::Graphics&, int width, int height) override;
    void drawPopupMenuItem(juce::Graphics&, const juce::Rectangle<int>& area, bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text, const juce::String& shortcutKeyText, const juce::Drawable* icon, const Colour* textColour) override;
    void drawPopupMenuUpDownArrow(juce::Graphics&, int width, int height, bool isScrollUpArrow) override;
    juce::Path getTickShape(float height) override;
    
    void drawTickBox(juce::Graphics&, juce::Component&, float x, float y, float w, float h, bool ticked, bool isEnabled, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    juce::Font getAlertWindowTitleFont() override { return getInterBold().withHeight(18.f); }
    juce::Font getAlertWindowMessageFont() override { return getInter().withHeight(16.f); }
    juce::Font getAlertWindowFont() override { return getInter().withHeight(14.f); }

    void drawCornerResizer(juce::Graphics&, int w, int h, bool isMouseOver, bool isMouseDragging) override;

    Colors colors;
};

//==============================================================================
/** Here's a custom look and feel for the envelope sliders. A template was not the only way to do this, but I thought I'd try it. */
enum class EnvelopeSlider : std::uint8_t
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
/** Here's a custom look and feel for the volume slider. */
class VolumeSliderLookAndFeel final : public CustomLookAndFeel
{
    juce::Slider::SliderLayout getSliderLayout(juce::Slider& slider) override;
    juce::Label* createSliderTextBox(juce::Slider& slider) override;
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle, juce::Slider&) override;
};

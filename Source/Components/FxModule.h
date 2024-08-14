/*
  ==============================================================================

    FXModule.h
    Created: 28 Dec 2023 8:09:18pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "FxDragTarget.h"
#include "../PluginParameters.h"
#include "../Utilities/ComponentUtils.h"

/** An FX module within the FX chain. */
class FxModule final : public CustomComponent
{
public:
    FxModule(FxDragTarget* fxChain, APVTS& apvts, const juce::String& fxName, PluginParameters::FxTypes effectType, const juce::String& fxEnabledParameter, Component& displayComponent);

    /** Constructor for a module with a mix control. */
    FxModule(FxDragTarget* fxChain, APVTS& apvts, const juce::String& fxName, PluginParameters::FxTypes effectType, const juce::String& fxEnabledParameter, const juce::String& mixControlParameter, Component& displayComponent);

    juce::Slider* addRotary(const juce::String& parameter, const juce::String& label, juce::Point<float> position, float width, const juce::String& unit = "");

    PluginParameters::FxTypes getEffectType() const { return fxType; }

private:
    void paint(juce::Graphics&) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

    void setupRotary(juce::Slider& rotary, bool useTextbox = true);

    int scale(float value) const { return int(std::round(value * getWidth() / (Layout::figmaWidth / 4.f))); }
    float scalef(float value) const { return value * getWidth() / (Layout::figmaWidth / 4.f); }

    //==============================================================================
    FxDragTarget* fxChain;
    APVTS& apvts;
    PluginParameters::FxTypes fxType;

    // Header controls
    juce::Label nameLabel;

    juce::Slider mixControl;
    std::unique_ptr<APVTS::SliderAttachment> mixControlAttachment;

    juce::ToggleButton fxEnabled;
    APVTS::ButtonAttachment enablementAttachment;

    // Controls
    Component& display;

    std::vector<std::unique_ptr<juce::Label>> labels;
    std::vector<std::unique_ptr<juce::Slider>> rotaries;
    std::vector<std::unique_ptr<APVTS::SliderAttachment>> attachments;
    std::vector<juce::Point<float>> rotaryPositions;  // Relative to the area
    std::vector<float> rotaryWidths;

    std::vector<juce::Slider*> rotaryReferences;

    bool dragging{ false };
    juce::Path dragIcon;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxModule)
};

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
#include "ComponentUtils.h"

using APVTS = juce::AudioProcessorValueTreeState;

/** Contains the relevant information for a control in an FX module */
struct ModuleControl
{
    enum Type
    {
        ROTARY
    };

    ModuleControl(const juce::String& label, const juce::String& id, Type type=ROTARY) : label(label), id(id), type(type)
    {
    }

    const juce::String& label;
    const juce::String& id;
    Type type;
};

using AttachmentVariant = std::variant<std::unique_ptr<APVTS::SliderAttachment>, std::unique_ptr<APVTS::ButtonAttachment>, std::unique_ptr<APVTS::ComboBoxAttachment>>;

/** A custom component that represents an FX module in the FX chain. Saving layout effort */
class FxModule final : public CustomComponent
{
public:
    FxModule(FxDragTarget* fxChain, APVTS& apvts, const juce::String& fxName, const juce::String& fxEnabledID);

    /** Constructor for a module with a mix control. */
    FxModule(FxDragTarget* fxChain, APVTS& apvts, const juce::String& fxName, const juce::String& fxEnabledID, const juce::String& mixControlID);

    ~FxModule() override = default;

    //==============================================================================
    /** Adds a row of controls to the FX module. */
    void addRow(const juce::Array<ModuleControl>& row);

    /** Sets the module's display component, and the height of the display component. */
    void setDisplayComponent(Component* displayComp, float compHeight = 60.f);

private:
    void paint(juce::Graphics&) override;
    void resized() override;

    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

    //==============================================================================
    FxDragTarget* fxChain;
    APVTS& apvts;

    // Module's name and enablement toggle
    juce::Label nameLabel;
    juce::ToggleButton fxEnabled;
    APVTS::ButtonAttachment enablementAttachment;

    bool useMixControl{ false };
    juce::Slider mixControl;
    std::unique_ptr<APVTS::SliderAttachment> mixControlAttachment;
    Component* displayComponent{ nullptr };
    float displayHeight{ 0.f };

    // Controls
    juce::Array<std::unique_ptr<juce::Array<ModuleControl>>> rows;
    std::unordered_map<juce::String, std::unique_ptr<Component>> controls;
    juce::Array<std::unique_ptr<APVTS::SliderAttachment>> attachments;

    const int DRAG_AREA{ 15 };
    bool mouseOver{ false };
    bool dragging{ false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxModule)
};

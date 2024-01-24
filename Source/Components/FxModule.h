/*
  ==============================================================================

    FXModule.h
    Created: 28 Dec 2023 8:09:18pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "FxDragger.h"
#include "ComponentUtils.h"


using namespace juce;

using APVTS = AudioProcessorValueTreeState;

struct ModuleControl
{
    enum Type
    {
        ROTARY
    };

    ModuleControl(const String& label, const String& id, Type type=ROTARY) : label(label), id(id), type(type)
    {
    }

    const String& label;
    const String& id;
    Type type;
};

using AttachmentVariant = std::variant<std::unique_ptr<APVTS::SliderAttachment>, std::unique_ptr<APVTS::ButtonAttachment>, std::unique_ptr<APVTS::ComboBoxAttachment>>;

class FxModule : public CustomComponent
{
public:
    FxModule(FxDragger* fxChain, AudioProcessorValueTreeState& apvts, const String& fxName, const String& fxEnabledID);
    FxModule(FxDragger* fxChain, AudioProcessorValueTreeState& apvts, const String& fxName, const String& fxEnabledID, const String& mixControlID);
    ~FxModule() override;

    void paint (Graphics&) override;
    void resized() override;

    void mouseEnter(const MouseEvent& event) override;
    void mouseExit(const MouseEvent& event) override;
    void mouseUp(const MouseEvent& event) override;
    void mouseDrag(const MouseEvent& event) override;

    void addRow(Array<ModuleControl> row);
    void setDisplayComponent(Component* displayComp, float compHeight = 60.f);

private:
    FxDragger* fxChain;
    AudioProcessorValueTreeState& apvts;

    Label nameLabel;
    ToggleButton fxEnabled;
    APVTS::ButtonAttachment enablementAttachment;

    bool useMixControl{ false };
    Slider mixControl;
    std::unique_ptr<APVTS::SliderAttachment> mixControlAttachment;
    Component* displayComponent{ nullptr };
    float displayHeight{ 0.f };

    Array<std::unique_ptr<Array<ModuleControl>>> rows;
    std::unordered_map<String, std::unique_ptr<Component>> controls;
    Array<std::unique_ptr<APVTS::SliderAttachment>> attachments;

    const int DRAG_AREA{ 15 };
    bool mouseOver{ false };
    bool dragging{ false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxModule)
};

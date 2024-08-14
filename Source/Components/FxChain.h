/*
  ==============================================================================

    FxChain.h
    Created: 5 Jan 2024 3:26:37pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "../PluginProcessor.h"
#include "../PluginParameters.h"
#include "../Utilities/ComponentUtils.h"
#include "FxModule.h"
#include "FxDragTarget.h"
#include "Displays/FilterResponse.h"
#include "Displays/ReverbResponse.h"
#include "Displays/DistortionVisualizer.h"
#include "Displays/ChorusVisualizer.h"

class FxChainShadows final : public CustomComponent
{
    void resized() override;
    void paint(juce::Graphics& g) override;

    melatonin::InnerShadow innerShadow{ {Colors::SLATE.withAlpha(0.25f), 3, {0, 2}}, {Colors::SLATE.withAlpha(0.25f), 3, {0, -2}} };
    melatonin::DropShadow dragShadow{ Colors::SLATE.withAlpha(0.125f), 3, {2, 2} };
};

/** The FX chain allows for drag and drop reordering of the effects modules. */
class FxChain final : public CustomComponent, public FxDragTarget
{
public:
    explicit FxChain(JustaSampleAudioProcessor& processor);
    ~FxChain() override = default;

    bool isChainDragging() const { return dragging; }
    Component* getDragComp() const { return dragComp; }

private:
    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDrag(const juce::MouseEvent& event) override;
    void dragStarted(juce::Component* component, const juce::MouseEvent& event) override;
    void dragEnded() override;

    FxModule& getModule(PluginParameters::FxTypes type);

    //==============================================================================
    FilterResponse eqDisplay;
    ReverbResponse reverbDisplay;
    DistortionVisualizer distortionDisplay;
    ChorusVisualizer chorusDisplay;

    FxModule reverbModule, distortionModule, eqModule, chorusModule;

    FxChainShadows shadows;

    // Dragging functionality
    juce::ParameterAttachment fxPermAttachment;
    int oldVal{ 0 };
    std::array<PluginParameters::FxTypes, 4> moduleOrder{};

    bool dragging{ false };
    PluginParameters::FxTypes dragTarget{ PluginParameters::REVERB };
    Component* dragComp{ nullptr };
    int dragCompIndex{ 0 };
    int mouseX{ 0 };
    int dragOffset{ 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxChain)
};

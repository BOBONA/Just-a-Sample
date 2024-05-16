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
#include "ComponentUtils.h"
#include "FxModule.h"
#include "FxDragTarget.h"
#include "displays/FilterResponse.h"
#include "displays/ReverbResponse.h"
#include "displays/DistortionVisualizer.h"
#include "displays/ChorusVisualizer.h"

/** The FX chain allows for drag and drop reordering of the effects modules. */
class FxChain final : public CustomComponent, public FxDragTarget
{
public:
    FxChain(JustaSampleAudioProcessor& processor);
    ~FxChain() override = default;

private:
    void paint(Graphics&) override;
    void resized() override;

    void mouseDrag(const MouseEvent& event) override;
    void dragStarted(const String& moduleName, const MouseEvent& event) override;
    void dragEnded() override;

    FxModule& getModule(PluginParameters::FxTypes type);

    //==============================================================================
    FxModule reverbModule, distortionModule, eqModule, chorusModule;

    FilterResponse eqDisplay;
    ReverbResponse reverbDisplay;
    DistortionVisualizer distortionDisplay;
    ChorusVisualizer chorusDisplay;

    // Dragging functionality
    ParameterAttachment fxPermAttachment;
    int oldVal{ 0 };
    std::array<PluginParameters::FxTypes, 4> moduleOrder{};

    bool dragging{ false };
    PluginParameters::FxTypes dragTarget{ PluginParameters::REVERB };
    Component* dragComp{ nullptr };
    int dragCompIndex{ 0 };
    Rectangle<int> targetArea;
    int mouseX{ 0 };
    int dragOffset{ 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxChain)
};

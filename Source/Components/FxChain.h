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
#include "FxDragger.h"
#include "displays/FilterResponse.h"
#include "displays/ReverbResponse.h"
#include "displays/DistortionVisualizer.h"

class FxChain : public CustomComponent, public FxDragger
{
public:
    FxChain(JustaSampleAudioProcessor& processor);
    ~FxChain() override;

    void paint(Graphics&) override;
    void resized() override;

    void mouseDrag(const MouseEvent& event) override;
    void dragStarted(const String& moduleName, const MouseEvent& event) override;
    void dragEnded() override;

    FxModule& getModule(PluginParameters::FxTypes type);

private:
    FxModule reverbModule, distortionModule, eqModule, chorusModule;
    FilterResponse eqDisplay;
    ReverbResponse reverbDisplay;
    DistortionVisualizer distortionDisplay;

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

/*
  ==============================================================================

    CustomComponent.h
    Created: 26 Sep 2023 11:49:53pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "CustomLookAndFeel.h"
#include "PluginParameters.h"

using APVTS = juce::AudioProcessorValueTreeState;

class CustomComponent : public juce::Component
{
public:
    CustomComponent() : lnf(dynamic_cast<CustomLookAndFeel&>(getLookAndFeel()))
    {

    }

    juce::Colour disabled(juce::Colour colour) const
    {
        return isEnabled() ? colour : lnf.DISABLED.withBrightness((lnf.DISABLED.getBrightness() + colour.getPerceivedBrightness()) / 2);
    }

    CustomLookAndFeel& lnf;
};
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

class CustomComponent : public juce::Component
{
public:
    CustomComponent() : lnf(dynamic_cast<CustomLookAndFeel&>(getLookAndFeel()))
    {

    }

    CustomLookAndFeel& lnf;
};
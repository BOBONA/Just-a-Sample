/*
  ==============================================================================

    SampleLoaderArea.h
    Created: 14 Jun 2024 7:41:32pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "ComponentUtils.h"

class SampleLoaderArea final : public CustomComponent
{
public:
    SampleLoaderArea() : promptLabel("prompt_label", "Drop a sample!")
    {
        promptLabel.setColour(juce::Label::textColourId, lnf.TITLE_TEXT);
        promptLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(promptLabel);
    }

    ~SampleLoaderArea() override = default;

private:
    void paint (juce::Graphics& g) override {}

    void resized() override
    {
        auto bounds = getLocalBounds();

        promptLabel.setBounds(bounds);
    }

    //==============================================================================
    juce::Label promptLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleLoaderArea)
};

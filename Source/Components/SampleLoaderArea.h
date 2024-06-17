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
    SampleLoaderArea() : promptLabel("prompt_label", dropText)
    {
        promptLabel.setColour(juce::Label::textColourId, lnf.TITLE_TEXT);
        promptLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(promptLabel);
    }

    ~SampleLoaderArea() override = default;

    void setLoading(bool isLoading)
    {
        if (isLoading != loading)
        {
            promptLabel.setText(isLoading ? loadingText : dropText, juce::dontSendNotification);
        }
        loading = isLoading;
    }

    bool isLoading() const { return loading; }

private:
    void paint (juce::Graphics& g) override {}

    void resized() override
    {
        auto bounds = getLocalBounds();

        promptLabel.setBounds(bounds);
    }

    //==============================================================================
    const juce::String& dropText{ "Drop a Sample!" };
    const juce::String& loadingText{ "Loading..." };

    juce::Label promptLabel;

    bool loading{ false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleLoaderArea)
};

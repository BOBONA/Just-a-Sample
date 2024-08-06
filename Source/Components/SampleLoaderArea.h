/*
  ==============================================================================

    SampleLoaderArea.h
    Created: 14 Jun 2024 7:41:32pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "../Utilities/ComponentUtils.h"

class SampleLoaderArea final : public CustomComponent
{
public:
    SampleLoaderArea() : promptLabel("prompt_label", loadingText)
    {
        promptLabel.setColour(juce::Label::textColourId, Colors::DARK);
        promptLabel.setColour(juce::Label::backgroundColourId, Colors::BACKGROUND.withAlpha(0.85f));
        promptLabel.setJustificationType(juce::Justification::centred);

        setInterceptsMouseClicks(false, false);

        addAndMakeVisible(promptLabel);
    }

    ~SampleLoaderArea() override = default;

    void setLoading(bool isLoading)
    {
        if (isLoading != loading)
        {
            promptLabel.setText(isLoading ? loadingText : dropText, juce::dontSendNotification);
            updateBounds();
        }
        loading = isLoading;
    }

    bool isLoading() const { return loading; }

private:
    void paint (juce::Graphics& g) override {}

    void resized() override
    {
        auto bounds = getLocalBounds().toFloat();
        auto fontSize = bounds.getWidth() * 0.025f;

        promptLabel.setFont(getInter().withHeight(fontSize));
        updateBounds();
    }

    void updateBounds()
    {
        auto bounds = getLocalBounds();
        
        auto width = promptLabel.getFont().getStringWidthFloat(promptLabel.getText());
        auto height = promptLabel.getFont().getHeight();
        int padding = int(std::ceil(bounds.getWidth() * 0.02f));

        promptLabel.setBounds(juce::Rectangle(0, 0, int(width), int(height)).withCentre(getLocalBounds().getCentre()).expanded(padding).toNearestInt());
    }

    //==============================================================================
    const juce::String& dropText{ "Drop a Sample!" };
    const juce::String& loadingText{ "Loading..." };

    juce::Label promptLabel;

    bool loading{ true };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleLoaderArea)
};

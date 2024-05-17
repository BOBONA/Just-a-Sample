/*
  ==============================================================================

    RangeSelection.h
    Created: 16 May 2024 8:28:47pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "ComponentUtils.h"

/** A component that allows the user to select a range of a display, such as a sample or waveform. */
class RangeSelector final : public CustomComponent
{
public:
    RangeSelector()
    {
        label.setAlwaysOnTop(true);
        label.setColour(juce::Label::ColourIds::textColourId, juce::Colours::white);
        label.setJustificationType(juce::Justification::centred);
        label.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(&label);

        cancelRangeSelect();
    }

    void promptRangeSelect(const juce::String& displayText, const std::function<void(int startPos, int endPos)>& onCloseCallback = {})
    {
        isSelecting = true;
        draggingStarted = false;
        label.setText(displayText, juce::NotificationType::dontSendNotification);
        onSelectedCallback = onCloseCallback;
        setVisible(true);
        setInterceptsMouseClicks(true, false);
        resized();
    }

    void cancelRangeSelect()
    {
        isSelecting = false;
        setVisible(false);
        setInterceptsMouseClicks(false, false);
        resized();
    }

    bool isSelectingRange() const
    {
        return isSelecting;
    }

private:
    void paint (juce::Graphics& g) override
    {
        if (isSelecting && draggingStarted)
        {
            g.setColour(lnf.SAMPLE_BOUNDS_SELECTED_COLOR);
            int x = juce::jmin(startLoc, endLoc);
            int width = juce::jmax(startLoc, endLoc) - x;
            g.fillRect(x, 0, width, getHeight());
        }
    }

    void resized() override
    {
        label.setVisible(isSelecting);
        label.setBounds(getLocalBounds());
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        if (isSelecting && !draggingStarted)
        {
            draggingStarted = true;
            startLoc = juce::jlimit<int>(getX(), getRight(), event.x);
        }
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (isSelecting && draggingStarted)
        {
            endLoc = juce::jlimit<int>(getX(), getRight(), event.x);
            repaint();
        }
    }

    void mouseUp(const juce::MouseEvent& event) override
    {
        if (!getLocalBounds().contains(event.getPosition()))
            return;
        if (isSelecting && draggingStarted)
        {
            endLoc = juce::jlimit<int>(getX(), getRight(), event.x);

            int leftLoc = juce::jmin(startLoc, endLoc);
            int rightLoc = juce::jmax(startLoc, endLoc);
            cancelRangeSelect();  // To hide the component
            onSelectedCallback(leftLoc, rightLoc);
        }
    }

    //==============================================================================
    juce::Label label;

    bool isSelecting{ false };
    bool draggingStarted{ false };
    int startLoc{ 0 }, endLoc{ 0 };
    std::function<void(int, int)> onSelectedCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RangeSelector)
};

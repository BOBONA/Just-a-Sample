/*
  ==============================================================================

    RangeSelection.h
    Created: 16 May 2024 8:28:47pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "../Utilities/ComponentUtils.h"

/** A component that allows the user to select a range of a display */
class RangeSelector final : public CustomComponent
{
public:
    RangeSelector()
    {
        cancelRangeSelect();
    }

    /** Begin the range selection prompt */
    void promptRangeSelect(const std::function<void(int startPos, int endPos)>& onCloseCallback = {})
    {
        isSelecting = true;
        draggingStarted = false;
        onSelectedCallback = onCloseCallback;

        setVisible(true);
        setInterceptsMouseClicks(true, false);
        resized();
    }

    /** Cancel the prompt */
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
    void paint(juce::Graphics& g) override
    {
        if (!isSelecting)
            return;

        if (draggingStarted)
        {
            int x = juce::jmin(startLoc, endLoc);
            int width = juce::jmax(startLoc, endLoc) - x;

            g.setColour(Colors::SLATE.withAlpha(0.15f));
            g.fillRect(x, 0, width, getHeight());

            g.setColour(Colors::SLATE);
            g.fillRect(float(startLoc), 0.f, getWidth() * Layout::boundsWidth / 2.f, float(getHeight()));
            g.fillRect(float(endLoc), 0.f, getWidth() * Layout::boundsWidth / 2.f, float(getHeight()));
        }
        else
        {
            int mousePos = getMouseXYRelative().getX();

            g.setColour(Colors::SLATE);
            g.fillRect(float(mousePos), 0.f, getWidth() * Layout::boundsWidth / 2.f, float(getHeight()));
        }
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        if (isSelecting && !draggingStarted)
        {
            draggingStarted = true;
            startLoc = endLoc = juce::jlimit<int>(getX(), getRight(), event.x);
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
        if (isSelecting && draggingStarted)
        {
            endLoc = juce::jlimit<int>(getX(), getRight(), event.x);

            int leftLoc = juce::jmin(startLoc, endLoc);
            int rightLoc = juce::jmax(startLoc, endLoc);

            cancelRangeSelect();  // To hide the component
            onSelectedCallback(leftLoc, rightLoc);
        }
    }

    void mouseMove(const juce::MouseEvent&) override
    {
        repaint();
    }

    //==============================================================================
    bool isSelecting{ false };
    bool draggingStarted{ false };
    int startLoc{ 0 }, endLoc{ 0 };
    std::function<void(int, int)> onSelectedCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RangeSelector)
};

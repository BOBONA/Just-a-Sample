/*
  ==============================================================================

    Prompt.h
    Created: 15 May 2024 5:09:33pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "ComponentUtils.h"

/** A prompt that can be opened and closed with a set of components made visible and invisible,
    an onClose callback, and a set of components to be placed in front of the prompt background.
    This neatly abstracts away a common UI pattern.
*/
class Prompt : public CustomComponent
{
public:
    Prompt()
    {
        closePrompt();
    }

    ~Prompt() override = default;

    /** Opens a prompt with showComponents made visible with the prompt and highlightComponents placed in front 
        of the prompt background (although their visibility is never changed). onClose is called when the prompt is closed.
    */
    void openPrompt(const juce::Array<juce::Component*>& showComponents, const std::function<void()>& onClose = {}, const juce::Array<juce::Component*>& highlightComponents = {})
    {
        if (visible)
            closePrompt();

        onCloseCallback = onClose;
        shownComponents = showComponents;
        highlightedComponents = highlightComponents;

        for (auto* component : shownComponents)
            component->setVisible(true);

        toFront(true);
        for (auto* component : highlightedComponents)
            component->toFront(true);

        setVisible(true);
        setInterceptsMouseClicks(true, true);
        setWantsKeyboardFocus(true);
        repaint();
        visible = true;
    }

    /** Closes the prompt */
    void closePrompt()
    {
        for (auto* component : shownComponents)
            component->setVisible(false);

        setVisible(false);
        setInterceptsMouseClicks(false, false);
        setWantsKeyboardFocus(false);
        visible = false;

        if (onCloseCallback)
            onCloseCallback();
    }

    /** Returns whether the prompt is visible */
    bool isPromptVisible() const
    {
        return visible;
    }

private:
    void paint(juce::Graphics& g) override
    {
        if (visible)
            g.fillAll(juce::Colours::black.withAlpha(0.35f));
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (visible)
        {
            if (key == juce::KeyPress::escapeKey || key == juce::KeyPress::spaceKey)
            {
                closePrompt();
                return true;
            }
        }

        return false;
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        if (visible)
            closePrompt();
    }

    bool visible{ false };

    juce::Array<juce::Component*> shownComponents;
    juce::Array<juce::Component*> highlightedComponents;
    std::function<void()> onCloseCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Prompt)
};

/*
  ==============================================================================

    Prompt.h
    Created: 15 May 2024 5:09:33pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "../Utilities/ComponentUtils.h"

/** A prompt that can be opened and closed with a set of components to be kept "active" */
class Prompt final : public CustomComponent
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
    void openPrompt(const juce::Array<Component*>& visibleComponents, const std::function<void()>& onClose = []() -> void {})
    {
        if (visible)
            closePrompt();

        onCloseCallback = onClose;
        shownComponents = visibleComponents;

        setVisible(true);
        setInterceptsMouseClicks(true, true);
        repaint();
        visible = true;
    }

    /** Closes the prompt */
    void closePrompt()
    {
        setVisible(false);
        setInterceptsMouseClicks(false, false);
        setWantsKeyboardFocus(false);
        visible = false;

        if (onCloseCallback)
            onCloseCallback();
    }

    /** Returns whether the prompt is visible */
    bool isPromptVisible() const { return visible; }

private:
    /** Paint in all but the shown components */
    void paint(juce::Graphics& g) override
    {
        if (!visible)
            return;

        for (auto* component : shownComponents)
            g.excludeClipRegion(component->getBounds().translated(-getX(), -getY()));

        g.fillAll(Colors::DARK.withAlpha(0.3f));
    }

    /** Close the prompt when escape or space is pressed */
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

    bool hitTest(int x, int y) override
    {
        return visible && !std::ranges::any_of(shownComponents, [this, x, y](const Component* component) {
            return component->getBounds().translated(-getX(), -getY()).contains(x, y);
        });
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        if (visible)
            closePrompt();
    }

    //==============================================================================
    bool visible{ false };

    juce::Array<juce::Component*> shownComponents;
    std::function<void()> onCloseCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Prompt)
};

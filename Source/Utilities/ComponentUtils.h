/*
  ==============================================================================

    CustomComponent.h
    Created: 26 Sep 2023 11:49:53pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "../CustomLookAndFeel.h"

// I think I can justify using this one globally. Note that using an entire namespace globally is generally frowned upon.
using APVTS = juce::AudioProcessorValueTreeState;

/** This class is for components which hold a custom help text display.
    The idea is for the Editor to react to mouse move and drag events on all child components. In addition, children can send
    help text changed updates to the Editor when necessary.
 */
class CustomHelpTextDisplay
{
public:
    virtual ~CustomHelpTextDisplay() = default;
    virtual void helpTextChanged(const juce::String& newText) = 0;
};

/** JUCE's default help text functionality is not adequate for the help text label I want to display.
    This class helps add custom help text functionality to a component. 
 */
class CustomHelpTextProvider
{
public:
    explicit CustomHelpTextProvider(juce::Component* associatedComponent, CustomHelpTextDisplay* helpTextDisplay = nullptr) :
        component(associatedComponent),
        textDisplay(helpTextDisplay) {}

    virtual ~CustomHelpTextProvider() = default;

    virtual juce::String getCustomHelpText()
    {
        return component->getHelpText();
    }

    virtual void setCustomHelpText(const juce::String& helpText)
    {
        bool changed = helpText != component->getHelpText();
        component->setHelpText(helpText);
        if (changed)
            sendHelpTextUpdate();
    }

    void sendHelpTextUpdate(bool checkMouseOver = true)
    {
        if ((component->isMouseOverOrDragging() || !checkMouseOver) && textDisplay)
            textDisplay->helpTextChanged(getCustomHelpText());
    }

private:
    juce::Component* component{ nullptr };
    CustomHelpTextDisplay* textDisplay{ nullptr };
};

/** This is a way to store the theme in the top level component of our plugin */
class ThemeProvider
{
public:
    virtual ~ThemeProvider() = default;
    virtual Colors getTheme() const = 0;
    virtual void setTheme(Colors newTheme) = 0;
};

/** It's nice to have my own base class when I need to add broader functionality. */
class CustomComponent : public virtual juce::Component, public CustomHelpTextProvider
{
public:
    explicit CustomComponent(CustomHelpTextDisplay* helpTextDisplay = nullptr) : CustomHelpTextProvider(this, helpTextDisplay), lnf(dynamic_cast<CustomLookAndFeel&>(getLookAndFeel()))
    {
    }

    /** Easy way to get a disabled version of a color */
    juce::Colour disabled(juce::Colour color, bool disabledCondition) const
    {
        return disabledCondition ? color.withMultipliedAlpha(0.5f) : color;
    }

    /** Easy way to get a disabled version of a color */
    juce::Colour disabled(juce::Colour color) const
    {
        return disabled(color, !isEnabled());
    }
    
    /** In situations where a callback can be called on the wrong thread, component methods cannot be safely used. This method wraps repaint in
        an async call with a SafePointer  to make sure nothing bad happens.
     */
    void safeRepaint()
    {
        juce::MessageManager::callAsync([p = juce::Component::SafePointer(this)] {
            if (p.getComponent())
                p->repaint();
        });
    }

    Colors getTheme()
    {
        if (themeProvider)
            return themeProvider->getTheme();

        juce::Component* c{ this };
        while (c != nullptr)
        {
            if (auto* tp = dynamic_cast<ThemeProvider*>(c))
            {
                themeProvider = tp;
                break;
            }

            c = c->getParentComponent();
        }

        return themeProvider ? themeProvider->getTheme() : defaultTheme;
    }

    CustomLookAndFeel& lnf;

private:
    ThemeProvider* themeProvider{ nullptr };
};

/** Simple mixin class to set a mouse cursor on a component when enabled only. */
class EnabledMouseCursor
{
public:
    void setEnabledMouseCursor(const juce::MouseCursor& cursor, const bool use = true)
    {
        enabledMouseCursor = cursor;
        useEnabledCursor = use;
    }

    void enablementChanged(juce::Component& component) const
    {
        if (useEnabledCursor)
        {
            if (component.isEnabled())
                component.setMouseCursor(enabledMouseCursor);
            else
                component.setMouseCursor(juce::MouseCursor::NormalCursor);
        }
    }

private:
    juce::MouseCursor enabledMouseCursor{ juce::MouseCursor::NoCursor };
    bool useEnabledCursor{ false };
};

/** This is a rotary slider that includes custom help text functionality. */
class CustomRotary final : public juce::Slider, public CustomHelpTextProvider
{
public:
    explicit CustomRotary(SliderStyle style = LinearHorizontal, TextEntryBoxPosition textBoxPosition = NoTextBox) : Slider(style, textBoxPosition), CustomHelpTextProvider(this) {}

    juce::String getCustomHelpText() override
    {
        auto numString = getTextFromValue(getValue());
        if (getProperties().contains(ComponentProps::ROTARY_GREATER_UNIT) && getValue() >= 1000.)
            numString += " " + getProperties()[ComponentProps::ROTARY_GREATER_UNIT].toString();
        else if (getProperties().contains(ComponentProps::ROTARY_UNIT))
            numString += " " + getProperties()[ComponentProps::ROTARY_UNIT].toString();

        if (getProperties().contains(ComponentProps::ROTARY_PARAMETER_NAME))
            numString = getProperties()[ComponentProps::ROTARY_PARAMETER_NAME].toString() + ": " + numString;
        
        return numString;
    }
};

/** Helper class to expose the parameter name to the attached rotary */
class CustomRotaryAttachment final : public APVTS::SliderAttachment
{
public:
    CustomRotaryAttachment(APVTS& apvts, const juce::String& parameterID, juce::Slider& rotary) : SliderAttachment(apvts, parameterID, rotary)
    {
        rotary.getProperties().set(ComponentProps::ROTARY_PARAMETER_NAME, parameterID);

        // While we use textFromValueFunction in the parameter to expose that to the host, we don't want this
        // to show in the rotaries textbox. 
        float parameterInterval = apvts.getParameter(parameterID)->getNormalisableRange().interval;
        rotary.textFromValueFunction = [parameterInterval](double v)
        {
            int numDecimalPlaces = juce::String{ int(1.f / parameterInterval) }.length() - 1;
            return juce::String(v, numDecimalPlaces);
        };
    }
};

class CustomLabel final : public juce::Label, public EnabledMouseCursor
{
    void enablementChanged() override
    {
        EnabledMouseCursor::enablementChanged(*this);
    }
};

/** This includes some utility methods for dealing with selection on a component. 
    T is intended to be an enum of different parts of the component where the first enum value is NONE. 
    See FilterResponse for a clear example of how these are used.
*/
template <typename T>
struct CompPart
{
    T part;
    juce::Rectangle<float> area;
    int priority;

    CompPart(T part, juce::Rectangle<float> area, int priority) : part(part), area(area), priority(priority) {}

    float distanceTo(int x, int y) const
    {
        float dx = juce::jmax<float>(area.getX() - x, 0, x - area.getRight());
        float dy = juce::jmax<float>(area.getY() - y, 0, y - area.getBottom());
        return std::sqrt(dx * dx + dy * dy);
    }

    static T getClosestInRange(juce::Array<CompPart<T>> targets, int x, int y, int snapAmount=0)
    {
        T closest = static_cast<T>(0);
        auto priority = -1;
        auto closestDistance = INFINITY;
        for (const auto& p : targets)
        {
            auto distance = p.distanceTo(x, y);
            if (((distance < closestDistance && p.priority == priority) || p.priority > priority) && distance <= snapAmount)
            {
                closest = p.part;
                priority = p.priority;
                closestDistance = distance;
            }
        }
        return closest;
    }
};

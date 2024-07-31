/*
  ==============================================================================

    Button.h
    Created: 30 Jul 2024 11:30:24pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class CustomToggleableButton final : public juce::Button, public juce::Button::Listener
{
public:
    /** Create a new button with the given colors. */
    CustomToggleableButton(juce::Colour offColor, juce::Colour onColor, bool altStyle = false) : Button(""), offColor(offColor), onColor(onColor), altStyle(altStyle)
    {
        setClickingTogglesState(true);
    }

    ~CustomToggleableButton() override
    {
        if (ownerButton)
            ownerButton->removeListener(this);
    }

    /** An owner button is a button that will be toggled on when this button is toggled but can also be toggled independently. */
    void setOwnerButton(Button* owner)
    {
        ownerButton = owner;
        owner->addListener(this);
        if (!owner->getToggleState())
            setClickingTogglesState(false);
    }

    void buttonStateChanged(Button* button) override
    {
        if (button == ownerButton)
        {
            setClickingTogglesState(ownerButton->getToggleState());
            if (getToggleState())
                setState(ownerButton->getState());
            repaint();
        }
    }

    void buttonClicked(Button*) override {}

    void clicked() override
    {
        if (ownerButton && !ownerButton->getToggleState())
        {
            ownerButton->setToggleState(true, juce::sendNotificationSync);
            ownerButton->setState(buttonNormal);
            setToggleState(true, juce::dontSendNotification);
            repaint();
        }
    }

    void buttonStateChanged() override
    {
        if (ownerButton && !ownerButton->getToggleState())
        {
            ownerButton->setState(getState());
            ownerButton->repaint();
        }
    }

    /** Use x, y, width, height to set the left, top, right, and bottom bounds of the button */
    void setBorder(float width, float rounding = 0.f, bool roundTopLeft = true, bool roundTopRight = true, bool roundBottomLeft = true, bool roundBottomRight = true)
    {
        borderWidth = width;
        borderRounding = rounding;
        topLeft = roundTopLeft;
        topRight = roundTopRight;
        bottomLeft = roundBottomLeft;
        bottomRight = roundBottomRight;
    }

    void setPadding(float amount)
    {
        setPadding(amount, amount, amount, amount);
    }

    void setPadding(float paddingLeft, float paddingRight, float paddingTop, float paddingBottom)
    {
        padLeft = paddingLeft;
        padRight = paddingRight;
        padTop = paddingTop;
        padBottom = paddingBottom;
    }

    void useShape(const juce::Path& shapePath)
    {
        shape = shapePath;
    }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        bool toggledOn = getToggleState() && (!ownerButton || ownerButton->getToggleState());
        bool drawAsOn = toggledOn != shouldDrawButtonAsDown;

        auto bounds = getLocalBounds().toFloat();

        if (drawAsOn)
        {
            juce::Path background;
            background.addRoundedRectangle(
                bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), 
                borderRounding, borderRounding, topLeft, topRight, bottomLeft, bottomRight);

            g.setColour(altStyle ? onColor : offColor);
            g.fillPath(background);
            onShadow.render(g, background);
        }
        else
        {
            juce::Path border;
            auto pathBounds = bounds.reduced(borderWidth * 0.5f);
            border.addRoundedRectangle(
                pathBounds.getX(), pathBounds.getY(), pathBounds.getWidth(), pathBounds.getHeight(),
                borderRounding, borderRounding, topLeft, topRight, bottomLeft, bottomRight);

            g.setColour(offColor);
            g.strokePath(border, juce::PathStrokeType(borderWidth));
        }

        bounds.removeFromLeft(padLeft);
        bounds.removeFromRight(padRight);
        bounds.removeFromTop(padTop);
        bounds.removeFromBottom(padBottom);
        auto trans = shape.getTransformToScaleToFit(bounds, true);

        g.setColour(drawAsOn && !altStyle ? onColor : offColor);
        g.fillPath(shape, trans);
    }

private:
    juce::Colour offColor, onColor;
    bool altStyle{ false };

    float borderWidth{ 1.f };
    float borderRounding{ 0.f };
    bool topLeft{ true }, topRight{ true }, bottomLeft{ true }, bottomRight{ true };
    float padLeft, padRight, padTop, padBottom;

    juce::Path shape;

    melatonin::InnerShadow onShadow{ juce::Colours::black.withAlpha(0.25f), 3, {0, 2} };

    Button* ownerButton{ nullptr };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomToggleableButton)
};

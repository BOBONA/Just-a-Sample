/*
  ==============================================================================

    Buttons.h
    Created: 30 Jul 2024 11:30:24pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "../Utilities/ComponentUtils.h"

/** Simple wrapper around JUCE's ShapeButton for transparent disabled styling */
class CustomShapeButton final : public juce::ShapeButton, public CustomHelpTextProvider
{
public:
    explicit CustomShapeButton(juce::Colour buttonColor, const juce::Path& shape = {}, CustomHelpTextDisplay* helpTextDisplay = nullptr) :
        ShapeButton("", buttonColor, buttonColor, buttonColor),
        CustomHelpTextProvider(this, helpTextDisplay),
        color(buttonColor) 
    {
        ShapeButton::setShape(shape, false, true, false);
    }

    explicit CustomShapeButton(const juce::Path& shape = {}, CustomHelpTextDisplay* helpTextDisplay = nullptr) :
        CustomShapeButton(juce::Colours::transparentBlack, shape, helpTextDisplay)
    {
    }

    void setColor(juce::Colour buttonColor)
    {
        color = buttonColor;
        setColours(buttonColor, buttonColor, buttonColor);
        repaint();
    }

    void setShape(const juce::Path& newShape)
    {
        ShapeButton::setShape(newShape, false, true, false);
        repaint();
    }

private:
    void enablementChanged() override
    {
        auto adjustedColor = color.withMultipliedAlpha(isEnabled() ? 1.f : 0.5f);
        setColours(adjustedColor, adjustedColor, adjustedColor);

        if (isEnabled())
        {
            setMouseCursor(previousMouseCursor);
        }
        else
        {
            previousMouseCursor = getMouseCursor();
            setMouseCursor(juce::MouseCursor::NormalCursor);
        }

        repaint();
    }

    juce::Colour color;
    juce::MouseCursor previousMouseCursor{ juce::MouseCursor::NormalCursor };
};

/** A nicely styled button that can be toggled on and off. An owner button can be set to "own" this button,
    meaning this button is only on when the owner button is on.
*/
class CustomToggleableButton final : public juce::Button, public juce::Button::Listener, public CustomHelpTextProvider
{
public:
    /** Create a new button with the given colors.
        altStyle keeps the text the same while changing the background, onBackground removes the shadow and sets a background when on
    */
    CustomToggleableButton(juce::Colour offColor, juce::Colour onColor, bool altStyle = false, bool useOnBackground = false, CustomHelpTextDisplay* helpTextDisplay = nullptr) :
        Button(""), CustomHelpTextProvider(this, helpTextDisplay),
        offColor(offColor), onColor(onColor), altStyle(altStyle), onBackground(useOnBackground)
    {
        setClickingTogglesState(true);
    }

    CustomToggleableButton(bool altStyle = false, bool useOnBackground = false, CustomHelpTextDisplay* helpTextDisplay = nullptr) :
        CustomToggleableButton(juce::Colours::transparentBlack, juce::Colours::transparentBlack, altStyle, useOnBackground, helpTextDisplay)
    {
    }

    ~CustomToggleableButton() override
    {
        if (ownerButton)
            ownerButton->removeListener(this);
    }

    void setColors(juce::Colour off, juce::Colour on, juce::Colour shadow = defaultTheme.dark.withAlpha(0.25f))
    {
        offColor = off;
        onColor = on;
        onShadow.setColor(shadow);

        repaint();
    }

    /** An owner button is a button that will be toggled on when this button is toggled but can also be toggled independently. */
    void setOwnerButton(Button* owner)
    {
        ownerButton = owner;
        owner->addListener(this);
        if (!owner->getToggleState())
            setClickingTogglesState(false);
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

    void useShape(const juce::Path& shapePath, const juce::Path& offShapePath = {}, juce::Justification justification = juce::Justification::centred)
    {
        shape = shapePath;
        offShape = offShapePath;
        shapeJustification = justification;
    }

private:
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

    void paintButton(juce::Graphics& g, bool /*shouldDrawButtonAsHighlighted*/, bool shouldDrawButtonAsDown) override
    {
        bool toggledOn = getToggleState() && (!ownerButton || ownerButton->getToggleState());
        bool drawAsOn = toggledOn != shouldDrawButtonAsDown;

        auto bounds = getLocalBounds().toFloat();

        if (drawAsOn)
        {
            juce::Path background;
            auto pathBounds = bounds.reduced(borderWidth * 0.33f);
            background.addRoundedRectangle(pathBounds.getX(), pathBounds.getY(), pathBounds.getWidth(), pathBounds.getHeight(), 
                borderRounding, borderRounding, topLeft, topRight, bottomLeft, bottomRight);

            g.setColour(altStyle ? onColor : offColor);
            g.fillPath(background);

            if (!onBackground)
                onShadow.render(g, background);
        }
        else if (!onBackground)
        {
            juce::Path border;
            auto pathBounds = bounds.reduced(borderWidth * 0.66f);
            border.addRoundedRectangle(pathBounds.getX(), pathBounds.getY(), pathBounds.getWidth(), pathBounds.getHeight(),
                borderRounding, borderRounding, topLeft, topRight, bottomLeft, bottomRight);

            g.setColour(offColor);
            g.strokePath(border, juce::PathStrokeType(borderWidth));
        }

        bounds.removeFromLeft(padLeft);
        bounds.removeFromRight(padRight);
        bounds.removeFromTop(padTop);
        bounds.removeFromBottom(padBottom);

        auto useShape = !drawAsOn && !offShape.isEmpty() ? offShape : shape;
        auto trans = useShape.getTransformToScaleToFit(bounds, true, shapeJustification);

        g.setColour(drawAsOn && !altStyle ? onColor : offColor);
        if (!useShape.isEmpty())
        {
            g.fillPath(useShape, trans);
        }
        else if (getButtonText().isNotEmpty())
        {
            g.setFont(getInterBold().withHeight(getHeight() * 0.381f));
            g.drawText(getButtonText(), getLocalBounds(), juce::Justification::centred);
        }

        if (!isEnabled())
        {
            g.setColour(findColour(Colors::backgroundColorId, true).withAlpha(0.5f));
            g.fillRect(getLocalBounds());
        }
    }

    void enablementChanged() override
    {
        if (isEnabled())
        {
            setMouseCursor(previousMouseCursor);
        }
        else
        {
            previousMouseCursor = getMouseCursor();
            setMouseCursor(juce::MouseCursor::NormalCursor);
        }

        repaint();
    }

    //==============================================================================
    juce::Colour offColor, onColor;
    bool altStyle{ false }, onBackground{ false };

    float borderWidth{ 1.f };
    float borderRounding{ 0.f };
    bool topLeft{ true }, topRight{ true }, bottomLeft{ true }, bottomRight{ true };
    float padLeft{ 0.f }, padRight{ 0.f }, padTop{ 0.f }, padBottom{ 0.f };

    juce::Path shape, offShape;
    juce::Justification shapeJustification{ juce::Justification::centred };

    juce::MouseCursor previousMouseCursor{ juce::MouseCursor::NormalCursor };

    melatonin::InnerShadow onShadow{ defaultTheme.dark.withAlpha(0.25f), 3, {0, 2} };

    Button* ownerButton{ nullptr };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomToggleableButton)
};

/** Two buttons that act as a nicely styled toggle. */
class CustomChoiceButton final : public CustomComponent
{
public:
    CustomChoiceButton(const APVTS& apvts, const juce::Identifier& parameter, juce::Colour offColor, juce::Colour onColor, 
        const juce::String& firstButtonText, const juce::String& secondButtonText, CustomHelpTextDisplay* helpTextDisplay = nullptr) :
        CustomComponent(helpTextDisplay),
        firstButton(offColor, onColor), secondButton(offColor, onColor),
        choiceAttachment(*apvts.getParameter(parameter), [this](float newValue) { valueChanged(newValue); }, apvts.undoManager),
        offColor(offColor), onColor(onColor)
    {
        setPaintingIsUnclipped(true);  // Stop border from clipping

        firstButton.setButtonText(firstButtonText);
        secondButton.setButtonText(secondButtonText);

        firstButton.onClick = [this] { choiceAttachment.setValueAsCompleteGesture(0.f); };
        secondButton.onClick = [this] { choiceAttachment.setValueAsCompleteGesture(1.f); };

        firstButton.onStateChange = [this] { secondButton.setState(firstButton.getState()); };
        secondButton.onStateChange = [this] { firstButton.setState(secondButton.getState()); };

        addAndMakeVisible(firstButton);
        addAndMakeVisible(secondButton);

        choiceAttachment.sendInitialUpdate();
    }

    CustomChoiceButton(const APVTS& apvts, const juce::Identifier& parameter, const juce::String& firstButtonText, const juce::String& secondButtonText,
        CustomHelpTextDisplay* helpTextDisplay = nullptr) :
        CustomChoiceButton(apvts, parameter, juce::Colours::transparentBlack, juce::Colours::transparentBlack, firstButtonText, secondButtonText, helpTextDisplay)
    {
    }

    void setColors(juce::Colour off, juce::Colour on, juce::Colour shadow = defaultTheme.dark.withAlpha(0.25f))
    {
        offColor = off;
        onColor = on;

        firstButton.setColors(off, on, shadow);
        secondButton.setColors(off, on, shadow);

        repaint();
    }

    void setBorder(float width, float rounding = 0.f)
    {
        borderWidth = width;
        borderRounding = rounding;
        repaint();
    }

    bool getChoice() const
    {
        return secondButton.getToggleState();
    }

    void setHelpText(const juce::String& firstHelpText, const juce::String& secondHelpText)
    {
        firstButton.setHelpText(firstHelpText);
        secondButton.setHelpText(secondHelpText);
    }

    juce::String getCustomHelpText() override
    {
        return getChoice() ? secondButton.getHelpText() : firstButton.getHelpText();
    }

private:
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        firstButton.setBorder(0.f, borderRounding, true, false, true, false);
        secondButton.setBorder(0.f, borderRounding, false, true, false, true);

        g.setColour(offColor);
        g.drawRoundedRectangle(bounds.reduced(borderWidth / 2.f), borderRounding, borderWidth);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        firstButton.setBounds(bounds.removeFromLeft(int(bounds.getWidth() / 2.f)));
        secondButton.setBounds(bounds);
    }

    void valueChanged(float newValue)
    {
        firstButton.setToggleState(newValue < 0.5f, juce::dontSendNotification);
        secondButton.setToggleState(newValue > 0.5f, juce::dontSendNotification);

        firstButton.setInterceptsMouseClicks(!firstButton.getToggleState(), false);
        secondButton.setInterceptsMouseClicks(!secondButton.getToggleState(), false);

        sendHelpTextUpdate(false);
    }

    void enablementChanged() override
    {
        if (!isEnabled())
        {
            firstButton.setInterceptsMouseClicks(false, false);
            secondButton.setInterceptsMouseClicks(false, false);
        }
        else
        {
            choiceAttachment.sendInitialUpdate();
        }
        repaint();
    }

    //==============================================================================
    CustomToggleableButton firstButton;
    CustomToggleableButton secondButton;
    juce::ParameterAttachment choiceAttachment;

    juce::Colour offColor, onColor;

    float borderWidth{ 1.f }, borderRounding{ 0.f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomChoiceButton)
};
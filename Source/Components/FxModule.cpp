/*
  ==============================================================================

    FXModule.cpp
    Created: 28 Dec 2023 8:09:18pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>

#include "FxModule.h"

FxModule::FxModule(FxDragTarget* fxChain, APVTS& apvts, const juce::String& fxName, PluginParameters::FxTypes effectType, const juce::String& fxEnabledParameter, Component& displayComponent) :
    fxChain(fxChain), apvts(apvts), fxType(fxType), nameLabel("", fxName), mixControl(juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox),
    enablementAttachment(apvts, fxEnabledParameter, fxEnabled), display(displayComponent),
    dragIcon(getOutlineFromSVG(BinaryData::IconDrag_svg))
{
    nameLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(&nameLabel);

    fxEnabled.onStateChange = [&] { 
        display.setEnabled(fxEnabled.getToggleState());
        for (auto rotary : rotaryReferences)
            rotary->setEnabled(fxEnabled.getToggleState());
        for (auto& label : labels)
            label->setEnabled(fxEnabled.getToggleState());
    };
    fxEnabled.onStateChange();  // Set the initial state
    addAndMakeVisible(&fxEnabled);

    addAndMakeVisible(&displayComponent);

    setRepaintsOnMouseActivity(true);
    addMouseListener(this, true);
}

FxModule::FxModule(FxDragTarget* fxChain, APVTS& apvts, const juce::String& fxName, PluginParameters::FxTypes fxType, const juce::String& fxEnabledParameter, const juce::String& mixControlParameter, Component& displayComponent) :
    FxModule(fxChain, apvts, fxName, fxType, fxEnabledParameter, displayComponent)
{
    setupRotary(mixControl, false);
    mixControlAttachment = std::make_unique<APVTS::SliderAttachment>(apvts, mixControlParameter, mixControl);
}

juce::Slider* FxModule::addRotary(const juce::String& parameter, const juce::String& label, juce::Point<float> position, float width, const juce::String& unit)
{
    auto rotary = std::make_unique<juce::Slider>();
    juce::Slider* rawRotaryPtr = rotary.get();

    setupRotary(*rotary);
    rotary->getProperties().set(ComponentProps::ROTARY_UNIT, unit);
    addAndMakeVisible(rawRotaryPtr);
    rotaries.push_back(std::move(rotary));

    auto rotaryLabel = std::make_unique<juce::Label>("", label);
    rotaryLabel->setInterceptsMouseClicks(false, false);
    rotaryLabel->setJustificationType(juce::Justification::centredBottom);
    rotaryLabel->setEnabled(fxEnabled.getToggleState());
    addAndMakeVisible(rotaryLabel.get());
    labels.push_back(std::move(rotaryLabel));

    auto attachment = std::make_unique<APVTS::SliderAttachment>(apvts, parameter, *rawRotaryPtr);
    attachments.push_back(std::move(attachment));

    rotaryPositions.push_back(position);
    rotaryWidths.push_back(width);

    return rawRotaryPtr;
}

void FxModule::setupRotary(juce::Slider& rotary, bool useTextbox)
{
    rotary.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    rotary.setMouseDragSensitivity(150);
    rotary.setNumDecimalPlacesToDisplay(2);
    if (useTextbox)
        rotary.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 0, 0);
    rotary.addMouseListener(this, true);  // We need this to pass drag events through the labels
    rotary.setEnabled(fxEnabled.getToggleState());
    addAndMakeVisible(rotary);

    rotaryReferences.push_back(&rotary);
}

void FxModule::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(Colors::BACKGROUND);
    g.fillAll();

    // Draw the drag icon
    auto header = bounds.removeFromTop(scale(73.f));
    auto mouseInHeader = header.contains(getMouseXYRelative().toFloat());
    if (mouseInHeader)
    {
        g.setColour(Colors::DARKER_SLATE);
        header.removeFromTop(scale(25.f));
        auto iconBounds = header.removeFromTop(scale(28.f));
        g.fillPath(dragIcon, dragIcon.getTransformToScaleToFit(iconBounds, true));
    }
}

void FxModule::resized()
{
    auto bounds = getLocalBounds().toFloat();

    // Header
    auto mixPad = scale(43.f) * Layout::rotaryPadding;
    auto header = bounds.removeFromTop(scale(73.f)).reduced(scale(21.f), scale(15.f) - mixPad);

    fxEnabled.setBounds(header.removeFromRight(scale(43.f)).reduced(0.f, mixPad).toNearestInt());
    header.removeFromRight(scale(20.f) - mixPad);
    if (mixControlAttachment)
        mixControl.setBounds(header.removeFromRight(scale(43.f) + 2 * mixPad).toNearestInt());
    header.removeFromRight(scale(20.f) - mixPad);
    nameLabel.setBounds(header.reduced(0.f, mixPad).toNearestInt());
    nameLabel.setFont(getInriaSans().withHeight(scalef(41.4f)));

    // Controls
    display.setBounds(bounds.removeFromTop(scale(103.f)).toNearestInt());

    bounds.reduce(0.f, scale(27.5f));

    for (size_t i = 0; i < rotaries.size(); i++)
    {
        auto* rotary = rotaries[i].get();
        auto [xPos, yPos] = rotaryPositions[i];
        auto width = rotaryWidths[i];
        auto padding = Layout::rotaryPadding * scale(width);

        rotary->setBounds(juce::Rectangle<float>(bounds.getX() + scale(xPos), bounds.getY() + scale(yPos), scale(width), scale(width) * Layout::rotaryHeightRatio).expanded(padding).toNearestInt());

        auto* label = labels[i].get();
        auto labelBounds = juce::Rectangle<float>(bounds.getX() + scale(xPos), bounds.getY() + scale(yPos) - scale(width) * 0.45f, scale(width), scale(width * 0.356f)).expanded(width, 0.f);
        label->setFont(getInriaSans().withHeight(scale(width) * 0.356f));
        label->setBounds(labelBounds.toNearestInt());
    }

    if (mixControlAttachment)
        mixControl.sendLookAndFeelChange();
    for (auto& rotary : rotaries)
        rotary->sendLookAndFeelChange();
}

void FxModule::mouseDown(const juce::MouseEvent& event)
{
    // We pass the events on the rotary number labels through to the rotary
    auto parent = event.originalComponent->getParentComponent();
    for (auto rotary : rotaryReferences)
        if (parent == rotary)
            rotary->mouseDown(event.getEventRelativeTo(rotary));
}

//==============================================================================
void FxModule::mouseUp(const juce::MouseEvent& event)
{
    if (dragging)
    {
        dragging = false;
        fxChain->dragEnded();
        repaint();
    }

    auto parent = event.originalComponent->getParentComponent();
    for (auto rotary : rotaryReferences)
        if (parent == rotary)
            rotary->mouseUp(event.getEventRelativeTo(rotary));
}

void FxModule::mouseDrag(const juce::MouseEvent& event)
{
    if (!dragging && event.eventComponent == this)
    {
        dragging = true;
        fxChain->dragStarted(this, event);
        repaint();
    }

    auto parent = event.originalComponent->getParentComponent();
    for (auto rotary : rotaryReferences)
        if (parent == rotary)
            rotary->mouseDrag(event.getEventRelativeTo(rotary));
}

void FxModule::mouseEnter(const juce::MouseEvent& event)
{
    repaint();
}

void FxModule::mouseExit(const juce::MouseEvent& event)
{
    repaint();
}

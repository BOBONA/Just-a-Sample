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
    fxChain(fxChain), apvts(apvts), fxType(effectType), nameLabel("", fxName), mixControl(juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox),
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
    fxEnabled.setHelpText("Toggle " + fxName.toLowerCase());
    addAndMakeVisible(&fxEnabled);

    addAndMakeVisible(&displayComponent);

    setRepaintsOnMouseActivity(true);
    addMouseListener(this, true);
    setHelpText(fxName + " module");

    setBufferedToImage(true);
}

FxModule::FxModule(FxDragTarget* fxChain, APVTS& apvts, const juce::String& fxName, PluginParameters::FxTypes effectType, const juce::String& fxEnabledParameter, const juce::String& mixControlParameter, Component& displayComponent) :
    FxModule(fxChain, apvts, fxName, effectType, fxEnabledParameter, displayComponent)
{
    setupRotary(mixControl, false);
    mixControl.getProperties().set(ComponentProps::ROTARY_UNIT, "%");
    mixControlAttachment = std::make_unique<CustomRotaryAttachment>(apvts, mixControlParameter, mixControl);
}

juce::Slider* FxModule::addRotary(const juce::String& parameter, const juce::String& label, juce::Point<float> position, float width, const juce::String& unit)
{
    auto rotary = std::make_unique<CustomRotary>();
    CustomRotary* rawRotaryPtr = rotary.get();

    auto attachment = std::make_unique<CustomRotaryAttachment>(apvts, parameter, *rawRotaryPtr);
    attachments.push_back(std::move(attachment));

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

    rotaryPositions.push_back(position);
    rotaryWidths.push_back(width);

    return rawRotaryPtr;
}

void FxModule::setupRotary(CustomRotary& rotary, bool useTextbox)
{
    rotary.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    rotary.setMouseDragSensitivity(150);
    if (useTextbox)
        rotary.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 0, 0);
    rotary.addMouseListener(this, true);  // We need this to pass drag events through the labels
    rotary.setEnabled(fxEnabled.getToggleState());
    addAndMakeVisible(rotary);

    rotaryReferences.push_back(&rotary);
}

void FxModule::paint(juce::Graphics& g)
{
    auto colors = getTheme();
    auto bounds = getLocalBounds();

    g.setColour(colors.background);
    g.fillAll();

    // Draw the drag icon
    auto header = bounds.removeFromTop(scale(Layout::fxModuleHeader));
    if (header.contains(getMouseXYRelative()))
    {
        g.setColour(colors.darkerSlate);
        header.removeFromTop(scale(25.f));
        auto iconBounds = header.removeFromTop(scale(28.f));
        g.fillPath(dragIcon, dragIcon.getTransformToScaleToFit(iconBounds.toFloat(), true));
    }
}

void FxModule::resized()
{
    auto bounds = getLocalBounds().toFloat();

    // Header
    auto mixPad = scale(43.f) * Layout::rotaryPadding;
    auto header = bounds.removeFromTop(scale(Layout::fxModuleHeader)).reduced(scale(21.f), scale(15.f) - mixPad);

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

void FxModule::enablementChanged()
{
    setInterceptsMouseClicks(isEnabled(), isEnabled());
    repaint();
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

void FxModule::mouseMove(const juce::MouseEvent& event)
{
    auto header = getLocalBounds().toFloat().removeFromTop(scale(Layout::fxModuleHeader));
    auto mouseInHeader = header.contains(getMouseXYRelative().toFloat());

    if (mouseInHeader)
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    else
        setMouseCursor(juce::MouseCursor::NormalCursor);
}

void FxModule::mouseDrag(const juce::MouseEvent& event)
{
    auto header = getLocalBounds().toFloat().removeFromTop(scale(Layout::fxModuleHeader));
    auto mouseInHeader = header.contains(getMouseXYRelative().toFloat());

    if (!dragging && event.eventComponent == this && mouseInHeader)
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

void FxModule::mouseEnter(const juce::MouseEvent&)
{
    repaint();
}

void FxModule::mouseExit(const juce::MouseEvent&)
{
    repaint();
}

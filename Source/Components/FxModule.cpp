/*
  ==============================================================================

    FXModule.cpp
    Created: 28 Dec 2023 8:09:18pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>

#include "FxModule.h"

FxModule::FxModule(FxDragger* fxChain, AudioProcessorValueTreeState& apvts, const String& fxName, const String& fxEnabledID) : 
    fxChain(fxChain), apvts(apvts), enablementAttachment(apvts, fxEnabledID, fxEnabled)
{
    nameLabel.setText(fxName, dontSendNotification);
    nameLabel.setColour(Label::textColourId, lnf.TITLE_TEXT);
    nameLabel.setJustificationType(Justification::centredLeft);
    addAndMakeVisible(&nameLabel);

    fxEnabled.onStateChange = [&] { 
        nameLabel.setEnabled(fxEnabled.getToggleState());
        if (displayComponent)
            displayComponent->setEnabled(fxEnabled.getToggleState()); 
        for (auto& control : controls)
            control.second->setEnabled(fxEnabled.getToggleState());
        repaint();
    };
    fxEnabled.onStateChange();
    addAndMakeVisible(&fxEnabled);

    addMouseListener(this, true);
}

FxModule::FxModule(FxDragger* fxChain, AudioProcessorValueTreeState& apvts, const String& fxName, const String& fxEnabledID, const String& mixControlID) : FxModule(fxChain, apvts, fxName, fxEnabledID)
{
    useMixControl = true;
    mixControlAttachment = std::make_unique<APVTS::SliderAttachment>(apvts, mixControlID, mixControl);
    mixControl.setSliderStyle(Slider::RotaryVerticalDrag);
    mixControl.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(&mixControl);
}

FxModule::~FxModule()
{
}

void FxModule::paint(Graphics& g)
{
    g.setColour(lnf.BACKGROUND_COLOR);
    g.fillAll();
    auto bounds = getLocalBounds();
    g.setColour(Colours::black);
    g.drawRect(bounds);
    if (mouseOver)
    {
        // this draws the dot thing that lots of applications use
        static float circleSize = 2.7f;
        static int circlesW = 4;
        static int circlesV = 3;

        auto dragArea = bounds.removeFromBottom(DRAG_AREA).toFloat();
        float difference = jmax<float>(0, (bounds.getWidth() - 20.f) / 2);
        dragArea.removeFromLeft(difference);
        dragArea.removeFromRight(difference + circleSize);
        dragArea.removeFromBottom(circleSize + 2);

        g.setColour(disabled(lnf.SAMPLE_BOUNDS_COLOR, !fxEnabled.getToggleState()));
        for (float x = dragArea.getX(); x <= dragArea.getRight(); x += dragArea.getWidth() / (circlesW - 1))
        {
            for (float y = dragArea.getY(); y <= dragArea.getBottom(); y += dragArea.getHeight() / (circlesV - 1))
            {
                g.fillEllipse(x, y, circleSize, circleSize);
            }
        }
    }
}

void FxModule::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromBottom(DRAG_AREA);
    auto top = bounds.removeFromTop(30);
    fxEnabled.setBounds(top.removeFromRight(30));
    if (useMixControl)
    {
        mixControl.setBounds(top.removeFromRight(17));
    }
    nameLabel.setBounds(top);
    
    if (displayComponent)
    {
        auto displayArea = bounds.removeFromTop(int(displayHeight));
        displayComponent->setBounds(displayArea);
    }

    auto numRows = rows.size();
    if (numRows)
    {
        auto rowHeight = bounds.getHeight() / numRows;
        for (const auto& row : rows)
        {
            auto rowBounds = bounds.removeFromTop(rowHeight);
            rowBounds.removeFromTop(8); // for the label
            auto controlWidth = rowBounds.getWidth() / row->size();
            for (const auto& control : *row)
            {
                auto& comp = controls[control.id];
                comp->setBounds(rowBounds.removeFromLeft(controlWidth));
            }
        }
    }
}

void FxModule::mouseEnter(const MouseEvent&)
{
    mouseOver = true;
    repaint();
}

void FxModule::mouseExit(const MouseEvent&)
{
    mouseOver = false;
    repaint();
}

void FxModule::mouseUp(const MouseEvent&)
{
    if (dragging)
    {
        dragging = false;
        fxChain->dragEnded();
    }
}

void FxModule::mouseDrag(const MouseEvent& event)
{
    if (!dragging && event.eventComponent == this)
    {
        dragging = true;
        fxChain->dragStarted(nameLabel.getText(), event);
    }
}

void FxModule::addRow(Array<ModuleControl> row)
{
    rows.add(std::make_unique<Array<ModuleControl>>(row));
    for (const auto& control : row)
    {
        if (control.type == ModuleControl::ROTARY)
        {
            auto slider = std::make_unique<Slider>();
            slider->setSliderStyle(Slider::RotaryVerticalDrag);
            slider->setTextBoxStyle(Slider::TextBoxRight, false, 40, 12);
            slider->setEnabled(fxEnabled.getToggleState());
            addAndMakeVisible(*slider);
            auto& ref = *slider;
            controls.emplace(control.id, std::move(slider));

            auto label = std::make_unique<Label>();
            label->setText(control.label, dontSendNotification);
            label->setFont(Font(8));
            label->setJustificationType(Justification::centred);
            label->attachToComponent(&ref, false);
            label->setEnabled(fxEnabled.getToggleState());
            addAndMakeVisible(*label);
            controls.emplace(control.id + "l", std::move(label));

            auto attachment = std::make_unique<APVTS::SliderAttachment>(apvts, control.id, ref);
            attachments.add(std::move(attachment));
        }
    }
    resized();
}

void FxModule::setDisplayComponent(Component* displayComp, float compHeight)
{
    displayComponent = displayComp;
    displayComponent->setEnabled(fxEnabled.getToggleState());
    addAndMakeVisible(displayComponent);
    displayHeight = compHeight;
    resized();
}

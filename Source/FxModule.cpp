/*
  ==============================================================================

    FXModule.cpp
    Created: 28 Dec 2023 8:09:18pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>
#include "FxModule.h"

FxModule::FxModule(AudioProcessorValueTreeState& apvts, const String& fxName) : apvts(apvts)
{
    nameLabel.setText(fxName, dontSendNotification);
    nameLabel.setColour(Label::textColourId, lnf.TITLE_TEXT);
    nameLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(&nameLabel);
}

FxModule::~FxModule()
{
}

void FxModule::paint (Graphics& g)
{
    auto bounds = getLocalBounds();
    g.drawRect(bounds);
}

void FxModule::resized()
{
    auto bounds = getLocalBounds();
    auto top = bounds.removeFromTop(30);
    nameLabel.setBounds(top);

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
            addAndMakeVisible(*slider);
            auto& ref = *slider;
            controls.emplace(control.id, std::move(slider));

            auto label = std::make_unique<Label>();
            label->setText(control.label, dontSendNotification);
            label->setFont(Font(8));
            label->setJustificationType(Justification::centred);
            label->attachToComponent(&ref, false);
            addAndMakeVisible(*label);
            controls.emplace(control.id + "l", std::move(label));

            auto attachment = std::make_unique<APVTS::SliderAttachment>(apvts, control.id, ref);
            attachments.add(std::move(attachment));
        }
    }
    resized();
}

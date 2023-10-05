/*
  ==============================================================================

    SampleEditor.cpp
    Created: 19 Sep 2023 2:03:29pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>
#include "SampleEditor.h"

//==============================================================================
SampleEditorOverlay::SampleEditorOverlay(juce::Array<int>& voicePositions) : voicePositions(voicePositions)
{
}

SampleEditorOverlay::~SampleEditorOverlay()
{
}

void SampleEditorOverlay::paint(juce::Graphics& g)
{
    using namespace juce;
    if (sample)
    {
        // paints the voice positions
        Path path{};
        for (auto i = 0; i < voicePositions.size(); i++)
        {
            if (voicePositions[i] > 0)
            {
                auto pos = jmap<float>(voicePositions[i], 0, sample->getNumSamples(), 0, getWidth());
                path.addLineSegment(Line<float>(pos, 0, pos, getHeight()), 1);
            }
        }
        g.setColour(lnf.VOICE_POSITION_COLOR);
        g.strokePath(path, PathStrokeType(1.f));
    }
}

void SampleEditorOverlay::resized()
{
}

void SampleEditorOverlay::setSample(juce::AudioBuffer<float>& sample)
{
    this->sample = &sample;
}

//==============================================================================
SampleEditor::SampleEditor(APVTS& apvts, juce::Array<int>& voicePositions) : apvts(apvts), overlay(voicePositions)
{
    apvts.state.addListener(this);

    addAndMakeVisible(&painter);
    overlay.toFront(true);
    addAndMakeVisible(&overlay);
}

SampleEditor::~SampleEditor()
{
    apvts.state.removeListener(this);
}

void SampleEditor::paint(juce::Graphics& g)
{
}

void SampleEditor::resized()
{
    auto bounds = getLocalBounds();
    painter.setBounds(bounds);
    overlay.setBounds(bounds);
}

void SampleEditor::valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property)
{
    if (property.toString() == PluginParameters::UI_VIEW_START || property.toString() == PluginParameters::UI_VIEW_STOP)
    {
        int start = treeWhosePropertyHasChanged.getProperty(PluginParameters::UI_VIEW_START);
        int stop = treeWhosePropertyHasChanged.getProperty(PluginParameters::UI_VIEW_STOP);
        painter.setSampleView(start, stop);
    }
}

void SampleEditor::updateSamplePosition()
{
    overlay.repaint();
}

void SampleEditor::setSample(juce::AudioBuffer<float>& sample, bool resetUI)
{
    if (resetUI)
    {
        painter.setSample(sample);
    }
    else
    {
        painter.setSample(sample, apvts.state.getProperty(PluginParameters::UI_VIEW_START), apvts.state.getProperty(PluginParameters::UI_VIEW_STOP));
    }
    overlay.setSample(sample);
}

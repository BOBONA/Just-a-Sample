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
SampleEditorOverlay::SampleEditorOverlay(APVTS& apvts, juce::Array<int>& voicePositions) : voicePositions(voicePositions)
{
    viewStart = apvts.state.getPropertyAsValue(PluginParameters::UI_VIEW_START, apvts.undoManager);
    viewStop = apvts.state.getPropertyAsValue(PluginParameters::UI_VIEW_STOP, apvts.undoManager);
    sampleStart = apvts.state.getPropertyAsValue(PluginParameters::SAMPLE_START, apvts.undoManager);
    sampleEnd = apvts.state.getPropertyAsValue(PluginParameters::SAMPLE_END, apvts.undoManager);
    viewStart.addListener(this);
    viewStop.addListener(this);
}

SampleEditorOverlay::~SampleEditorOverlay()
{
    viewStart.removeListener(this);
    viewStop.removeListener(this);
}

void SampleEditorOverlay::paint(juce::Graphics& g)
{
    using namespace juce;
    if (sample)
    {
        // paints the voice positions
        int startP = viewStart.getValue();
        int stopP = viewStop.getValue();
        Path path{};
        for (auto i = 0; i < voicePositions.size(); i++)
        {
            if (voicePositions[i] > 0)
            {
                auto pos = jmap<float>(voicePositions[i] - startP, 0, stopP - startP, 0, getWidth());
                path.addLineSegment(Line<float>(pos, 0, pos, getHeight()), 1);
            }
        }
        g.setColour(lnf.VOICE_POSITION_COLOR);
        g.strokePath(path, PathStrokeType(1.f));
        // paints the sample bounds
        g.setColour(lnf.SAMPLE_BOUNDS_COLOR);
        g.drawVerticalLine(lnf.SAMPLE_PADDING + sampleToPosition(sampleStart.getValue()) - 1, 0, getHeight());
        g.drawVerticalLine(lnf.SAMPLE_PADDING + sampleToPosition(sampleEnd.getValue()), 0, getHeight());
    }
}

void SampleEditorOverlay::resized()
{
    painterWidth = getWidth() - 2 * lnf.SAMPLE_PADDING;
}

void SampleEditorOverlay::valueChanged(juce::Value& value)
{
    repaint();
}


float SampleEditorOverlay::sampleToPosition(int sample)
{
    int start = viewStart.getValue();
    int end = viewStop.getValue();
    return juce::jmap<float>(sample - start, 0, end - start, 0, painterWidth);
}

int SampleEditorOverlay::positionToSample(float position)
{
    int start = viewStart.getValue();
    int end = viewStop.getValue();
    return start + juce::jmap<int>(position, 0, painterWidth, 0, end - start);
}

void SampleEditorOverlay::setSample(juce::AudioBuffer<float>& sample)
{
    this->sample = &sample;
}

//==============================================================================
SampleEditor::SampleEditor(APVTS& apvts, juce::Array<int>& voicePositions) : apvts(apvts), overlay(apvts, voicePositions)
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
    overlay.setBounds(bounds);
    bounds.removeFromLeft(lnf.SAMPLE_PADDING);
    bounds.removeFromRight(lnf.SAMPLE_PADDING);
    painter.setBounds(bounds);
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

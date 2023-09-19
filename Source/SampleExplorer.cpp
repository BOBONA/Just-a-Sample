/*
  ==============================================================================

    SampleExplorer.cpp
    Created: 19 Sep 2023 2:03:29pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>
#include "SampleExplorer.h"

//==============================================================================
SampleExplorer::SampleExplorer(juce::Array<int>& voicePositions) : painter(voicePositions)
{
    addAndMakeVisible(&painter);
}

SampleExplorer::~SampleExplorer()
{
}

void SampleExplorer::paint(juce::Graphics& g)
{
}

void SampleExplorer::resized()
{
    auto bounds = getBounds();

    auto navigator = bounds.removeFromBottom(20);

    painter.setBounds(bounds);
}

void SampleExplorer::setSample(juce::AudioBuffer<float>& sample)
{
    painter.setSample(sample);
}

/*
  ==============================================================================

    SampleNavigator.cpp
    Created: 19 Sep 2023 4:41:12pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>
#include "SampleNavigator.h"

SampleNavigatorOverlay::SampleNavigatorOverlay(juce::Array<int>& voicePositions) : voicePositions(voicePositions), lnf(dynamic_cast<CustomLookAndFeel&>(getLookAndFeel()))
{

}

SampleNavigatorOverlay::~SampleNavigatorOverlay()
{
}

void SampleNavigatorOverlay::paint(juce::Graphics&)
{
}

void SampleNavigatorOverlay::resized()
{
}

void SampleNavigatorOverlay::setSample(juce::AudioBuffer<float>& sample)
{
    this->sample = &sample;
}

//==============================================================================
SampleNavigator::SampleNavigator(juce::Array<int>& voicePositions) : overlay(voicePositions)
{
    addAndMakeVisible(&painter);
    overlay.toFront(true);
    addAndMakeVisible(&overlay);
}

SampleNavigator::~SampleNavigator()
{
}

void SampleNavigator::paint (juce::Graphics& g)
{
    
}

void SampleNavigator::resized()
{
    auto bounds = getLocalBounds();
    painter.setBounds(bounds);
    overlay.setBounds(bounds);
}

void SampleNavigator::updateSamplePosition()
{
    overlay.repaint();
}

void SampleNavigator::setSample(juce::AudioBuffer<float>& sample)
{
    painter.setSample(sample);
    overlay.setSample(sample);
}

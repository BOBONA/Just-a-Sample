/*
  ==============================================================================

    SampleComponent.cpp
    Created: 19 Sep 2023 2:03:29pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>
#include "SampleComponent.h"

//==============================================================================
SampleComponent::SampleComponent(juce::Array<int>& voicePositions) : painter(), voicePositions(voicePositions), lnf(dynamic_cast<CustomLookAndFeel&>(getLookAndFeel()))
{
    addAndMakeVisible(&painter);
}

SampleComponent::~SampleComponent()
{
}

void SampleComponent::paint(juce::Graphics& g)
{
}

void SampleComponent::paintOverChildren(juce::Graphics& g)
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
                auto pos = jmap<float>(voicePositions[i], 0, sample->getNumSamples(), 0, sampleArea.getWidth());
                path.addLineSegment(Line<float>(pos, 0, pos, sampleArea.getHeight()), 1);
            }
        }
        g.setColour(lnf.VOICE_POSITION_COLOR);
        g.strokePath(path, PathStrokeType(1.f));
    }
}

void SampleComponent::resized()
{
    auto bounds = getBounds();

    auto navigator = bounds.removeFromBottom(20);
    sampleArea = bounds;
    painter.setBounds(bounds);
}

void SampleComponent::setSample(juce::AudioBuffer<float>& sample)
{
    painter.setSample(sample);
    this->sample = &sample;
}

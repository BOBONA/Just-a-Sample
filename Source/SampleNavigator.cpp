/*
  ==============================================================================

    SampleNavigator.cpp
    Created: 19 Sep 2023 4:41:12pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>
#include "SampleNavigator.h"

SampleNavigatorOverlay::SampleNavigatorOverlay(juce::Array<int>& voicePositions) : voicePositions(voicePositions)
{
    addMouseListener(this, false);
}

SampleNavigatorOverlay::~SampleNavigatorOverlay()
{
}

void SampleNavigatorOverlay::paint(juce::Graphics& g)
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
                auto pos = jmap<float>(voicePositions[i], 0, sample->getNumSamples(), 0, painterBounds.getWidth());
                path.addLineSegment(Line<float>(pos, 0, pos, getHeight()), 1);
            }
        }
        g.setColour(lnf.VOICE_POSITION_COLOR);
        g.strokePath(path, PathStrokeType(1.f));
        // paints the start and stop
        auto startPos = sampleToPosition(startSample);
        auto stopPos = sampleToPosition(stopSample);

        g.setColour(dragging && draggingTarget == SAMPLE_START ? lnf.SAMPLE_BOUNDS_SELECTED_COLOR : lnf.SAMPLE_BOUNDS_COLOR);
        g.fillPath(startSamplePath, juce::AffineTransform::translation(startPos, 0));
        g.setColour(dragging && draggingTarget == SAMPLE_STOP ? lnf.SAMPLE_BOUNDS_SELECTED_COLOR : lnf.SAMPLE_BOUNDS_COLOR);
        g.fillPath(stopSamplePath, juce::AffineTransform::translation(stopPos - painterBounds.getWidth(), 0));
    }
}

void SampleNavigatorOverlay::resized()
{
    startSamplePath.clear();
    auto loc = 1;
    startSamplePath.addLineSegment(juce::Line<float>(loc, 0, loc, getHeight()), 1);
    startSamplePath.addLineSegment(juce::Line<float>(loc, 0, loc + 4, 0), 2);
    startSamplePath.addLineSegment(juce::Line<float>(loc, getHeight(), loc + 4, getHeight()), 2);

    stopSamplePath.clear();
    loc = getWidth() - 1;
    stopSamplePath.addLineSegment(juce::Line<float>(loc, 0, loc, getHeight()), 1);
    stopSamplePath.addLineSegment(juce::Line<float>(loc - 4, 0, loc, 0), 2);
    stopSamplePath.addLineSegment(juce::Line<float>(loc - 4, getHeight(), loc, getHeight()), 2);
}

void SampleNavigatorOverlay::mouseDown(const juce::MouseEvent& event)
{
    if (sample)
    {
        dragging = true;
        auto startPos = sampleToPosition(startSample);
        auto stopPos = sampleToPosition(stopSample);
        if (std::abs(event.getMouseDownX() - (startPos - painterPadding)) < 5)
        {
            draggingTarget = SAMPLE_START;
        }
        else if (std::abs(event.getMouseDownX() - (stopPos - painterPadding)) < 5)
        {
            draggingTarget = SAMPLE_STOP;
        }
        else
        {
            dragging = false;
        }
        repaint();
    }
}

void SampleNavigatorOverlay::mouseUp(const juce::MouseEvent& event)
{
    if (sample)
    {
        dragging = false;
        repaint();
    }
}

void SampleNavigatorOverlay::mouseDrag(const juce::MouseEvent& event)
{
    if (sample)
    {
        auto newSample = positionToSample(event.getMouseDownX() + event.getOffsetFromDragStart().getX() - painterPadding);
        switch (draggingTarget)
        {
        case SAMPLE_START:
            if (0 < newSample && newSample < stopSample)
            {
                startSample = newSample;
                repaint();
            }
            break;
        case SAMPLE_STOP:
            if (startSample < newSample && newSample < sample->getNumSamples())
            {
                stopSample = newSample;
                repaint();
            }
            break;
        }
    }
}

float SampleNavigatorOverlay::sampleToPosition(int sample)
{
    return juce::jmap<float>(sample, 0, this->sample->getNumSamples(), 0, painterBounds.getWidth());
}

int SampleNavigatorOverlay::positionToSample(float position)
{
    return juce::jmap<int>(position, 0, painterBounds.getWidth(), 0, this->sample->getNumSamples());
}

void SampleNavigatorOverlay::setSample(juce::AudioBuffer<float>& sample)
{
    this->sample = &sample;
    startSample = 0;
    stopSample = sample.getNumSamples();
    repaint();
}

void SampleNavigatorOverlay::setPainterBounds(juce::Rectangle<int> bounds)
{
    painterBounds = bounds;
    painterPadding = (getWidth() - bounds.getWidth()) / 2;
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
    overlay.setBounds(bounds);
    bounds.removeFromLeft(2);
    bounds.removeFromRight(2);
    painter.setBounds(bounds);
    overlay.setPainterBounds(bounds);
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

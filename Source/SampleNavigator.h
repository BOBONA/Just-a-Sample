/*
  ==============================================================================

    SampleNavigator.h
    Created: 19 Sep 2023 4:41:12pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "CustomComponent.h"
#include "SamplePainter.h"

//==============================================================================
/*
*/
enum SampleOverlayParts
{
    NONE,
    SAMPLE_START,
    SAMPLE_STOP
};

class SampleNavigatorOverlay : public CustomComponent, public juce::MouseListener, public juce::Value::Listener
{
public:
    SampleNavigatorOverlay(APVTS& apvts, juce::Array<int>& voicePositions);
    ~SampleNavigatorOverlay() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

    void valueChanged(juce::Value& value) override;

    float sampleToPosition(int sample);
    int positionToSample(float position);

    void setSample(juce::AudioBuffer<float>& sample, bool resetUI);
    void setPainterBounds(juce::Rectangle<int> bounds);
private:
    juce::Rectangle<int> painterBounds;
    int painterPadding{ 0 };

    juce::AudioBuffer<float>* sample{ nullptr };
    juce::Array<int>& voicePositions;

    juce::Value startSample, stopSample;
    juce::Path startSamplePath, stopSamplePath;

    bool dragging{ false };
    SampleOverlayParts draggingTarget{ NONE };
};

//==============================================================================
class SampleNavigator : public CustomComponent
{
public:
    SampleNavigator(APVTS& apvts, juce::Array<int>& voicePositions);
    ~SampleNavigator() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void updateSamplePosition();
    void setSample(juce::AudioBuffer<float>& sample, bool resetUI);
private:
    APVTS& apvts;
    SamplePainter painter;
    SampleNavigatorOverlay overlay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleNavigator)
};

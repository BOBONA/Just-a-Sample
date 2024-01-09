/*
  ==============================================================================

    SampleNavigator.h
    Created: 19 Sep 2023 4:41:12pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "../sampler/CustomSamplerVoice.h"
#include "ComponentUtils.h"
#include "displays/SamplePainter.h"

enum class NavigatorParts
{
    NONE,
    SAMPLE_START,
    SAMPLE_END,
    SAMPLE_FULL
};

using Drag = NavigatorParts;

class SampleNavigatorOverlay : public CustomComponent, public juce::Value::Listener
{
public:
    SampleNavigatorOverlay(APVTS& apvts, juce::Array<CustomSamplerVoice*>& synthVoices);
    ~SampleNavigatorOverlay() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseMove(const MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    Drag getDraggingTarget(int x, int y);

    void valueChanged(juce::Value& value) override;

    float sampleToPosition(int sampleIndex);
    int positionToSample(float position);

    void setSample(juce::AudioBuffer<float>& sampleBuffer, bool resetUI);
    void setPainterBounds(juce::Rectangle<int> bounds);
private:
    juce::Rectangle<int> painterBounds;
    int painterPadding{ 0 };

    juce::AudioBuffer<float>* sample{ nullptr };
    juce::Array<CustomSamplerVoice*>& synthVoices;

    juce::Value viewStart, viewEnd;
    juce::Value sampleStart, sampleEnd, loopStart, loopEnd, isLooping, loopHasStart, loopHasEnd;
    juce::Path startSamplePath, stopSamplePath;

    bool dragging{ false };
    NavigatorParts draggingTarget{ Drag::NONE };
    int dragOriginStartSample{ 0 };
};

//==============================================================================
class SampleNavigator : public CustomComponent
{
public:
    SampleNavigator(APVTS& apvts, juce::Array<CustomSamplerVoice*>& synthVoices);
    ~SampleNavigator() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void enablementChanged() override;

    void repaintUI();
    void setSample(juce::AudioBuffer<float>& sampleBuffer, bool resetUI);
private:
    APVTS& apvts;
    SamplePainter painter;
    SampleNavigatorOverlay overlay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleNavigator)
};

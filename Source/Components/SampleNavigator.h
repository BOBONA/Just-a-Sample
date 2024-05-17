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

/** A navigator control for the viewing window of the sample editor */
class SampleNavigator final : public CustomComponent, public APVTS::Listener, public juce::Value::Listener
{
    using Drag = NavigatorParts;

public:
    SampleNavigator(APVTS& apvts, const juce::Array<CustomSamplerVoice*>& synthVoices);
    ~SampleNavigator() override;

    //==============================================================================
    void setSample(const juce::AudioBuffer<float>& sampleBuffer, bool initialLoad);

    /** Call this while recording when more samples have been filled in the sampleBuffer */
    void sampleUpdated(int oldSize, int newSize);
    void setRecordingMode(bool recording);

private:
    /** React to gain changes */
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    /** React to view changes */
    void valueChanged(juce::Value& value) override;

    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;
    void enablementChanged() override;

    //==============================================================================
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

    NavigatorParts getDraggingTarget(int x, int y) const;

    /** Relative to the bounds of the painter */
    float sampleToPosition(int sampleIndex) const;
    int positionToSample(float position) const;

    //==============================================================================
    APVTS& apvts;
    SamplePainter painter;

    const juce::AudioBuffer<float>* sample{ nullptr };
    const juce::Array<CustomSamplerVoice*>& synthVoices;

    juce::Value viewStart, viewEnd;
    juce::Value sampleStart, sampleEnd, loopStart, loopEnd, isLooping, loopHasStart, loopHasEnd;
    juce::Path startSamplePath, stopSamplePath;

    bool dragging{ false };
    NavigatorParts draggingTarget{ NavigatorParts::NONE };
    int dragOriginStartSample{ 0 };

    bool recordingMode{ false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleNavigator)
};
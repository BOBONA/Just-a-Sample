/*
  ==============================================================================

    SampleComponent.h
    Created: 19 Sep 2023 2:03:29pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "../sampler/CustomSamplerVoice.h"
#include "ComponentUtils.h"
#include "SampleNavigator.h"
#include "displays/SamplePainter.h"

enum class EditorParts 
{
    NONE,
    SAMPLE_START,
    SAMPLE_END,
    LOOP_START,
    LOOP_END,
    LOOP_START_BUTTON,
    LOOP_END_BUTTON
};

class SampleEditorOverlay : public CustomComponent, public juce::Value::Listener
{
public:
    SampleEditorOverlay(APVTS& apvts, juce::Array<CustomSamplerVoice*>& synthVoices);
    ~SampleEditorOverlay() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseMove(const MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    EditorParts getClosestPartInRange(int x, int y);

    void valueChanged(juce::Value& value) override;

    float sampleToPosition(int sampleIndex);
    int positionToSample(float position);

    void setSample(juce::AudioBuffer<float>& sampleBuffer);
private:
    int painterWidth{ 0 };

    std::unique_ptr<juce::AudioBuffer<float>> sample;
    juce::Array<CustomSamplerVoice*>& synthVoices;

    juce::Value viewStart, viewEnd;
    juce::Value sampleStart, sampleEnd;
    juce::Value loopStart, loopEnd;
    juce::Path sampleStartPath, sampleEndPath;

    juce::Value isLooping, loopingHasStart, loopingHasEnd;
    juce::Path loopIcon, loopIconWithStart, loopIconWithEnd;
    juce::Path loopIconArrows;

    bool dragging{ false };
    EditorParts draggingTarget{ EditorParts::NONE };
};

//==============================================================================
class SampleEditor : public CustomComponent, public juce::ValueTree::Listener
{
public:
    SampleEditor(APVTS& apvts, juce::Array<CustomSamplerVoice*>& synthVoices);
    ~SampleEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void enablementChanged() override;

    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override;

    void repaintUI();
    void setSample(juce::AudioBuffer<float>& sample, bool resetUI);
private:
    APVTS& apvts;

    SamplePainter painter;
    SampleEditorOverlay overlay;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleEditor)
};

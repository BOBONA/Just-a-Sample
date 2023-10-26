/*
  ==============================================================================

    SampleComponent.h
    Created: 19 Sep 2023 2:03:29pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "CustomComponent.h"
#include "SamplePainter.h"
#include "SampleNavigator.h"
#include "CustomSamplerVoice.h"

//==============================================================================
enum class EditorParts {
    NONE,
    SAMPLE_START,
    SAMPLE_END,
    LOOP_START,
    LOOP_END
};

class SampleEditorOverlay : public CustomComponent, public juce::Value::Listener, public juce::MouseListener
{
public:
    SampleEditorOverlay(APVTS& apvts, juce::Array<CustomSamplerVoice*>& synthVoices);
    ~SampleEditorOverlay() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

    void valueChanged(juce::Value& value) override;
    void voiceStateChanged(CustomSamplerVoice* samplerVoice, VoiceState newState);

    float sampleToPosition(int sample);
    int positionToSample(float position);

    void setSample(juce::AudioBuffer<float>& sample);
private:
    int painterWidth{ 0 };

    juce::AudioBuffer<float>* sample{ nullptr };
    juce::Array<CustomSamplerVoice*>& synthVoices;
    std::map<CustomSamplerVoice*, juce::Path> voicePaths; // keeps track of the active voices, and their paths

    juce::Value viewStart, viewEnd;
    juce::Value sampleStart, sampleEnd;
    juce::Path sampleStartPath, sampleEndPath;

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

    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override;
    void voiceStateChanged(CustomSamplerVoice* samplerVoice, VoiceState newState);

    void updateSamplePosition();
    void setSample(juce::AudioBuffer<float>& sample, bool resetUI);
private:
    APVTS& apvts;

    SamplePainter painter;
    SampleEditorOverlay overlay;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleEditor)
};

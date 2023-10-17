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
    SampleEditorOverlay(APVTS& apvts, juce::Array<int>& voicePositions);
    ~SampleEditorOverlay() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

    void valueChanged(juce::Value& value) override;

    float sampleToPosition(int sample);
    int positionToSample(float position);

    void setSample(juce::AudioBuffer<float>& sample);
private:
    int painterWidth{ 0 };

    juce::AudioBuffer<float>* sample{ nullptr };
    juce::Array<int>& voicePositions;

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
    SampleEditor(APVTS& apvts, juce::Array<int>& voicePositions);
    ~SampleEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override;

    void updateSamplePosition();
    void setSample(juce::AudioBuffer<float>& sample, bool resetUI);
private:
    APVTS& apvts;

    SamplePainter painter;
    SampleEditorOverlay overlay;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleEditor)
};

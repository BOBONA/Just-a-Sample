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
    void enablementChanged() override;
    void mouseMove(const MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    EditorParts getClosestPartInRange(int x, int y);

    void valueChanged(juce::Value& value) override;

    float sampleToPosition(int sampleIndex);
    int positionToSample(float position);

    void setSample(juce::AudioBuffer<float>& sampleBuffer);
    void setRecordingMode(bool recording);

private:
    int painterWidth{ 0 };

    juce::AudioBuffer<float>* sample{ nullptr };
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
    bool recordingMode{ false }; // Whether the overlay should display in recording mode
};

class BoundsSelectListener
{
public:
    virtual ~BoundsSelectListener() {}
    virtual void boundsSelected(int startSample, int endSample) = 0;
};

class SampleEditor : public CustomComponent, public juce::ValueTree::Listener
{
public:
    SampleEditor(APVTS& apvts, juce::Array<CustomSamplerVoice*>& synthVoices);
    ~SampleEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void enablementChanged() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override;

    void repaintUI();
    Rectangle<int> getPainterBounds() const;
    void setSample(juce::AudioBuffer<float>& sample, bool resetUI);
    void setRecordingMode(bool recording);
    void sampleUpdated(int oldSize, int newSize); // Currently used for recording

    /** The bound select routine allows this component to double as a bounds selector for any parameters that need it */
    void boundsSelectPrompt(const String& text);
    void endBoundsSelectPrompt();
    void addBoundsSelectListener(BoundsSelectListener* listener);
    void removeBoundsSelectListener(BoundsSelectListener* listener);

private:
    APVTS& apvts;

    Label label;
    SamplePainter painter;
    SampleEditorOverlay overlay;

    bool boundsSelecting{ false };
    bool dragging{ false };
    int startLoc{ 0 }, endLoc{ 0 };
    Array<BoundsSelectListener*> boundSelectListeners;

    bool recordingMode{ false };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleEditor)
};

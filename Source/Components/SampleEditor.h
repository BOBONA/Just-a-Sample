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
    SampleEditorOverlay(APVTS& apvts, const juce::Array<CustomSamplerVoice*>& synthVoices);
    ~SampleEditorOverlay() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void enablementChanged() override;
    void mouseMove(const MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

    /** Similar to juce::jlimit<int>, limits a sample between two bounds, however takes into account
        the previousValue and MINIMUM_BOUNDS_DISTANCE to allow for a cleaner experience with minimum distances.
    */
    int limitBounds(int previousValue, int sample, int start, int end);
    EditorParts getClosestPartInRange(int x, int y);

    void valueChanged(juce::Value& value) override;

    float sampleToPosition(int sampleIndex);
    int positionToSample(float position);

    void setSample(const juce::AudioBuffer<float>& sampleBuffer);
    void setRecordingMode(bool recording);

private:
    const juce::AudioBuffer<float>* sample{ nullptr };
    const juce::Array<CustomSamplerVoice*>& synthVoices;

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

    int painterWidth{ 0 };
};

class BoundsSelectListener
{
public:
    virtual ~BoundsSelectListener() {}
    virtual void boundsSelected(int startSample, int endSample) = 0;
};

class SampleEditor : public CustomComponent, public juce::ValueTree::Listener, public APVTS::Listener
{
public:
    SampleEditor(APVTS& apvts, const juce::Array<CustomSamplerVoice*>& synthVoices);
    ~SampleEditor() override;

    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void enablementChanged() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    void repaintUI();
    Rectangle<int> getPainterBounds() const;
    void setSample(const juce::AudioBuffer<float>& sample, bool initialLoad);
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

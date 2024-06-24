/*
  ==============================================================================

    SampleEditorOverlay.h
    Created: 19 Sep 2023 2:03:29pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "../sampler/CustomSamplerVoice.h"
#include "ComponentUtils.h"
#include "RangeSelector.h"
#include "displays/SamplePainter.h"

/** An enum of selectable parts of the editor overlay. */
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

/** The overlay draws the bounds and active voices, to be placed over the main editor object */
class SampleEditorOverlay final : public CustomComponent, public juce::AudioProcessorParameter::Listener, public ValueListener<int>
{
public:
    SampleEditorOverlay(const APVTS& apvts, PluginParameters::State& pluginState, const juce::Array<CustomSamplerVoice*>& synthVoices);
    ~SampleEditorOverlay() override;

    void setSample(const juce::AudioBuffer<float>& sample);
    void setRecordingMode(bool recording);

    /** Utility functions */
    float sampleToPosition(int sampleIndex) const;
    int positionToSample(float position) const;

private:
    void valueChanged(ListenableValue<int>& source, int newValue) override;
    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}

    void paint(juce::Graphics&) override;
    void resized() override;
    void enablementChanged() override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

    /** Similar to juce::jlimit<int>, limits a sample between two bounds, however takes into account
        the previousValue and MINIMUM_BOUNDS_DISTANCE to allow for a cleaner experience with minimum distances.
    */
    int limitBounds(int previousValue, int sample, int start, int end) const;
    EditorParts getClosestPartInRange(int x, int y) const;

    //==============================================================================
    const juce::AudioBuffer<float>* sampleBuffer{ nullptr };
    const juce::Array<CustomSamplerVoice*>& synthVoices;

    ListenableAtomic<int>& viewStart, & viewEnd, & sampleStart, & sampleEnd, & loopStart, & loopEnd;
    juce::Path sampleStartPath, sampleEndPath;

    juce::AudioParameterBool* isLooping, * loopingHasStart, * loopingHasEnd;
    juce::Path loopIcon, loopIconWithStart, loopIconWithEnd;
    juce::Path loopIconArrows;

    bool dragging{ false };
    EditorParts draggingTarget{ EditorParts::NONE };
    bool recordingMode{ false };  // Whether the overlay should display in recording mode

    int painterWidth{ 0 };
};

/*
  ==============================================================================

    SampleEditor.h
    Created: 19 Sep 2023 2:03:29pm
    Author:  binya

  ==============================================================================
*/

/** The SampleEditor is the main component responsible for editing a sample. It allows for setting
    the bounds for playback and looping. It also allows for bounds selection for operations that need it.
    It reacts to viewport changes from the sample navigator. Like the navigator, it also displays active voices.
*/
class SampleEditor final : public CustomComponent, public APVTS::Listener, public ValueListener<int>
{
public:
    SampleEditor(APVTS& apvts, PluginParameters::State& pluginState, const juce::Array<CustomSamplerVoice*>& synthVoices);
    ~SampleEditor() override;

    //==============================================================================
    void setSample(const juce::AudioBuffer<float>& sample, bool initialLoad);

    /** Recording mode hides the bounds selection and turns the editor into a view only
        display while a recording is in progress.
    */
    void setRecordingMode(bool recording);
    bool isRecordingMode() const;
    void sampleUpdated(int oldSize, int newSize); // Currently used for recording

    //==============================================================================
    /** Prompts the user to select a range of samples within the current viewport. */
    void promptBoundsSelection(const juce::String& text, const std::function<void(int startSample, int endSample)>& callback);
    void cancelBoundsSelection();
    bool isInBoundsSelection() const;

private:
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void valueChanged(ListenableValue<int>& source, int newValue) override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void enablementChanged() override;

    juce::Rectangle<int> getPainterBounds() const;

    //==============================================================================
    APVTS& apvts;
    PluginParameters::State& pluginState;

    SamplePainter painter;
    SampleEditorOverlay overlay;
    RangeSelector boundsSelector;

    bool recordingMode{ false };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleEditor)
};

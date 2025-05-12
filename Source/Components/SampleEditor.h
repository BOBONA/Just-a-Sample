/*
  ==============================================================================

    SampleEditorOverlay.h
    Created: 19 Sep 2023 2:03:29pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "../Sampler/CustomSamplerVoice.h"
#include "../Utilities/ComponentUtils.h"
#include "RangeSelector.h"
#include "Displays/SamplePainter.h"

/** An enum of selectable parts of the editor overlay. */
enum class EditorParts 
{
    NONE,
    SAMPLE_START,
    SAMPLE_END,
    LOOP_START,
    LOOP_END,
};

/** The overlay draws the bounds and active voices, to be placed over the main editor object */
class SampleEditorOverlay final : public CustomComponent, public ValueListener<int>
{
public:
    SampleEditorOverlay(const APVTS& apvts, PluginParameters::State& pluginState, const juce::Array<CustomSamplerVoice*>& synthVoices, UIDummyParam& dummy, juce::Component* forwardEventsTo = nullptr);
    ~SampleEditorOverlay() override;

    void setSample(const juce::AudioBuffer<float>& sample, float bufferSampleRate);

    /** Utility functions */
    float sampleToPosition(int sampleIndex) const;
    int positionToSample(float position) const;

    bool isWaveformMode() const;

private:
    void valueChanged(ListenableValue<int>& source, int newValue) override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void enablementChanged() override;

    void mouseMove(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent&) override { repaint(); };
    void mouseExit(const juce::MouseEvent&) override { repaint(); };

    juce::String getCustomHelpText() override;

    EditorParts getClosestPartInRange(int x, int y) const;
    float getBoundsWidth() const;

    //==============================================================================
    const juce::AudioBuffer<float>* sampleBuffer{ nullptr };
    float sampleRate{ 0.f };
    const juce::Array<CustomSamplerVoice*>& synthVoices;
    UIDummyParam& dummyParam;

    ListenableAtomic<int>& viewStart, & viewEnd, & sampleStart, & sampleEnd, & loopStart, & loopEnd;
    ListenableAtomic<bool>& pinnedBounds;
    juce::AudioParameterBool* isLooping, * loopingHasStart, * loopingHasEnd;
    juce::ParameterAttachment isLoopingAttachment, loopingHasStartAttachment, loopingHasEndAttachment;

    bool dragging{ false };
    EditorParts draggingTarget{ EditorParts::NONE };

    melatonin::DropShadow boundsShadow{ {{Colors::SLATE.withAlpha(0.25f), 2, {1, 0}}, {Colors::SLATE.withAlpha(0.25f), 2, {-1, 0}}} };
    melatonin::DropShadow loopBoundsShadow{ {{Colors::LOOP.withAlpha(0.25f), 2, {1, 0}}, {Colors::LOOP.withAlpha(0.25f), 2, {-1, 0}}} };
    melatonin::InnerShadow innerShadow{{Colors::SLATE.withAlpha(0.25f), 3, {0, 2}}, {Colors::SLATE.withAlpha(0.25f), 3, {0, -2}}};

    juce::Path handleLeft, handleRight;

    /** The overlay can be provided with a component to forward unhandled mouse events to.
        This currently only forwards particular mouse events, but can be expanded if we need.
     */
    juce::Component* forwardMouseEvents{ nullptr };
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
class SampleEditor final : public CustomComponent, public ValueListener<int>
{
public:
    /** The SampleEditor requires reference to the apvts, pluginState, synthVoices, and a
        function reference to scroll the navigator (navigator.scrollView).
    */
    SampleEditor(APVTS& apvts, PluginParameters::State& pluginState, const juce::Array<CustomSamplerVoice*>& synthVoices, 
        const std::function<void(const juce::MouseWheelDetails& details, int centerSample)>& navScrollFunc);
    ~SampleEditor() override;

    //==============================================================================
    void setSample(const juce::AudioBuffer<float>& sample, float bufferSampleRate, bool resetView);

    /** Recording mode hides the bounds selection and turns the editor into a view only
        display while a recording is in progress.
    */
    void setRecordingMode(bool recording);
    bool isRecordingMode() const;
    void sampleUpdated(int oldSize, int newSize); // Currently used for recording

    //==============================================================================
    /** Prompts the user to select a range of samples within the current viewport. */
    void promptBoundsSelection(const std::function<void(int startSample, int endSample)>& callback);
    void cancelBoundsSelection();
    bool isInBoundsSelection() const;

private:
    void valueChanged(ListenableValue<int>& source, int newValue) override;

    void resized() override;
    void enablementChanged() override;

    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    int positionToSample(float position) const;

    //==============================================================================
    APVTS& apvts;
    PluginParameters::State& pluginState;
    UIDummyParam dummyParam;
    const juce::AudioBuffer<float>* sampleBuffer{ nullptr };
    float sampleRate{ 0.f };

    SamplePainter painter;
    juce::ParameterAttachment gainAttachment;
    juce::ParameterAttachment monoAttachment;

    SampleEditorOverlay overlay;
    RangeSelector boundsSelector;

    bool recordingMode{ false };

    std::function<void(const juce::MouseWheelDetails& details, int centerSample)> scrollFunc;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleEditor)
};

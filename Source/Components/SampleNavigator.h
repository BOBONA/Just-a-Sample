/*
  ==============================================================================

    SampleNavigator.h
    Created: 19 Sep 2023 4:41:12pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "../Sampler/CustomSamplerVoice.h"
#include "../Utilities/ComponentUtils.h"
#include "Displays/SamplePainter.h"

enum class NavigatorParts
{
    NONE,
    SAMPLE_START,
    SAMPLE_END,
    SAMPLE_FULL
};

/** A navigator control for the viewing window of the sample editor */
class SampleNavigator final : public CustomComponent, public ValueListener<int>, public ValueListener<bool>
{
    using Drag = NavigatorParts;

public:
    SampleNavigator(APVTS& apvts, PluginParameters::State& pluginState, const juce::OwnedArray<CustomSamplerVoice>& synthVoices);
    ~SampleNavigator() override;

    //==============================================================================
    void setSample(const juce::AudioBuffer<float>& sampleBuffer, float bufferSampleRate, bool resetView);

    /** Call this while recording when more samples have been filled in the sampleBuffer */
    void sampleUpdated(int oldSize, int newSize);
    void setRecordingMode(bool recording);

    /** Scrolling can be centered on a sample */
    void scrollView(const juce::MouseWheelDetails& wheel, int sampleCenter, bool centerZoomOut = false);

    /** Set the view to fit the sample play bounds */
    void fitView();

private:
    /** React to view changes */
    void valueChanged(ListenableValue<int>& source, int newValue) override;
    void valueChanged(ListenableValue<bool>& source, bool newValue) override;

    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;
    void lookAndFeelChanged() override;
    void enablementChanged() override;

    //==============================================================================
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

    /** Move the view start, maintaining constraints */
    void moveStart(float change, float sensitivity);
    void moveEnd(float change, float sensitivity);
    void moveBoth(float change, float sensitivity);

    NavigatorParts getDraggingTarget(int x, int y) const;

    /** Relative to the bounds of the painter */
    float sampleToPosition(int sampleIndex) const;
    int positionToSample(float position) const;

    /** The curve I found here is somewhat arbitrary but feels nice...
        checkSecondary looks at the secondary keys to decide to do the secondary response.
        If checkSecondary, useSecondary is ignored.
     */
    float getDragSensitivity(bool checkSecondary = true, bool useSecondary = false) const;

    /** In some cases we'd like to adjust the loop start/end positions when they are enabled */
    void loopHasStartUpdate(bool newValue);
    void loopHasEndUpdate(bool newValue);

    /** Update the stored position ratios, for use when pinned */
    void updatePinnedPositions();
    void moveBoundsToPinnedPositions(bool startToEnd);

    bool isWaveformMode() const;

    //==============================================================================
    APVTS& apvts;
    PluginParameters::State& state;
    UIDummyParam dummyParam;
    SamplePainter painter;
    juce::ParameterAttachment gainAttachment;
    juce::ParameterAttachment monoAttachment;

    const juce::AudioBuffer<float>* sample{ nullptr };
    float sampleRate;
    const juce::OwnedArray<CustomSamplerVoice>& synthVoices;

    juce::AudioParameterBool* isLooping, * loopHasStart, * loopHasEnd;
    juce::ParameterAttachment loopAttachment, loopStartAttachment, loopEndAttachment;

    bool dragging{ false };
    NavigatorParts draggingTarget{ NavigatorParts::NONE };
    float dragSelectOffset{ 0 };  // When dragging full, this is the offset from the view start where the drag started
    int lastDragOffset{ 0 };

    bool recordingMode{ false };

    // To keep the bounds in a consistent location when pinned, we need to store the ratios (necessary because zooming in more
    // causes rounding issues and the position moves drastically).
    long double pinnedSampleStart{ 0.f }, pinnedSampleEnd{ 0.f }, pinnedLoopStart{ 0.f }, pinnedLoopEnd{ 0.f };
    bool navigatorUpdate{ false };  // This flag allows the navigator to avoid updating the pinned positions when they are being used to update the bounds

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleNavigator)
};
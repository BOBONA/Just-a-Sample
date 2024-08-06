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
class SampleNavigator final : public CustomComponent, public ValueListener<int>
{
    using Drag = NavigatorParts;

public:
    SampleNavigator(APVTS& apvts, PluginParameters::State& pluginState, const juce::Array<CustomSamplerVoice*>& synthVoices);
    ~SampleNavigator() override;

    //==============================================================================
    void setSample(const juce::AudioBuffer<float>& sampleBuffer, bool initialLoad);

    /** Call this while recording when more samples have been filled in the sampleBuffer */
    void sampleUpdated(int oldSize, int newSize);
    void setRecordingMode(bool recording);

    /** Scrolling can be centered on a sample */
    void scrollView(const juce::MouseWheelDetails& wheel, int sampleCenter, bool centerZoomOut = false) const;

    /** Set the view to fit the sample play bounds */
    void fitView();

private:
    /** React to view changes */
    void valueChanged(ListenableValue<int>& source, int newValue) override;

    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;
    void enablementChanged() override;

    //==============================================================================
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    /** Move the view start, maintaining constraints */
    void moveStart(float change, float sensitivity) const;
    void moveEnd(float change, float sensitivity) const;
    void moveBoth(float change, float sensitivity) const;

    NavigatorParts getDraggingTarget(int x, int y) const;

    /** Relative to the bounds of the painter */
    float sampleToPosition(int sampleIndex) const;
    int positionToSample(float position) const;

    /** The curve I found here is somewhat arbitrary but feels nice...
        checkSecondary looks at the secondary keys to decide to do the secondary response.
        If checkSecondary, useSecondary is ignored.
     */
    float getDragSensitivity(bool checkSecondary = true, bool useSecondary = false) const;

    //==============================================================================
    APVTS& apvts;
    PluginParameters::State& state;
    SamplePainter painter;
    juce::ParameterAttachment gainAttachment;

    const juce::AudioBuffer<float>* sample{ nullptr };
    const juce::Array<CustomSamplerVoice*>& synthVoices;

    juce::AudioParameterBool* isLooping, * loopHasStart, * loopHasEnd;

    bool dragging{ false };
    NavigatorParts draggingTarget{ NavigatorParts::NONE };
    float dragSelectOffset{ 0 };  // When dragging full, this is the offset from the view start where the drag started
    float lastDragOffset{ 0 };

    bool recordingMode{ false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleNavigator)
};
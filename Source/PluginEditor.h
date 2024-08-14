/*
  ==============================================================================

    PluginProcessor.cpp
    Created: 5 Sep 2023
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "CustomLookAndFeel.h"
#include "Components/Buttons.h"
#include "Components/SampleEditor.h"
#include "Components/FxChain.h"
#include "Components/Prompt.h"
#include "Components/SampleLoaderArea.h"
#include "Components/SampleNavigator.h"

#include "melatonin_inspector/melatonin_inspector.h"

/** Painting ordering requires a separate component... */
class EditorOverlay final : public CustomComponent
{
    void paint(juce::Graphics& g) override;
    void resized() override;

    float scale(float value) const { return value * getWidth() / Layout::figmaWidth; }

    melatonin::DropShadow sampleControlShadow{ Colors::SLATE.withAlpha(0.125f), 3, {2, 2} };
    melatonin::DropShadow navControlShadow{ Colors::SLATE.withAlpha(0.25f), 3, {-2, -2} };
};

class JustaSampleAudioProcessorEditor final : public juce::AudioProcessorEditor, public juce::Timer, public juce::FileDragAndDropTarget, 
                                              public juce::FilenameComponentListener
{
public:
    explicit JustaSampleAudioProcessorEditor(JustaSampleAudioProcessor& audioProcessor);
    ~JustaSampleAudioProcessorEditor() override;

private:
    void timerCallback() override;
    void paint(juce::Graphics&) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

    //==============================================================================
    /** Update the Editor to fit with the processor's sample. On the initial load, the
        SampleNavigator will not update the viewing bounds.
    */
    void loadSample();

    /** Handles the incoming recording messages from the recorder queue */
    void handleActiveRecording();

    /** If permitted, toggles whether the plugin state will use a file reference or store the 
        samples directly. If necessary, this will prompt the user to save the current sample.
    */
    void toggleLinkSample();

    /** Signals to the processor to start recording, or first opens the device settings prompt if 
        no valid input device is selected. Note that no editor changes occur here, instead it polls
        for changes in the timer callback (simpler than registering as a listener to the recorder).
    */
    void startRecording(bool promptSettings = true);

    /** Starts a pitch detection prompt */
    void promptPitchDetection();

    //==============================================================================
    /** Whether the editor is interested in a file */
    bool isInterestedInFileDrag(const juce::String& file);
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void filenameComponentChanged(juce::FilenameComponent* fileComponentThatHasChanged) override;

    //==============================================================================
    /** Scaling the sizes in our Figma demo to percentages of width. */
    int scale(float value) const { return int(std::round(value * getWidth() / Layout::figmaWidth)); }
    float scalef(float value) const { return value * getWidth() / Layout::figmaWidth; }

    //==============================================================================
    JustaSampleAudioProcessor& p;
    PluginParameters::State& pluginState;
    const juce::Array<CustomSamplerVoice*>& synthVoices;
    bool currentlyPlaying{ false };

    /** Some thought is needed to keep the editor synchronized when changes to the sample occur
        or files are loaded. I decided the easiest way is to have the processor and editor both
        keep track of the buffer's hash.
    */
    juce::String expectedHash{ 0 };

    //==============================================================================
    juce::AudioBuffer<float> pendingRecordingBuffer;
    int recordingBufferSize{ 0 };

    //==============================================================================
    // Modules
    juce::Label tuningLabel, attackLabel, releaseLabel, playbackLabel, loopingLabel, masterLabel;
    juce::Array<juce::Slider*> rotaries;

    // Tuning module
    juce::Slider semitoneRotary, centRotary;
    APVTS::SliderAttachment semitoneSliderAttachment, centSliderAttachment;

    juce::Label tuningDetectLabel;
    juce::Path tuningDetectIcon;
    juce::ShapeButton tuningDetectButton;

    // Attack and release modules
    juce::Slider attackTimeRotary, attackCurve, releaseTimeRotary, releaseCurve;
    APVTS::SliderAttachment attackTimeAttachment, attackCurveAttachment, releaseTimeAttachment, releaseCurveAttachment;
    EnvelopeSliderLookAndFeel<EnvelopeSlider::attack> attackCurveLNF;
    EnvelopeSliderLookAndFeel<EnvelopeSlider::release> releaseCurveLNF;

    // Playback module
    CustomToggleableButton lofiModeButton;
    APVTS::ButtonAttachment lofiModeAttachment;
    CustomChoiceButton playbackModeButton;
    juce::Slider playbackSpeedRotary;
    APVTS::SliderAttachment playbackSpeedAttachment;

    // Loop module
    CustomToggleableButton loopButton, loopStartButton, loopEndButton;
    APVTS::ButtonAttachment loopAttachment, loopStartAttachment, loopEndAttachment;

    // Master module
    CustomToggleableButton monoOutputButton;
    APVTS::ButtonAttachment monoOutputAttachment;
    juce::Slider gainSlider;
    APVTS::SliderAttachment gainSliderAttachment;
    VolumeSliderLookAndFeel gainSliderLNF;

    // Sample controls
    EditorOverlay editorOverlay;
    juce::FilenameComponent filenameComponent;
    CustomToggleableButton linkSampleToggle;  // Whether the sample should be stored in the plugin state

    juce::Path playPath, stopPath;
    juce::ShapeButton playStopButton;
    juce::ShapeButton recordButton;
    juce::ShapeButton deviceSettingsButton;
    juce::AudioDeviceSelectorComponent audioDeviceSettings;

    // Nav controls
    juce::ShapeButton fitButton;
    CustomToggleableButton pinButton;
    ToggleButtonAttachment pinButtonAttachment;

    // Main components
    SampleLoaderArea sampleLoader;
    SampleEditor sampleEditor;

    SampleNavigator sampleNavigator;  // Note that SampleNavigator manages ViewStart and ViewEnd
    FxChain fxChain;

    Prompt prompt;
    juce::TooltipWindow tooltipWindow;

    CustomLookAndFeel& lnf;
    juce::OpenGLContext openGLContext;
    juce::PluginHostType hostType;

#if JUCE_DEBUG
    melatonin::Inspector inspector{ *this, false };
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JustaSampleAudioProcessorEditor)
};

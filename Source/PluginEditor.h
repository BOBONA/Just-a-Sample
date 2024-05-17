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
#include "Components/SampleEditor.h"
#include "Components/FxChain.h"
#include "Components/Prompt.h"
#include "Components/SampleNavigator.h"

class JustaSampleAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer, public juce::FileDragAndDropTarget, 
                                        public juce::FilenameComponentListener
{
public:
    JustaSampleAudioProcessorEditor(JustaSampleAudioProcessor& processor);
    ~JustaSampleAudioProcessorEditor() override;

private:
    void timerCallback() override;
    void paint(juce::Graphics&) override;
    void resized() override;

    //==============================================================================
    /** Update the Editor to fit with the processor's sample, if initialLoad is set to the true,
        then the SampleNavigator will not update the viewing bounds (since this corresponds to a
        saved sample that was loaded)
    */
    void loadSample(bool initialLoad = false);

    /** Handles the incoming recording messages from the recorder queue */
    void handleActiveRecording();

    /** If permitted, toggles whether the plugin state will use a file reference or store the 
        samples directly. If necessary, this will prompt the user to save the current sample.
    */
    void toggleStoreSample();

    /** Signals to the processor to start recording, or first opens the device settings prompt if 
        no valid input device is selected. Note that no editor changes occur here, instead it polls
        for changes in the timer callback (simpler than registering as a listener to the recorder).
    */
    void startRecording(bool promptSettings = true);

    /** Starts a pitch detection prompt, which  */
    void promptPitchDetection();

    void setSampleControlsEnabled(bool enablement);

    //==============================================================================
    /** Whether the editor is interested in a file */
    bool isInterestedInFileDrag(const juce::String& file);
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void filenameComponentChanged(juce::FilenameComponent* fileComponentThatHasChanged) override;

    //==============================================================================
    JustaSampleAudioProcessor& p;
    const juce::Array<CustomSamplerVoice*>& synthVoices;
    bool currentlyPlaying{ false };

    /** Some thought is needed to keep the editor synchronized when changes to the sample occur
        or files are loaded. I decided the easiest way is to have the processor and editor both
        keep track of the buffer's hash.
    */
    juce::String expectedHash{ 0 }; 

    //==============================================================================
    juce::Array<Component*> sampleRequiredControls;  // Controls that should be disabled when no sample is loaded

    juce::AudioBuffer<float> pendingRecordingBuffer;
    int recordingBufferSize{ 0 };

    //==============================================================================
    // Preset management
    juce::FilenameComponent filenameComponent;
    juce::ShapeButton storeSampleToggle; // Whether the sample should be stored in the plugin state

    // Tuning
    juce::Label tuningLabel;
    juce::Slider semitoneSlider;
    juce::Slider centSlider;
    juce::ShapeButton magicPitchButton;
    juce::Path magicPitchButtonShape;

    // Recording
    juce::ShapeButton recordButton;
    juce::ShapeButton deviceSettingsButton;
    juce::AudioDeviceSelectorComponent audioDeviceSettings;

    // Playback controls
    juce::ComboBox playbackOptions;
    juce::Label isLoopingLabel;
    juce::ToggleButton isLoopingButton;
    juce::Slider masterGainSlider;
    juce::ShapeButton haltButton;

    APVTS::SliderAttachment semitoneSliderAttachment, centSliderAttachment;
    APVTS::ComboBoxAttachment playbackOptionsAttachment;
    APVTS::ButtonAttachment loopToggleButtonAttachment;
    APVTS::SliderAttachment masterGainSliderAttachment;

    // Main components
    SampleEditor sampleEditor;
    SampleNavigator sampleNavigator;  // Note that SampleNavigator manages ViewStart and ViewEnd
    FxChain fxChain;

    Prompt prompt;
    juce::TooltipWindow tooltipWindow;

    CustomLookAndFeel& lnf;
    juce::OpenGLContext openGLContext;
    juce::PluginHostType hostType;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JustaSampleAudioProcessorEditor)
};

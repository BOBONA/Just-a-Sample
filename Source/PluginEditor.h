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
#include "PluginParameters.h"
#include "Components/SampleEditor.h"
#include "Components/FxChain.h"
#include "Components/Paths.h"
#include "Components/Prompt.h"

class JustaSampleAudioProcessorEditor : public AudioProcessorEditor, public Timer, public FileDragAndDropTarget, 
    public FilenameComponentListener, public BoundsSelectListener
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

    /** Callback after bounds are selected for pitch detection */
    void boundsSelected(int startSample, int endSample) override;

    void setSampleControlsEnabled(bool enablement);

    //==============================================================================
    /** Whether the editor is interested in a file */
    bool isInterestedInFileDrag(const String& file);
    bool isInterestedInFileDrag(const StringArray& files) override;
    void filesDropped(const StringArray& files, int x, int y) override;
    void filenameComponentChanged(FilenameComponent* fileComponentThatHasChanged) override;

    //==============================================================================
    JustaSampleAudioProcessor& p;
    const Array<CustomSamplerVoice*>& synthVoices;
    bool currentlyPlaying{ false };

    /** Some thought is needed to keep the editor synchronized when changes to the sample occur
        or files are loaded. I decided the easiest way is to have the processor and editor both
        keep track of the buffer's hash.
    */
    String expectedHash{ 0 }; 

    //==============================================================================
    Array<Component*> sampleRequiredControls;  // Controls that should be disabled when no sample is loaded

    AudioBuffer<float> pendingRecordingBuffer;
    int recordingBufferSize{ 0 };

    //==============================================================================
    // Preset management
    FilenameComponent filenameComponent;
    ShapeButton storeSampleToggle; // Whether the sample should be stored in the plugin state

    // Tuning
    Label tuningLabel;
    Slider semitoneSlider;
    Slider centSlider;
    ShapeButton magicPitchButton;
    Path magicPitchButtonShape;

    // Recording
    ShapeButton recordButton;
    ShapeButton deviceSettingsButton;
    AudioDeviceSelectorComponent audioDeviceSettings;

    // Playback controls
    ComboBox playbackOptions;
    Label isLoopingLabel;
    ToggleButton isLoopingButton;
    Slider masterGainSlider;
    ShapeButton haltButton;

    APVTS::SliderAttachment semitoneSliderAttachment, centSliderAttachment;
    APVTS::ComboBoxAttachment playbackOptionsAttachment;
    APVTS::ButtonAttachment loopToggleButtonAttachment;
    APVTS::SliderAttachment masterGainSliderAttachment;

    // Main components
    SampleEditor sampleEditor;
    SampleNavigator sampleNavigator;  // Note that SampleNavigator manages ViewStart and ViewEnd
    FxChain fxChain;

    Prompt prompt;
    TooltipWindow tooltipWindow;

    CustomLookAndFeel& lnf;
    OpenGLContext openGLContext;
    PluginHostType hostType;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JustaSampleAudioProcessorEditor)
};

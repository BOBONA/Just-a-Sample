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
#include "components/SampleEditor.h"
#include "components/FxChain.h"
#include "Components/Paths.h"

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

    void handleActiveRecording();
    void toggleStoreSample();

    void setSampleControlsEnabled(bool enablement);

    //==============================================================================
    void boundsSelected(int startSample, int endSample) override; // currently for pitch detection
    bool keyPressed(const KeyPress& key) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    /** A translucent background for when a prompt is visible */
    void showPromptBackground(Array<Component*> visibleComponents = {});
    void hidePromptBackground();
    void onPromptExit();

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
    // Components
    Array<Component*> sampleRequiredControls;  // Controls that should be disabled when no sample is loaded
    bool layoutDirty{ false };  // Set to true to resize on next timer callback

    // Preset management
    FilenameComponent filenameComponent;
    ShapeButton storeSampleToggle; // Whether the sample should be stored in the plugin state

    // Tuning
    Label tuningLabel;
    Slider semitoneSlider;
    Slider centSlider;
    ShapeButton magicPitchButton;
    Path magicPitchButtonShape;

    // Controls
    ShapeButton recordButton;
    ShapeButton deviceSettingsButton;
    AudioDeviceSelectorComponent audioDeviceSettings;

    // Playback
    ComboBox playbackOptions;
    Label isLoopingLabel;
    ToggleButton isLoopingButton;
    Slider masterGainSlider;
    ShapeButton haltButton;

    SampleEditor sampleEditor;
    SampleNavigator sampleNavigator;  // Note that SampleNavigator manages ViewStart and ViewEnd

    bool boundsSelectRoutine{ false }; // this is set to true once the user is prompted to select a bounds region for pitch detection
    bool recordingPrompt{ false }; // this is to keep track of when a user was prompted to settings after pressing record
    AudioBuffer<float> pendingRecordingBuffer;
    int recordingBufferSize{ 0 };

    DrawableRectangle promptBackground;
    Array<Component*> promptVisibleComponents;
    bool promptBackgroundVisible{ false };
    bool firstMouseUp{ false }; // necessary since a mouseup outside relevant areas should cancel prompts

    FxChain fxChain;

    APVTS::SliderAttachment semitoneSliderAttachment, centSliderAttachment;
    APVTS::ComboBoxAttachment playbackOptionsAttachment;
    APVTS::ButtonAttachment loopToggleButtonAttachment;
    APVTS::SliderAttachment masterGainSliderAttachment;

    std::vector<String> listeningParameters;

    CustomLookAndFeel& lnf;

    TooltipWindow tooltipWindow;
    OpenGLContext openGLContext;
    PluginHostType hostType;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JustaSampleAudioProcessorEditor)
};

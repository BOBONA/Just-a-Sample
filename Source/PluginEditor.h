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
#include "Components/CustomAudioDeviceSelector.h"
#include "Components/SampleEditor.h"
#include "Components/FxChain.h"
#include "Components/Prompt.h"
#include "Components/SampleNavigator.h"

/** Painting ordering requires a separate component... */
class EditorOverlay final : public CustomComponent
{
public:
    void setWaveformMode(bool isWaveformMode)
    {
        if (waveformMode != isWaveformMode)
        {
            waveformMode = isWaveformMode;
            repaint();
        }
    }

private:
    void paint(juce::Graphics& g) override;
    void resized() override;

    float scale(float value) const { return value * getWidth() / Layout::figmaWidth; }

    //==============================================================================
    melatonin::DropShadow sampleControlShadow{ Colors::SLATE.withAlpha(0.125f), 3, {2, 2} };
    melatonin::DropShadow navControlShadow{ Colors::SLATE.withAlpha(0.25f), 3, {-2, -2} };

    bool waveformMode{ false };
};

//==============================================================================
class JustaSampleAudioProcessorEditor final : public juce::AudioProcessorEditor, public juce::Timer, public juce::FileDragAndDropTarget, 
                                              public juce::FilenameComponentListener, public CustomHelpTextDisplay
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

    /** We need to handle mouse move events for the editor to update the help text. */
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void helpTextChanged(const juce::String& newText) override;

    bool keyPressed(const juce::KeyPress& key) override;

    juce::Rectangle<int> getConstrainedBounds() const;

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

    /** Opens the device settings prompt */
    void promptDeviceSettings(bool recordOnClose = false);

    /** We use this as a place to update the enablement and display of our plugin controls. */
    void enablementChanged() override;

    //==============================================================================
    /** Whether the editor is interested in a file */
    bool isInterestedInFile(const juce::String& file) const;
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void filenameComponentChanged(juce::FilenameComponent* fileComponentThatHasChanged) override;

    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;

    void updateLabel(const juce::String& text = "");

    //==============================================================================
    /** Scaling the sizes in our Figma demo to percentages of width. */
    int scale(float value) const { return int(std::round(value * prompt.getWidth() / Layout::figmaWidth)); }
    float scalef(float value) const { return value * prompt.getWidth() / Layout::figmaWidth; }

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
    CustomRotary semitoneRotary, centRotary, waveformSemitoneRotary, waveformCentRotary;
    CustomRotaryAttachment semitoneRotaryAttachment, centRotaryAttachment, waveformSemitoneRotaryAttachment, waveformCentRotaryAttachment;

    juce::Label tuningDetectLabel;
    CustomShapeButton tuningDetectButton;

    // Attack and release modules
    CustomRotary attackTimeRotary, attackCurve, releaseTimeRotary, releaseCurve;
    CustomRotaryAttachment attackTimeAttachment, attackCurveAttachment, releaseTimeAttachment, releaseCurveAttachment;
    EnvelopeSliderLookAndFeel<EnvelopeSlider::attack> attackCurveLNF;
    EnvelopeSliderLookAndFeel<EnvelopeSlider::release> releaseCurveLNF;

    // Playback module
    CustomToggleableButton lofiModeButton;
    APVTS::ButtonAttachment lofiModeAttachment;
    CustomChoiceButton playbackModeButton;
    juce::ParameterAttachment playbackModeAttachment;
    CustomRotary playbackSpeedRotary;
    CustomRotaryAttachment playbackSpeedAttachment;

    // Loop module
    CustomToggleableButton loopButton, loopStartButton, loopEndButton;
    APVTS::ButtonAttachment loopAttachment, loopStartAttachment, loopEndAttachment;

    // Master module
    CustomToggleableButton monoOutputButton;
    APVTS::ButtonAttachment monoOutputAttachment;
    CustomRotary gainSlider;
    CustomRotaryAttachment gainSliderAttachment;
    VolumeSliderLookAndFeel gainSliderLNF;

    // Sample controls
    EditorOverlay editorOverlay;
    juce::FilenameComponent filenameComponent;
    CustomToggleableButton linkSampleToggle;  // Whether the sample should be stored in the plugin state
    juce::Label waveformModeLabel;

    CustomShapeButton playStopButton;
    juce::Path playPath, stopPath;
    const juce::String& playHelpText{ "Play sample" }, stopHelpText{ "Halt voices" };
    CustomShapeButton recordButton;
    CustomShapeButton deviceSettingsButton;
    CustomAudioDeviceSelector audioDeviceSettings;

    // Nav controls
    CustomShapeButton fitButton;
    CustomToggleableButton pinButton;
    ToggleButtonAttachment pinButtonAttachment;

    // Main components
    SampleEditor sampleEditor;
    SampleNavigator sampleNavigator;  // Note that SampleNavigator manages ViewStart and ViewEnd
    FxChain fxChain;

    juce::Label statusLabel;
    const juce::String& defaultMessage{ "Welcome!" };
    bool fileDragging{ false };
  
    // Footer
    CustomShapeButton logo;
    juce::Label helpText;
    CustomToggleableButton showFXButton;
    ToggleButtonAttachment showFXAttachment;
    juce::ParameterAttachment eqEnablementAttachment, reverbEnablementAttachment, distortionEnablementAttachment, chorusEnablementAttachment;
    bool eqEnabled{ false }, reverbEnabled{ false }, distortionEnabled{ false }, chorusEnabled{ false };
    bool initialSizing{ true };

    Prompt prompt;

    CustomLookAndFeel& lnf;
    juce::OpenGLContext openGLContext;
    juce::PluginHostType hostType;

#if JUCE_DEBUG 
    melatonin::Inspector inspector{ *this, false };
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JustaSampleAudioProcessorEditor)
};

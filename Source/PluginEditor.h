#pragma once
#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "CustomLookAndFeel.h"
#include "PluginParameters.h"
#include "components/SampleEditor.h"
#include "components/FxChain.h"
#include "Components/Paths.h"

class JustaSampleAudioProcessorEditor : public AudioProcessorEditor, public Timer, public FileDragAndDropTarget, 
    public ValueTree::Listener, public APVTS::Listener, public FilenameComponentListener, public BoundsSelectListener
{
public:
    JustaSampleAudioProcessorEditor (JustaSampleAudioProcessor&);
    ~JustaSampleAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void boundsSelected(int startSample, int endSample) override; // currently for pitch detection
    bool keyPressed(const KeyPress& key) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    /** A translucent background for when a prompt is visible */
    void showPromptBackground(Array<Component*> visibleComponents = {});
    void hidePromptBackground();
    void onPromptExit();

    bool isInterestedInFileDrag(const String& file);
    bool isInterestedInFileDrag(const StringArray& files) override;
    void filesDropped(const StringArray& files, int x, int y) override;
    void filenameComponentChanged(FilenameComponent* fileComponentThatHasChanged) override;

    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override;
    void parameterChanged(const String& parameterID, float newValue) override;
    void addListeningParameters(std::vector<String> parameters);

    void updateWorkingSample(bool resetUI = false);

private:
    JustaSampleAudioProcessor& processor;
    Array<CustomSamplerVoice*>& synthVoices;
    bool currentlyPlaying{ false };

    // Components
    Array<Component*> sampleRequiredControls;
    bool needsResize{ false }; // These are so that updates stay in the messager thread
    bool needsSampleUpdate{ false };
    bool sampleUpdateShouldReset{ false }; // This extra variable is necessary unfortunately

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
    SampleNavigator sampleNavigator;

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

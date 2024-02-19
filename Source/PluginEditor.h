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

    bool isInterestedInFileDrag(const String& file);
    bool isInterestedInFileDrag(const StringArray& files) override;
    void filesDropped(const StringArray& files, int x, int y) override;
    void filenameComponentChanged(FilenameComponent* fileComponentThatHasChanged) override;

    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override;
    void parameterChanged(const String& parameterID, float newValue) override;
    void addListeningParameters(std::vector<String> parameters);

    void updateWorkingSample();

private:
    JustaSampleAudioProcessor& processor;
    Array<CustomSamplerVoice*>& synthVoices;
    bool currentlyPlaying{ false };

    Array<Component*> sampleRequiredControls;
    FilenameComponent filenameComponent;
    ShapeButton storeFileButton;
    Label tuningLabel;
    Slider semitoneSlider;
    Slider centSlider;
    ShapeButton magicPitchButton;
    Path magicPitchButtonShape;
    ComboBox playbackOptions;
    Label isLoopingLabel;
    ToggleButton isLoopingButton;
    Slider masterGainSlider;
    ShapeButton haltButton;

    SampleEditor sampleEditor;
    SampleNavigator sampleNavigator;

    bool boundsSelectRoutine{ false }; // this is set to true once the user is prompted to select a bounds region for pitch detection
    bool samplePortionDisabled{ false }; // so that the UI can correctly enable the button a single time
    bool firstMouseUp{ false }; // necessary since a mouseup outside relevant areas should cancel bound selection but the first mouseup will be on the activation button

    FxChain fxChain;

    APVTS::SliderAttachment semitoneSliderAttachment, centSliderAttachment;
    APVTS::ComboBoxAttachment playbackOptionsAttachment;
    APVTS::ButtonAttachment loopToggleButtonAttachment;
    APVTS::SliderAttachment masterGainSliderAttachment;

    std::vector<String> listeningParameters;

    CustomLookAndFeel& lnf;

    OpenGLContext openGLContext;
    PluginHostType hostType;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JustaSampleAudioProcessorEditor)
};

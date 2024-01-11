#pragma once
#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "CustomLookAndFeel.h"
#include "PluginParameters.h"
#include "components/SampleEditor.h"
#include "components/FxChain.h"
#include "Components/Paths.h"

class JustaSampleAudioProcessorEditor  : public AudioProcessorEditor, public Timer, public FileDragAndDropTarget, public ValueTree::Listener, public APVTS::Listener
{
public:
    JustaSampleAudioProcessorEditor (JustaSampleAudioProcessor&);
    ~JustaSampleAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Inherited via FileDragAndDropTarget
    bool isInterestedInFileDrag(const String& file);
    bool isInterestedInFileDrag(const StringArray& files) override;
    void filesDropped(const StringArray& files, int x, int y) override;

    // Inherited via Timer
    void timerCallback() override;

    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override;
    void parameterChanged(const String& parameterID, float newValue) override;
    void addListeningParameters(std::vector<String> parameters);

    void updateWorkingSample();
private:
    JustaSampleAudioProcessor& processor;
    Array<CustomSamplerVoice*>& synthVoices;
    bool currentlyPlaying{ false };

    Array<Component*> sampleRequiredControls;
    Label fileLabel;
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

    bool samplePortionEnabled{ false };
    SampleEditor sampleEditor;
    SampleNavigator sampleNavigator;

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

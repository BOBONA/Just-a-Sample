/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
JustaSampleAudioProcessorEditor::JustaSampleAudioProcessorEditor(JustaSampleAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p), lnf(dynamic_cast<CustomLookAndFeel&>(getLookAndFeel())),
    sampleEditor(processor.apvts, voicePositions),
    sampleNavigator(processor.apvts, voicePositions),
    playbackOptionsAttachment(processor.apvts, PluginParameters::PLAYBACK_MODE, playbackOptions)
{
    p.apvts.state.addListener(this);
    setResizable(true, true);
    setSize(500, 300);
    
    fileLabel.setText("Test", juce::dontSendNotification);
    addAndMakeVisible(fileLabel);
    playbackOptions.addItemList(PluginParameters::PLAYBACK_MODE_LABELS, 1);
    playbackOptions.setSelectedItemIndex(processor.apvts.getParameterAsValue(PluginParameters::PLAYBACK_MODE).getValue());
    playbackOptions.setEnabled(false);
    addAndMakeVisible(playbackOptions);
    addAndMakeVisible(sampleEditor);
    addAndMakeVisible(sampleNavigator);
    updateWorkingSample();

    startTimerHz(60);
}

JustaSampleAudioProcessorEditor::~JustaSampleAudioProcessorEditor()
{
    processor.apvts.state.removeListener(this);
}

//==============================================================================
void JustaSampleAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(lnf.BACKGROUND_COLOR);
}

void JustaSampleAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    auto topControls = bounds.removeFromTop(15);
    auto label = topControls.removeFromLeft(topControls.getWidth() * 0.7f);
    fileLabel.setFont(juce::Font(label.getHeight()));
    fileLabel.setBounds(label);
    playbackOptions.setBounds(topControls);

    auto navigator = bounds.removeFromBottom(50);

    sampleEditor.setBounds(bounds);
    sampleNavigator.setBounds(navigator);
}

void JustaSampleAudioProcessorEditor::timerCallback()
{
    auto& synth = processor.getSynth();
    voicePositions.resize(synth.getNumVoices());
    bool isPlaying{ false };
    for (auto i = 0; i < synth.getNumVoices(); i++)
    {
        auto* voice = synth.getVoice(i);
        if (voice->isVoiceActive())
        {
            if (auto* v = dynamic_cast<CustomSamplerVoice*>(voice))
            {
                voicePositions.set(i, v->getEffectiveLocation());
                isPlaying = true;
            }
        }
        else
        {
            voicePositions.set(i, 0);
        }
    }
    if (isPlaying)
    {
        sampleEditor.updateSamplePosition();
        sampleNavigator.updateSamplePosition();
    }
}

bool JustaSampleAudioProcessorEditor::isInterestedInFileDrag(const String& file)
{
    return processor.canLoadFileExtension(file);
}

bool JustaSampleAudioProcessorEditor::isInterestedInFileDrag(const StringArray& files)
{
    for (const String& file : files)
    {
        if (isInterestedInFileDrag(file))
        {
            return true;
        }
    }
    return false;
}

void JustaSampleAudioProcessorEditor::filesDropped(const StringArray& files, int x, int y)
{
    for (const String& file : files)
    {
        if (isInterestedInFileDrag(file))
        {
            processor.loadFileAndReset(file);
            break;
        }
    }
}

void JustaSampleAudioProcessorEditor::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property)
{
    if (property.toString() == PluginParameters::FILE_PATH)
    {
        updateWorkingSample();
    }
}

void JustaSampleAudioProcessorEditor::updateWorkingSample()
{
    if (processor.getSample().getNumSamples() > 0)
    {
        playbackOptions.setEnabled(true);
        fileLabel.setText(processor.apvts.state.getProperty(PluginParameters::FILE_PATH), juce::dontSendNotification);
        sampleEditor.setSample(processor.getSample(), processor.resetParameters);
        sampleNavigator.setSample(processor.getSample(), processor.resetParameters);
    }
    else
    {
        playbackOptions.setEnabled(false);
    }
}

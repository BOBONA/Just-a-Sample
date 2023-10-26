/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
JustaSampleAudioProcessorEditor::JustaSampleAudioProcessorEditor(JustaSampleAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p), synthVoices(p.getSynthVoices()), lnf(dynamic_cast<CustomLookAndFeel&>(getLookAndFeel())),
    sampleEditor(processor.apvts, synthVoices),
    sampleNavigator(processor.apvts, synthVoices),
    playbackOptionsAttachment(processor.apvts, PluginParameters::PLAYBACK_MODE, playbackOptions),
{
    p.apvts.state.addListener(this);

    setResizeLimits(250, 200, 1000, 800);
    setResizable(false, true);
    setSize(500, 400);
    
    fileLabel.setText("File not selected", juce::dontSendNotification);
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

    auto editor = bounds.removeFromTop(bounds.getHeight() * 0.66f);
    auto navigator = bounds.removeFromTop(bounds.getHeight() * 0.2f);

    sampleEditor.setBounds(editor);
    sampleNavigator.setBounds(navigator);
}

void JustaSampleAudioProcessorEditor::timerCallback()
{
    bool stopped{ true };
    for (CustomSamplerVoice* voice : synthVoices)
    {
        if (voice->getCurrentlyPlayingSound() && !currentlyPlaying)
        {
            currentlyPlaying = true;
            stopped = false;
            break;
        }
    }
    if (stopped)
    {
        currentlyPlaying = false;
    }
    if (stopped || currentlyPlaying)
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

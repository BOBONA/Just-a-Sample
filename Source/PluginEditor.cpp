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
    loopToggleButtonAttachment(processor.apvts, PluginParameters::IS_LOOPING, isLoopingButton)
{
    p.apvts.state.addListener(this);

    setResizeLimits(250, 200, 1000, 800);
    setResizable(false, true);
    setSize(500, 400);
    
    fileLabel.setText("File not selected", dontSendNotification);
    fileLabel.setFont(15);
    addAndMakeVisible(fileLabel);

    playbackOptions.addItemList(PluginParameters::PLAYBACK_MODE_LABELS, 1);
    playbackOptions.setSelectedItemIndex(processor.apvts.getParameterAsValue(PluginParameters::PLAYBACK_MODE).getValue());
    playbackOptions.setEnabled(false);
    addAndMakeVisible(playbackOptions);

    isLoopingLabel.setText("Looping:", dontSendNotification);
    addAndMakeVisible(isLoopingLabel);

    isLoopingButton.setEnabled(false);
    addAndMakeVisible(isLoopingButton);

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
void JustaSampleAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll(lnf.BACKGROUND_COLOR);
}

void JustaSampleAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    bool singleLine = getWidth() > 400;
    auto top = bounds.removeFromTop(singleLine ? 15 : 30);
    FlexBox topControls{ FlexBox::Direction::row, FlexBox::Wrap::wrap, FlexBox::AlignContent::stretch, 
        FlexBox::AlignItems::stretch, FlexBox::JustifyContent::flexEnd };
    topControls.items.add(FlexItem(fileLabel).withFlex(1).withMinWidth(singleLine ? 150 : getWidth()));
    topControls.items.add(FlexItem(playbackOptions).withMinWidth(150));
    topControls.items.add(FlexItem(isLoopingLabel).withMinWidth(50));
    topControls.items.add(FlexItem(isLoopingButton).withMinWidth(20));
    topControls.performLayout(top);

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
        isLoopingButton.setEnabled(true);
        fileLabel.setText(processor.apvts.state.getProperty(PluginParameters::FILE_PATH), dontSendNotification);
        sampleEditor.setSample(processor.getSample(), processor.resetParameters);
        sampleNavigator.setSample(processor.getSample(), processor.resetParameters);
    }
    else
    {
        playbackOptions.setEnabled(false);
        isLoopingButton.setEnabled(false);
    }
}

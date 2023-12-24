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
    loopToggleButtonAttachment(processor.apvts, PluginParameters::IS_LOOPING, isLoopingButton),
    masterGainSliderAttachment(processor.apvts, PluginParameters::MASTER_GAIN, masterGainSlider)
{
    if (hostType.isReaper())
    {
        openGLContext.attachTo(*getTopLevelComponent());
    }

    p.apvts.state.addListener(this);
    p.apvts.addParameterListener(PluginParameters::MASTER_GAIN, this);

    setResizeLimits(250, 200, 1000, 800);
    setResizable(false, true);
    int width = p.apvts.state.getProperty(PluginParameters::WIDTH);
    int height = p.apvts.state.getProperty(PluginParameters::HEIGHT);
    if (250 > width || width > 1000 || 200 > height || height > 800)
    {
        setSize(500, 400);
    }
    else
    {
        setSize(width, height);
    }
    
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

    gainLabel.setText("+0db", dontSendNotification);
    gainLabel.setFont(12);
    addAndMakeVisible(gainLabel);

    masterGainSlider.setEnabled(false);
    masterGainSlider.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
    masterGainSlider.setSliderStyle(Slider::LinearHorizontal);
    addAndMakeVisible(masterGainSlider);

    addAndMakeVisible(sampleEditor);
    addAndMakeVisible(sampleNavigator);
    updateWorkingSample();

    startTimerHz(60);
}

JustaSampleAudioProcessorEditor::~JustaSampleAudioProcessorEditor()
{
    processor.apvts.state.removeListener(this);
    processor.apvts.removeParameterListener(PluginParameters::MASTER_GAIN, this);
}

//==============================================================================
void JustaSampleAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll(lnf.BACKGROUND_COLOR);
}

void JustaSampleAudioProcessorEditor::resized()
{
    processor.editorWidth = getWidth();
    processor.editorHeight = getHeight();
    auto bounds = getLocalBounds();

    FlexBox topControls{ FlexBox::Direction::row, FlexBox::Wrap::wrap, FlexBox::AlignContent::stretch, 
        FlexBox::AlignItems::stretch, FlexBox::JustifyContent::flexEnd };
    topControls.items.add(FlexItem(fileLabel).withFlex(1).withMinWidth(getWidth()));
    topControls.items.add(FlexItem(playbackOptions).withMinWidth(150));
    topControls.items.add(FlexItem(isLoopingLabel).withMinWidth(50));
    topControls.items.add(FlexItem(isLoopingButton).withMinWidth(20));
    topControls.items.add(FlexItem(gainLabel).withMinWidth(40));
    topControls.items.add(FlexItem(masterGainSlider).withMinWidth(60));
    float totalWidth = 0;
    for (FlexItem item : topControls.items)
    {
        totalWidth += item.minWidth;
    }
    auto top = bounds.removeFromTop(15 * ceil(totalWidth / getWidth()));
    topControls.performLayout(top);

    auto editor = bounds.removeFromTop(bounds.getHeight() * 0.66f);
    auto navigator = bounds.removeFromTop(bounds.getHeight() * 0.2f);

    sampleEditor.setBounds(editor);
    sampleNavigator.setBounds(navigator);
}

void JustaSampleAudioProcessorEditor::timerCallback()
{
    bool wasPlaying = currentlyPlaying;
    currentlyPlaying = false;
    for (CustomSamplerVoice* voice : synthVoices)
    {
        if (voice->getCurrentlyPlayingSound())
        {
            
            currentlyPlaying = true;
            break;
        }
    }
    if ((wasPlaying && !currentlyPlaying) || currentlyPlaying)
    {
        sampleEditor.repaintUI();
        sampleNavigator.repaintUI();
    }
    if (gainChanged)
    {
        String label = masterGainSlider.getValue() < 0 ? "" : "+";
        label += String(masterGainSlider.getValue(), 1, false);
        label += "db";
        gainLabel.setText(label, dontSendNotification);
        gainChanged = false;
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
        masterGainSlider.setEnabled(true);
        fileLabel.setText(processor.apvts.state.getProperty(PluginParameters::FILE_PATH), dontSendNotification);
        sampleEditor.setSample(processor.getSample(), processor.resetParameters);
        sampleNavigator.setSample(processor.getSample(), processor.resetParameters);
    }
    else
    {
        playbackOptions.setEnabled(false);
        isLoopingButton.setEnabled(false);
        masterGainSlider.setEnabled(false);
    }
}

void JustaSampleAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue)
{
    if (parameterID == PluginParameters::MASTER_GAIN)
    {
        gainChanged = true;
    }
}

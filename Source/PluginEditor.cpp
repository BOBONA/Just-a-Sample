/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
JustaSampleAudioProcessorEditor::JustaSampleAudioProcessorEditor(JustaSampleAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p), lnf(dynamic_cast<CustomLookAndFeel&>(getLookAndFeel())), sampleEditor(voicePositions), sampleNavigator(voicePositions)
{
    p.apvts.state.addListener(this);
    setSize(500, 300);
    
    addAndMakeVisible(sampleEditor);
    addAndMakeVisible(sampleNavigator);
    updateUI();

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
            if (processor.loadFile(file))
            {
                processor.apvts.state.setProperty(PluginParameters::FILE_PATH, file, &processor.undoManager);
            }
            break;
        }
    }
}

void JustaSampleAudioProcessorEditor::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property)
{
    if (property.toString() == PluginParameters::FILE_PATH)
    {
        updateSample();
    }
}

void JustaSampleAudioProcessorEditor::updateUI()
{
    updateSample();
}

void JustaSampleAudioProcessorEditor::updateSample()
{
    if (processor.getSample().getNumSamples() > 0)
    {
        sampleEditor.setSample(processor.getSample());
        sampleNavigator.setSample(processor.getSample());
    }
}

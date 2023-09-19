/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
JustaSampleAudioProcessorEditor::JustaSampleAudioProcessorEditor(JustaSampleAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p), sampleExplorer(voicePositions)
{
    setSize(500, 300);
    
    addAndMakeVisible(sampleExplorer);

    startTimerHz(20);
}

JustaSampleAudioProcessorEditor::~JustaSampleAudioProcessorEditor()
{
}

//==============================================================================
void JustaSampleAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::lightslategrey);
}

void JustaSampleAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    sampleExplorer.setBounds(bounds);
}

void JustaSampleAudioProcessorEditor::timerCallback()
{
    auto& synth = processor.getSynth();
    voicePositions.resize(synth.getNumVoices());
    for (auto i = 0; i < synth.getNumVoices(); i++)
    {
        auto* voice = synth.getVoice(i);
        if (voice->isVoiceActive())
        {
            if (auto* v = dynamic_cast<CustomSamplerVoice*>(voice))
            {
                voicePositions.set(i, v->getPlayingLocation());
            }
        }
        else
        {
            voicePositions.set(i, 0);
        }
    }
    sampleExplorer.repaint();
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
                sampleExplorer.setSample(processor.getSample());
            }
            break;
        }
    }
}

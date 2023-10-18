/*
  ==============================================================================

    CustomSamplerSound.cpp
    Created: 5 Sep 2023 3:35:11pm
    Author:  binya

  ==============================================================================
*/

#include "CustomSamplerSound.h"

CustomSamplerSound::CustomSamplerSound(AudioProcessorValueTreeState& apvts, AudioBuffer<float>& sample, int sampleRate, float& baseFreq) : 
    sample(sample), sampleRate(sampleRate), baseFreq(baseFreq)
{
    sampleStart = apvts.state.getPropertyAsValue(PluginParameters::SAMPLE_START, apvts.undoManager);
    sampleEnd = apvts.state.getPropertyAsValue(PluginParameters::SAMPLE_END, apvts.undoManager);
}

bool CustomSamplerSound::appliesToNote(int midiNoteNumber)
{
    return true;
}

bool CustomSamplerSound::appliesToChannel(int midiChannel)
{
    return true;
}

int CustomSamplerSound::getSampleStart()
{
    return sampleStart.getValue();
}

int CustomSamplerSound::getSampleEnd()
{
    return sampleEnd.getValue();
}

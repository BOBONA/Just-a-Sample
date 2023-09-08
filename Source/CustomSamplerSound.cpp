/*
  ==============================================================================

    CustomSamplerSound.cpp
    Created: 5 Sep 2023 3:35:11pm
    Author:  binya

  ==============================================================================
*/

#include "CustomSamplerSound.h"

CustomSamplerSound::CustomSamplerSound(AudioBuffer<float>& sample, int sampleRate, float& baseFreq) : sample(sample), sampleRate(sampleRate), baseFreq(baseFreq)
{
}

bool CustomSamplerSound::appliesToNote(int midiNoteNumber)
{
    return true;
}

bool CustomSamplerSound::appliesToChannel(int midiChannel)
{
    return true;
}

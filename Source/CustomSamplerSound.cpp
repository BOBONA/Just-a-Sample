/*
  ==============================================================================

    CustomSamplerSound.cpp
    Created: 5 Sep 2023 3:35:11pm
    Author:  binya

  ==============================================================================
*/

#include "CustomSamplerSound.h"

bool CustomSamplerSound::appliesToNote(int midiNoteNumber)
{
    return true;
}

bool CustomSamplerSound::appliesToChannel(int midiChannel)
{
    return true;
}

/*
  ==============================================================================

    CustomSamplerSound.h
    Created: 5 Sep 2023 3:35:11pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

using namespace juce;

class CustomSamplerSound : public SynthesiserSound
{
    // Inherited via SynthesiserSound
    bool appliesToNote(int midiNoteNumber) override;
    bool appliesToChannel(int midiChannel) override;
};
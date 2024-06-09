/*
  ==============================================================================

    BlankSynthesizerSound.h
    Created: 9 Jun 2024 10:37:38am
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class BlankSynthesizerSound final : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

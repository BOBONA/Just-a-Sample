/*
  ==============================================================================

    BlankSynthesizerSound.h
    Created: 9 Jun 2024 10:37:38am
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

/** JUCE's voice and sound paradigm is not so helpful for us, so we use a blank sound class and pass in our parameters directly to the voices. */
class BlankSynthesizerSound final : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

/*
  ==============================================================================

    CustomSamplerVoice.h
    Created: 5 Sep 2023 3:35:03pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "CustomSamplerSound.h"

using namespace juce;

enum VoiceState
{
    PLAYING,
    STOPPING,
    STOPPED
};

class CustomSamplerVoice : public SynthesiserVoice
{
public:
    // Inherited via SynthesiserVoice
    bool canPlaySound(SynthesiserSound*) override;
    void startNote(int midiNoteNumber, float velocity, SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;
private:
    VoiceState state;
    CustomSamplerSound* samplerSound;
    float velocity;
    int pitchWheel;

    double phase;
    double pos;

    const int stopSamples = 20;
    double smoothStop;
};

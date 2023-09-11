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
#include <RubberBandStretcher.h>

using namespace juce;

enum VoiceState
{
    STARTING,
    PLAYING,
    STOPPING,
    STOPPED
};

class CustomSamplerVoice : public SynthesiserVoice
{
public:
    CustomSamplerVoice(double sampleRate, int numChannels);
    // Inherited via SynthesiserVoice
    bool canPlaySound(SynthesiserSound*) override;
    void startNote(int midiNoteNumber, float velocity, SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;
private:
    const int START_SAMPLES = 300;
    const int STOP_SAMPLES = 3000;

    CustomSamplerSound* sampleSound{ nullptr };
    float sampleRatio{ 1 };
    float velocity{ 0 };
    int pitchWheel{ 0 };

    VoiceState state{ STOPPED };
    int smoothingSample{ 0 };

    using Stretcher = RubberBand::RubberBandStretcher;
    Stretcher stretcher;
    AudioBuffer<float> paddedSound;
    int nextUnpitchedSample{ 0 };
    AudioBuffer<float> pitchedSound;
    int totalPitchedSamples{ 0 };
    int currentSample{ 0 };
};

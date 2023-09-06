/*
  ==============================================================================

    CustomSamplerVoice.cpp
    Created: 5 Sep 2023 3:35:03pm
    Author:  binya

  ==============================================================================
*/

#include "CustomSamplerVoice.h"

bool CustomSamplerVoice::canPlaySound(SynthesiserSound* sound)
{
    return bool(dynamic_cast<CustomSamplerSound*>(sound));
}

void CustomSamplerVoice::startNote(int midiNoteNumber, float velocity, SynthesiserSound* sound, int currentPitchWheelPosition)
{   
    this->velocity = velocity;
    pitchWheel = currentPitchWheelPosition;
    samplerSound = dynamic_cast<CustomSamplerSound*>(sound);
    state = PLAYING;

    phase = MidiMessage::getMidiNoteInHertz(midiNoteNumber) * MathConstants<double>::twoPi;
    pos = 0;
}

void CustomSamplerVoice::stopNote(float velocity, bool allowTailOff)
{
    if (allowTailOff)
    {
        state = STOPPING;
        smoothStop = 1;
    }
    else
    {
        state = STOPPED;
        clearCurrentNote();
    }
}

void CustomSamplerVoice::pitchWheelMoved(int newPitchWheelValue)
{
    pitchWheel = newPitchWheelValue;
}

void CustomSamplerVoice::controllerMoved(int controllerNumber, int newControllerValue)
{
}

void CustomSamplerVoice::renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (state == STOPPED || !samplerSound)
        return;
    auto note = getCurrentlyPlayingNote();
    for (auto i = startSample; i < startSample + numSamples; i++) 
    {
        double value = std::sin(pos);
        if (state == PLAYING)
        {
            outputBuffer.setSample(0, i, outputBuffer.getSample(0, i) + value);
            outputBuffer.setSample(1, i, outputBuffer.getSample(1, i) + value);
        }
        else if (state == STOPPING)
        {
            double factor = (stopSamples - smoothStop) / stopSamples;
            outputBuffer.setSample(0, i, outputBuffer.getSample(0, i) + factor * value);
            outputBuffer.setSample(1, i, outputBuffer.getSample(1, i) + factor * value);
            smoothStop++;
            if (smoothStop == stopSamples) {
                state = STOPPED;
                clearCurrentNote();
            }
        }
        pos += phase / getSampleRate();
    }
}

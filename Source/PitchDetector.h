/*
  ==============================================================================

    PitchDetector.h
    Created: 27 Dec 2023 2:51:29pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class PitchDetector : public juce::Thread
{
public:
    PitchDetector() : Thread("Pitch_Detector"), pitchMPM(0, 0)
    {
    }

    void setAudioBuffer(juce::AudioBuffer<float>& buffer, int sampleRate)
    {
        this->audioBuffer = buffer;
        this->sampleRate = sampleRate;
    }

    // Inherited via Thread
    void run() override
    {
        pitchMPM.setBufferSize(juce::nextPowerOfTwo(audioBuffer.getNumSamples()));
        pitchMPM.setSampleRate(sampleRate);
        pitch = pitchMPM.getPitch(audioBuffer.getReadPointer(0));
        signalThreadShouldExit();
    }

    float getPitch() const
    {
        return pitch;
    }

private:
    juce::AudioBuffer<float> audioBuffer;
    int sampleRate;
    float pitch;
    adamski::PitchMPM pitchMPM;
};
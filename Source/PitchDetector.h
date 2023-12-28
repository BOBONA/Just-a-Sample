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

    void setData(juce::AudioBuffer<float>& buffer, int sampleStart, int sampleEnd, int sampleRate)
    {
        audioBuffer.setSize(1, juce::nextPowerOfTwo(sampleEnd - sampleStart + 1));
        audioBuffer.clear();
        audioBuffer.copyFrom(0, 0, buffer.getReadPointer(0), sampleEnd - sampleStart + 1);
        this->sampleRate = sampleRate;
    }

    // Inherited via Thread
    void run() override
    {
        pitchMPM.setBufferSize(audioBuffer.getNumSamples());
        pitchMPM.setSampleRate(sampleRate);
        pitch = pitchMPM.getPitch(audioBuffer.getReadPointer(0));
        audioBuffer = juce::AudioBuffer<float>(); // clear the buffer
        signalThreadShouldExit();
    }

    float getPitch() const
    {
        return pitch;
    }

private:
    juce::AudioBuffer<float> audioBuffer;
    int sampleRate{ 0 };
    float pitch{ 0 };
    adamski::PitchMPM pitchMPM;
};
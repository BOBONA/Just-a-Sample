/*
  ==============================================================================

    PitchDetector.h
    Created: 27 Dec 2023 2:51:29pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <RubberBandStretcher.h>

#include "PluginProcessor.h"

using Stretcher = RubberBand::RubberBandStretcher;

class PitchDetector : public juce::Thread
{
public:
    PitchDetector() : Thread("Pitch_Detector"), pitchMPM(0, 0), stretcher(44100, 1, Stretcher::OptionProcessOffline | Stretcher::OptionEngineFiner, 2)
    {
        inChannel = new const float* [1];
        outChannel = new float* [1];
    }

    ~PitchDetector()
    {
        stopThread(1000);
        delete[] inChannel;
        delete[] outChannel;
    }

    void setData(juce::AudioBuffer<float>& buffer, int sampleStart, int sampleEnd, int audioSampleRate)
    {
        sampleSize = sampleEnd - sampleStart + 1;
        audioBuffer.setSize(1, juce::nextPowerOfTwo(sampleSize));
        audioBuffer.clear();
        audioBuffer.copyFrom(0, 0, buffer.getReadPointer(0), sampleSize);
        sampleRate = audioSampleRate;
    }

    // Inherited via Thread
    void run() override
    {
        pitchMPM.setBufferSize(audioBuffer.getNumSamples());
        pitchMPM.setSampleRate(sampleRate);
        pitch = pitchMPM.getPitch(audioBuffer.getReadPointer(0));
        int ratio = 1; // pitch shifting to try to get an output out of the pitch detector (a bit sketchy yes)
        while (pitch == -1 && ratio < 16)
        {
            ratio *= 2;
            stretcher.reset();
            inChannel[0] = audioBuffer.getReadPointer(0);
            stretcher.study(inChannel, sampleSize, true);
            stretcher.process(inChannel, sampleSize, true);
            sampleSize = stretcher.available();
            if (audioBuffer.getNumSamples() < sampleSize)
            {
                audioBuffer.setSize(audioBuffer.getNumChannels(), sampleSize, true);
            }
            stretcher.retrieve(audioBuffer.getArrayOfWritePointers(), sampleSize);
            audioBuffer.setSize(1, juce::nextPowerOfTwo(sampleSize), true, true);
            pitchMPM.setBufferSize(audioBuffer.getNumSamples());
            pitch = pitchMPM.getPitch(audioBuffer.getReadPointer(0));
        }
        audioBuffer = juce::AudioBuffer<float>(); // clear the buffer
        signalThreadShouldExit();
    }

    float getPitch() const
    {
        return pitch;
    }

private:
    juce::AudioBuffer<float> audioBuffer;
    int sampleSize{ 0 };
    int sampleRate{ 0 };
    float pitch{ 0 };

    adamski::PitchMPM pitchMPM;
    Stretcher stretcher;

    const float** inChannel{ nullptr };
    float** outChannel{ nullptr };
};
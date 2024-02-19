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
    PitchDetector() : Thread("Pitch_Detector")
    {
    }

    ~PitchDetector()
    {
        stopThread(1000);
    }

    void setData(juce::AudioBuffer<float>& buffer, int startSample, int endSample, double audioSampleRate)
    {
        audioBuffer = &buffer;
        sampleStart = startSample;
        sampleEnd = endSample;
        sampleRate = audioSampleRate;
    }

    // Inherited via Thread
    void run() override
    {
        if (!audioBuffer)
            return;

        detectPitch();
        signalThreadShouldExit();
    }

    void detectPitch()
    {
        int numSamples = sampleEnd - sampleStart + 1;

        LEAF leaf;
        tPitchDetector pitchDetector;

        const size_t memorySize = 5000;
        char memory[memorySize];
        LEAF_init(&leaf, float(sampleRate), memory, memorySize, []() -> float { return Random().nextFloat(); });
        tPitchDetector_init(&pitchDetector, 50, 20000, &leaf);
        for (int i = 0; i < numSamples; i++)
        {
            bool ready = audioBuffer->getNumChannels() == 2 ?
                tPitchDetector_tick(&pitchDetector, (audioBuffer->getSample(0, sampleStart + i) + audioBuffer->getSample(1, sampleStart + i)) / 2.f) :
                tPitchDetector_tick(&pitchDetector, audioBuffer->getSample(0, sampleStart + i));
            if (ready)
                break;
        }
        pitch = double(tPitchDetector_predictFrequency(&pitchDetector));
        tPitchDetector_free(&pitchDetector);
    }

    double getPitch() const
    {
        return pitch;
    }

private:
    juce::AudioBuffer<float>* audioBuffer{ nullptr };
    int sampleStart{ 0 }, sampleEnd{ 0 };
    double sampleRate{ 0. };
    double pitch{ 0 };
};
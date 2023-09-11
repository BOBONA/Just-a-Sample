/*
  ==============================================================================

    PitchShifter.h
    Created: 11 Sep 2023 9:34:55am
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <RubberBandStretcher.h>

using Stretcher = RubberBand::RubberBandStretcher;

class BufferPitcher
{
public:
    // The assumption right now is that sampleRate and numChannels are global properties that don't need to be configured per buffer
    BufferPitcher(bool online, size_t sampleRate, size_t numChannels, Stretcher::Options stretcherOptions = -1);
    ~BufferPitcher();

    void initializeWithBuffer(juce::AudioBuffer<float> buffer);
    void resetProcessing();
    void setPitchScale(double scale);
    void setTimeRatio(double ratio);

    void processSamples(int currentSample, int numSamples);

    juce::AudioBuffer<float> processedBuffer;
    int totalPitchedSamples{ 0 };
    int delay{ 0 };

private:
    Stretcher::Options REALTIME_DEFAULT = Stretcher::OptionProcessRealTime | Stretcher::OptionEngineFiner | Stretcher::OptionWindowShort;
    Stretcher::Options OFFLINE_DEFAULT = Stretcher::OptionProcessOffline;

    juce::AudioBuffer<float> paddedSound;
    int nextUnpitchedSample{ 0 };

    Stretcher stretcher;
    bool initialized{ false };
    bool realtime{ false };

    const float** inChannels{ nullptr };
    float** outChannels{ nullptr };
};
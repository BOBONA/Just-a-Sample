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
    BufferPitcher(juce::AudioBuffer<float>& buffer, size_t sampleRate, size_t numChannels, bool resetProcessing = true, Stretcher::Options stretcherOptions = DEFAULT_OPTIONS);
    ~BufferPitcher();

    void resetProcessing();
    void setPitchScale(double scale);
    void setTimeRatio(double ratio);
    void setSampleStart(int sample);
    void setSampleEnd(int sample);

    void processSamples(int currentSample, int numSamples);
    int expectedExtraSamples();

    juce::AudioBuffer<float> processedBuffer;
    int totalPitchedSamples{ 0 };
    int startDelay{ 0 };
private:
    const static Stretcher::Options DEFAULT_OPTIONS = Stretcher::OptionProcessRealTime | Stretcher::OptionEngineFiner | Stretcher::OptionWindowShort;

    juce::AudioBuffer<float>& buffer;
    int nextUnpitchedSample{ 0 };
    int sampleEnd{ 0 };
    int sampleStart{ 0 };

    Stretcher stretcher;
    bool initialized{ false };

    const float** inChannels{ nullptr };
    float** outChannels{ nullptr };
};
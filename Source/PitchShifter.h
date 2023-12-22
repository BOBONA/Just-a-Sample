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
    /* Basic initialization of the buffer pitcher, resetProcessing must be called at some point */
    BufferPitcher(juce::AudioBuffer<float>& buffer, size_t sampleRate, size_t numChannels, bool resetProcessing = true, Stretcher::Options stretcherOptions = DEFAULT_OPTIONS);
    ~BufferPitcher();

    /* Resets the pitcher and prepares the padding, necessary after changing certain parameters */
    void resetProcessing();
    void setPitchScale(double scale);
    void setTimeRatio(double ratio);
    void setSampleStart(int sample);
    void setSampleEnd(int sample);

    /* Updates the processed buffer up to currentSample + numSamples */
    void processSamples(int currentSample, int numSamples);
    int expectedExtraSamples();

    /* The processed sample buffer */
    std::shared_ptr<juce::AudioBuffer<float>> processedBuffer{ nullptr };

    /* The number of samples in the buffer that should be accessed */
    int totalPitchedSamples{ 0 };

    /* This is the delay in the processed buffer before real output */
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
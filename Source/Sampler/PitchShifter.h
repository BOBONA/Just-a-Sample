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
    BufferPitcher(juce::AudioBuffer<float>& buffer, int sampleRate, bool resetProcessing = true, Stretcher::Options stretcherOptions = DEFAULT_OPTIONS);
    ~BufferPitcher();

    /* Resets the pitcher and prepares the padding, necessary after changing certain parameters */
    void resetProcessing(bool processPadding=true);
    void setPitchScale(double scale);
    void setTimeRatio(double ratio);
    void setSampleStart(int sample);
    void setSampleEnd(int sample);

    /* Updates the processed buffer up to currentSample + numSamples */
    void processSamples(int currentSample, int numSamples);

    /* These are used to process the initial padding in parts */
    int startPad();
    void processZeros(int numSamples);

    /* The processed sample buffer */
    juce::AudioBuffer<float> processedBuffer;

    /* The number of samples in the buffer that should be accessed */
    int totalPitchedSamples{ 0 };

    /* The number of output samples, not including the startDelay, PitchShifter will stop at this regardless of where the Stretcher is */
    int expectedOutputSamples{ 0 };

    /* This is the delay in the processed buffer before real output */
    int startDelay{ 0 };

    const static int EXPECTED_PADDING = 1280;
    const static Stretcher::Options DEFAULT_OPTIONS = Stretcher::OptionProcessRealTime | Stretcher::OptionEngineFiner | Stretcher::OptionWindowShort;
private:
    juce::AudioBuffer<float>& buffer;
    int nextUnpitchedSample{ 0 };
    int sampleEnd{ 0 };
    int sampleStart{ 0 };

    Stretcher stretcher;
    bool initialized{ false };

    const float** inChannels{ nullptr };
    float** outChannels{ nullptr };

    juce::AudioBuffer<float> zeroBuffer;
};
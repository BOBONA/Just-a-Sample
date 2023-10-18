/*
  ==============================================================================

    PitchShifter.cpp
    Created: 11 Sep 2023 9:34:55am
    Author:  binya

  ==============================================================================
*/

#include "PitchShifter.h"

BufferPitcher::BufferPitcher(juce::AudioBuffer<float> buffer, size_t sampleRate, size_t numChannels, Stretcher::Options options) :
    stretcher(sampleRate, numChannels, options)
{
    initialized = true;
    paddedSound.setSize(buffer.getNumChannels(), buffer.getNumSamples() + stretcher.getPreferredStartPad());
    paddedSound.clear();
    for (auto ch = 0; ch < buffer.getNumChannels(); ch++)
    {
        paddedSound.copyFrom(ch, stretcher.getPreferredStartPad(), buffer.getReadPointer(ch), buffer.getNumSamples());
    }
    sampleEnd = paddedSound.getNumSamples();
    resetProcessing();
    delete[] inChannels;
    delete[] outChannels;
    inChannels = new const float* [paddedSound.getNumChannels()];
    outChannels = new float* [processedBuffer.getNumChannels()];
}

BufferPitcher::~BufferPitcher()
{
    delete[] inChannels;
    delete[] outChannels;
}

void BufferPitcher::resetProcessing()
{
    processedBuffer.setSize(paddedSound.getNumChannels(), sampleEnd + stretcher.getStartDelay());
    stretcher.reset();
    juce::AudioBuffer<float> emptyBuffer = juce::AudioBuffer<float>(stretcher.getChannelCount(), stretcher.getPreferredStartPad());
    stretcher.process(emptyBuffer.getArrayOfReadPointers(), emptyBuffer.getNumSamples(), false);

    totalPitchedSamples = 0;
    startDelay = stretcher.getStartDelay();
    nextUnpitchedSample = 0;
}

void BufferPitcher::setPitchScale(double scale)
{
    stretcher.setPitchScale(scale);
}

void BufferPitcher::setTimeRatio(double ratio)
{
    stretcher.setTimeRatio(ratio);
    processedBuffer.setSize(paddedSound.getNumChannels(), sampleEnd + stretcher.getStartDelay());
}

void BufferPitcher::setSampleEnd(int sample)
{
    sampleEnd = sample + stretcher.getStartDelay();
}

void BufferPitcher::processSamples(int currentSample, int numSamples)
{
    if (!initialized)
        return;
    while (totalPitchedSamples - currentSample < numSamples && sampleEnd > nextUnpitchedSample)
    {
        // find amount of input samples
        auto requiredSamples = stretcher.getSamplesRequired();
        auto last = false;
        if (requiredSamples > sampleEnd - nextUnpitchedSample)
        {
            requiredSamples = sampleEnd - nextUnpitchedSample;
            last = true;
        }
        // get input pointer
        for (auto ch = 0; ch < paddedSound.getNumChannels(); ch++)
        {
            inChannels[ch] = paddedSound.getReadPointer(ch, nextUnpitchedSample);
        }
        // process
        stretcher.process(inChannels, requiredSamples, last);
        nextUnpitchedSample += requiredSamples;
        // find amount of output samples
        auto availableSamples = stretcher.available();
        if (availableSamples <= 0 && last)
            break;
        if (availableSamples < 0)
            availableSamples = 0;
        if (totalPitchedSamples + availableSamples > processedBuffer.getNumSamples())
        {
            processedBuffer.setSize(processedBuffer.getNumChannels(), totalPitchedSamples + availableSamples, true);
        }
        // get output pointer
        for (auto ch = 0; ch < processedBuffer.getNumChannels(); ch++)
        {
            outChannels[ch] = processedBuffer.getWritePointer(ch, totalPitchedSamples);
        }
        // retrieve
        stretcher.retrieve(outChannels, availableSamples);
        totalPitchedSamples += availableSamples;
    }
}

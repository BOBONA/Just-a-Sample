/*
  ==============================================================================

    PitchShifter.cpp
    Created: 11 Sep 2023 9:34:55am
    Author:  binya

  ==============================================================================
*/

#include "PitchShifter.h"

BufferPitcher::BufferPitcher(juce::AudioBuffer<float>& buffer, size_t sampleRate, size_t numChannels, bool resetProcessing, Stretcher::Options options) :
    stretcher(sampleRate, numChannels, options), buffer(buffer)
{
    initialized = true;
    sampleEnd = buffer.getNumSamples();
    if (resetProcessing)
    {
        this->resetProcessing();
    }
    delete[] inChannels;
    delete[] outChannels;
    inChannels = new const float* [buffer.getNumChannels()];
    outChannels = new float* [processedBuffer.getNumChannels()];
}

BufferPitcher::~BufferPitcher()
{
    delete[] inChannels;
    delete[] outChannels;
}

void BufferPitcher::resetProcessing()
{
    processedBuffer.setSize(buffer.getNumChannels(), sampleEnd + stretcher.getStartDelay());
    stretcher.reset();
    juce::AudioBuffer<float> emptyBuffer = juce::AudioBuffer<float>(stretcher.getChannelCount(), stretcher.getPreferredStartPad());
    emptyBuffer.clear();
    // hopefully this is a proper thing to do, for some reason the requiredSamples stays high even afterwards
    stretcher.process(emptyBuffer.getArrayOfReadPointers(), emptyBuffer.getNumSamples(), false);

    totalPitchedSamples = sampleStart;
    startDelay = stretcher.getStartDelay();
    nextUnpitchedSample = sampleStart;
}

void BufferPitcher::setPitchScale(double scale)
{
    stretcher.setPitchScale(scale);
}

void BufferPitcher::setTimeRatio(double ratio)
{
    stretcher.setTimeRatio(ratio);
    processedBuffer.setSize(buffer.getNumChannels(), sampleEnd + stretcher.getStartDelay());
}

void BufferPitcher::setSampleStart(int sample)
{
    sampleStart = sample;
}

void BufferPitcher::setSampleEnd(int sample)
{
    sampleEnd = sample;
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
        for (auto ch = 0; ch < buffer.getNumChannels(); ch++)
        {
            inChannels[ch] = buffer.getReadPointer(ch, nextUnpitchedSample);
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
        DBG("STEP " << requiredSamples << " STEP " << availableSamples /*/ stretcher.getTimeRatio()*/);
        DBG("Input " << nextUnpitchedSample << " Output " << totalPitchedSamples /*/ stretcher.getTimeRatio()*/);
    }
}

int BufferPitcher::expectedExtraSamples()
{
    return stretcher.getPreferredStartPad();
}

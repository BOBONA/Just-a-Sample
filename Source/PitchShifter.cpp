/*
  ==============================================================================

    PitchShifter.cpp
    Created: 11 Sep 2023 9:34:55am
    Author:  binya

  ==============================================================================
*/

#include "PitchShifter.h"

BufferPitcher::BufferPitcher(bool realtime, size_t sampleRate, size_t numChannels, Stretcher::Options options) :
    realtime(realtime), stretcher(sampleRate, numChannels, options)
{
}

BufferPitcher::~BufferPitcher()
{
    delete[] inChannels;
    delete[] outChannels;
}

void BufferPitcher::initializeWithBuffer(juce::AudioBuffer<float> buffer)
{
    initialized = true;
    stretcher.reset();
    if (realtime)
    {
        paddedSound.setSize(buffer.getNumChannels(), buffer.getNumSamples() + stretcher.getPreferredStartPad());
        paddedSound.clear();
        for (auto ch = 0; ch < buffer.getNumChannels(); ch++)
        {
            paddedSound.copyFrom(ch, stretcher.getPreferredStartPad(), buffer.getReadPointer(ch), buffer.getNumSamples());
        }
        delay = stretcher.getStartDelay();
    }
    else 
    {
        stretcher.study(buffer.getArrayOfReadPointers(), buffer.getNumSamples(), true); 
        paddedSound = buffer;
    }
    resetProcessing();
    delete[] inChannels;
    delete[] outChannels;
    inChannels = new const float* [paddedSound.getNumChannels()];
    outChannels = new float* [processedBuffer.getNumChannels()];
}

void BufferPitcher::resetProcessing()
{
    if (realtime)
    {
        processedBuffer.setSize(paddedSound.getNumChannels(), paddedSound.getNumSamples() + stretcher.getStartDelay());
    }
    else 
    {
        processedBuffer.setSize(paddedSound.getNumChannels(), paddedSound.getNumSamples());
    }
    totalPitchedSamples = 0;
    nextUnpitchedSample = 0;
}

void BufferPitcher::setPitchScale(double scale)
{
    if (!realtime)
    {
        stretcher.reset();
    }
    stretcher.setPitchScale(scale);
}

void BufferPitcher::setTimeRatio(double ratio)
{
    stretcher.setTimeRatio(ratio);
}

void BufferPitcher::processSamples(int currentSample, int numSamples)
{
    if (!initialized)
        return;
    while (totalPitchedSamples - currentSample < numSamples && paddedSound.getNumSamples() > nextUnpitchedSample)
    {
        // find amount of input samples
        auto requiredSamples = stretcher.getSamplesRequired();
        auto last = false;
        if (requiredSamples > paddedSound.getNumSamples() - nextUnpitchedSample)
        {
            requiredSamples = paddedSound.getNumSamples() - nextUnpitchedSample;
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

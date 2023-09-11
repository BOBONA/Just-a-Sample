/*
  ==============================================================================

    PitchShifter.cpp
    Created: 11 Sep 2023 9:34:55am
    Author:  binya

  ==============================================================================
*/

#include "PitchShifter.h"

BufferPitcher::BufferPitcher(bool realtime, size_t sampleRate, size_t numChannels, Stretcher::Options options) :
    realtime(realtime), stretcher(sampleRate, numChannels, (options < 0 ? (realtime ? REALTIME_DEFAULT : OFFLINE_DEFAULT) : options))
{
}

BufferPitcher::~BufferPitcher()
{
    delete[] inChannels;
    delete[] outChannels;
}

void BufferPitcher::initializeBuffer(juce::AudioBuffer<float> buffer)
{
    stretcher.reset();
    if (realtime)
    {
        paddedSound.setSize(buffer.getNumChannels(), buffer.getNumSamples() + stretcher.getPreferredStartPad());
        paddedSound.clear();
        for (auto ch = 0; ch < buffer.getNumChannels(); ch++)
        {
            paddedSound.copyFrom(ch, stretcher.getPreferredStartPad(), buffer.getReadPointer(ch), buffer.getNumSamples());
        }
        nextUnpitchedSample = 0;

        processedBuffer.setSize(paddedSound.getNumChannels(), paddedSound.getNumSamples() + stretcher.getStartDelay());
        processedBuffer.clear();
        totalPitchedSamples = 0;
        delay = stretcher.getStartDelay();
    }
    else 
    {
        stretcher.study(buffer.getArrayOfReadPointers(), buffer.getNumSamples(), true);
        
        paddedSound = buffer;
        nextUnpitchedSample = 0;

        processedBuffer.setSize(paddedSound.getNumChannels(), paddedSound.getNumSamples());
        processedBuffer.clear();
        totalPitchedSamples = 0;
        delay = 0;
    }
    delete[] inChannels;
    delete[] outChannels;
    inChannels = new const float* [paddedSound.getNumChannels()];
    outChannels = new float* [processedBuffer.getNumChannels()];
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
        {
            break;
        }
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

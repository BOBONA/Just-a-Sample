/*
  ==============================================================================

    PitchShifter.cpp
    Created: 11 Sep 2023 9:34:55am
    Author:  binya

  ==============================================================================
*/

#include "PitchShifter.h"

BufferPitcher::BufferPitcher(juce::AudioBuffer<float>& buffer, int sampleRate, bool resetProcessing, Stretcher::Options options) :
    stretcher(sampleRate, buffer.getNumChannels(), options), buffer(buffer)
{
    sampleEnd = buffer.getNumSamples();
    if (resetProcessing)
    {
        this->resetProcessing();
    }
    delete[] inChannels;
    delete[] outChannels;
    inChannels = new const float* [buffer.getNumChannels()];
    outChannels = new float* [buffer.getNumChannels()];
    zeroBuffer.setSize(buffer.getNumChannels(), 2 * EXPECTED_PADDING, false, true);
}

BufferPitcher::~BufferPitcher()
{
    delete[] inChannels;
    delete[] outChannels;
}

void BufferPitcher::resetProcessing(bool processPadding)
{
    processedBuffer.setSize(buffer.getNumChannels(), sampleEnd - sampleStart + int(stretcher.getStartDelay()));
    stretcher.reset();
    if (processPadding)
    {
        zeroBuffer.setSize(int(stretcher.getChannelCount()), int(stretcher.getPreferredStartPad()), true, true);
        // hopefully this is a proper thing to do, for some reason the requiredSamples stays high even afterwards
        stretcher.process(zeroBuffer.getArrayOfReadPointers(), zeroBuffer.getNumSamples(), false);
    }
    
    totalPitchedSamples = 0;
    startDelay = int(stretcher.getStartDelay());
    expectedOutputSamples = int((sampleEnd - sampleStart + 1) * stretcher.getTimeRatio());
    nextUnpitchedSample = sampleStart;
    initialized = true;
}

void BufferPitcher::setPitchScale(double scale)
{
    stretcher.setPitchScale(scale);
}

void BufferPitcher::setTimeRatio(double ratio)
{
    stretcher.setTimeRatio(ratio);
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
    while (totalPitchedSamples < currentSample + numSamples && sampleEnd >= nextUnpitchedSample && totalPitchedSamples - startDelay < expectedOutputSamples)
    {
        // find amount of input samples
        int requiredSamples = int(stretcher.getSamplesRequired());
        bool last = false;
        if (requiredSamples > sampleEnd - nextUnpitchedSample + 1)
        {
            requiredSamples = sampleEnd - nextUnpitchedSample + 1;
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
    }
}

int BufferPitcher::startPad()
{
    return int(stretcher.getPreferredStartPad());
}

void BufferPitcher::processZeros(int numSamples)
{
    if (numSamples > zeroBuffer.getNumSamples())
    {
        zeroBuffer.setSize(zeroBuffer.getNumChannels(), numSamples, true, true);
    }
    stretcher.process(zeroBuffer.getArrayOfReadPointers(), numSamples, false);
}

/*
  ==============================================================================

    PitchShifter.cpp
    Created: 11 Sep 2023 9:34:55am
    Author:  binya

  ==============================================================================
*/

#include "PitchShifter.h"

BufferPitcher::BufferPitcher(juce::AudioBuffer<float>& buffer, size_t sampleRate, size_t numChannels, bool resetProcessing, Stretcher::Options options) :
    stretcher(sampleRate, buffer.getNumChannels(), options), buffer(buffer)
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
    outChannels = new float* [buffer.getNumChannels()];
}

BufferPitcher::~BufferPitcher()
{
    delete[] inChannels;
    delete[] outChannels;
}

void BufferPitcher::resetProcessing()
{
    processedBuffer = std::make_unique<juce::AudioBuffer<float>>(buffer.getNumChannels(), sampleEnd - sampleStart + stretcher.getStartDelay());
    stretcher.reset();
    juce::AudioBuffer<float> emptyBuffer = juce::AudioBuffer<float>(stretcher.getChannelCount(), stretcher.getPreferredStartPad());
    emptyBuffer.clear();
    // hopefully this is a proper thing to do, for some reason the requiredSamples stays high even afterwards
    stretcher.process(emptyBuffer.getArrayOfReadPointers(), emptyBuffer.getNumSamples(), false);
    //DBG(emptyBuffer.getNumSamples());

    totalPitchedSamples = 0;
    startDelay = stretcher.getStartDelay();
    expectedOutputSamples = (sampleEnd - sampleStart + 1) * stretcher.getTimeRatio();
    nextUnpitchedSample = sampleStart;
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
        int requiredSamples = stretcher.getSamplesRequired();
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
        if (totalPitchedSamples + availableSamples > processedBuffer->getNumSamples())
        {
            processedBuffer->setSize(processedBuffer->getNumChannels(), totalPitchedSamples + availableSamples, true);
        }
        // get output pointer
        for (auto ch = 0; ch < processedBuffer->getNumChannels(); ch++)
        {
            outChannels[ch] = processedBuffer->getWritePointer(ch, totalPitchedSamples);
        }
        // retrieve
        stretcher.retrieve(outChannels, availableSamples);
        totalPitchedSamples += availableSamples;
    }
}

int BufferPitcher::expectedExtraSamples()
{
    return stretcher.getPreferredStartPad();
}

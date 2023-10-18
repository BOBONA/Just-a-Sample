/*
  ==============================================================================

    CustomSamplerVoice.cpp
    Created: 5 Sep 2023 3:35:03pm
    Author:  binya

  ==============================================================================
*/

#include "CustomSamplerVoice.h"

CustomSamplerVoice::CustomSamplerVoice(double sampleRate, int numChannels) : numChannels(numChannels)
{
}

CustomSamplerVoice::~CustomSamplerVoice()
{
    delete bufferPitcher;
}

int CustomSamplerVoice::getEffectiveLocation()
{
    auto sampleConversion = sampleSound->sampleRate / getSampleRate();
    auto totalLength = sampleSound->getSampleEnd() - sampleSound->getSampleStart();
    auto percentage = ((float(currentSample) - bufferPitcher->startDelay) * sampleConversion - sampleSound->getSampleStart()) / (bufferPitcher->expectedExtraSamples() * sampleConversion + totalLength);
    auto loc = sampleSound->getSampleStart() + percentage * totalLength;
    return loc;
}

bool CustomSamplerVoice::canPlaySound(SynthesiserSound* sound)
{
    return bool(dynamic_cast<CustomSamplerSound*>(sound));
}

void CustomSamplerVoice::startNote(int midiNoteNumber, float velocity, SynthesiserSound* sound, int currentPitchWheelPosition)
{   
    this->velocity = velocity;
    pitchWheel = currentPitchWheelPosition;
    auto check = dynamic_cast<CustomSamplerSound*>(sound);
    auto newSound = sampleSound != check;
    sampleSound = check;
    if (sampleSound)
    {
        if (newSound)
        {
            delete bufferPitcher;
            bufferPitcher = new BufferPitcher(sampleSound->sample, getSampleRate(), numChannels, false);
        }
        auto sampleRateConversion = getSampleRate() / sampleSound->sampleRate;
        auto noteFreq = MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        bufferPitcher->setPitchScale(noteFreq / sampleSound->baseFreq / sampleRateConversion);
        bufferPitcher->setTimeRatio(sampleRateConversion);
        bufferPitcher->setSampleStart(sampleSound->getSampleStart());
        bufferPitcher->setSampleEnd(sampleSound->getSampleEnd());
        bufferPitcher->resetProcessing();

        currentSample = bufferPitcher->startDelay + sampleSound->getSampleStart() * sampleRateConversion;

        DBG("\n\n\nSTART " << bufferPitcher->expectedExtraSamples() << " DELAY " << bufferPitcher->startDelay << " TOTAL " << sampleSound->getSampleEnd() - sampleSound->getSampleStart());

        state = STARTING;
        smoothingSample = 0;
    }
}

void CustomSamplerVoice::stopNote(float velocity, bool allowTailOff)
{
    if (allowTailOff)
    {
        state = STOPPING;
        smoothingSample = 0;
    }
    else
    {
        state = STOPPED;
        clearCurrentNote();
    }
}

void CustomSamplerVoice::pitchWheelMoved(int newPitchWheelValue)
{
    pitchWheel = newPitchWheelValue;
}

void CustomSamplerVoice::controllerMoved(int controllerNumber, int newControllerValue)
{
}

void CustomSamplerVoice::renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (state == STOPPED)
        return;
    auto note = getCurrentlyPlayingNote();

    bufferPitcher->processSamples(currentSample, numSamples);
    
    // these temp variables are so that each channel is treated the same without modifying the overall context
    auto tempState = STOPPED;
    auto tempCurrentSample = 0;
    auto tempSmoothingSample = 0;
    for (auto ch = 0; ch < outputBuffer.getNumChannels(); ch++)
    {   
        tempState = state;
        tempCurrentSample = currentSample;
        tempSmoothingSample = smoothingSample;

        auto sampleChannel = ch;
        if (sampleChannel >= bufferPitcher->processedBuffer.getNumChannels())
            sampleChannel = bufferPitcher->processedBuffer.getNumChannels() - 1;
        for (auto i = startSample; i < startSample + numSamples; i++)
        {
            if (tempCurrentSample >= bufferPitcher->totalPitchedSamples)
            {
                tempState = STOPPED;
                clearCurrentNote();
                break;
            }
            float sample = bufferPitcher->processedBuffer.getSample(ch, tempCurrentSample);
            if (tempState == STOPPED)
            {
                continue;
            }
            if (tempState == STARTING)
            {
                double factor = double(tempSmoothingSample) / START_SAMPLES;
                sample *= factor;
                tempSmoothingSample++;
                if (tempSmoothingSample == START_SAMPLES) 
                {
                    tempState = PLAYING;
                }
            }
            else if (tempState == STOPPING)
            {
                double factor = (STOP_SAMPLES - tempSmoothingSample) / double(STOP_SAMPLES);
                sample *= factor;
                tempSmoothingSample++;
                if (tempSmoothingSample == STOP_SAMPLES) 
                {
                    tempState = STOPPED;
                    clearCurrentNote();
                }
            }
            outputBuffer.setSample(ch, i, outputBuffer.getSample(ch, i) + sample);
            tempCurrentSample++;
        }
    }
    state = tempState;
    currentSample = tempCurrentSample;
    smoothingSample = tempSmoothingSample;    
}

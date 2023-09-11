/*
  ==============================================================================

    CustomSamplerVoice.cpp
    Created: 5 Sep 2023 3:35:03pm
    Author:  binya

  ==============================================================================
*/

#include "CustomSamplerVoice.h"

CustomSamplerVoice::CustomSamplerVoice(double sampleRate, int numChannels) : stretcher(Stretcher(sampleRate, numChannels,
    Stretcher::OptionProcessRealTime | Stretcher::OptionEngineFiner | Stretcher::OptionWindowShort))
{
}

bool CustomSamplerVoice::canPlaySound(SynthesiserSound* sound)
{
    return bool(dynamic_cast<CustomSamplerSound*>(sound));
}

void CustomSamplerVoice::startNote(int midiNoteNumber, float velocity, SynthesiserSound* sound, int currentPitchWheelPosition)
{   
    this->velocity = velocity;
    pitchWheel = currentPitchWheelPosition;
    sampleSound = dynamic_cast<CustomSamplerSound*>(sound);
    if (sampleSound)
    {
        auto noteFreq = MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        phase = noteFreq * MathConstants<double>::twoPi;

        stretcher.reset();
        stretcher.setPitchScale(noteFreq / sampleSound->baseFreq);
        paddedSound.setSize(sampleSound->sample.getNumChannels(), sampleSound->sample.getNumSamples() + stretcher.getPreferredStartPad());
        paddedSound.clear();
        for (auto ch = 0; ch < sampleSound->sample.getNumChannels(); ch++)
        {
            paddedSound.copyFrom(ch, stretcher.getPreferredStartPad(), sampleSound->sample.getReadPointer(ch), sampleSound->sample.getNumSamples());
        }
        nextUnpitchedSample = 0;
        pitchedSound.setSize(sampleSound->sample.getNumChannels(), sampleSound->sample.getNumSamples() + stretcher.getStartDelay());
        pitchedSound.clear();
        totalPitchedSamples = 0;
        currentSample = stretcher.getStartDelay();

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
        const float** inChannels = new const float*[paddedSound.getNumChannels()];
        for (auto ch = 0; ch < paddedSound.getNumChannels(); ch++)
        {
            inChannels[ch] = paddedSound.getReadPointer(ch, nextUnpitchedSample);
        }
        // process
        stretcher.process(inChannels, requiredSamples, last);
        delete[] inChannels;
        nextUnpitchedSample += requiredSamples;
        // find amount of output samples
        auto availableSamples = stretcher.available();
        if (availableSamples <= 0 && last)
        {
            break;
        }
        if (totalPitchedSamples + availableSamples > pitchedSound.getNumSamples())
        {
            pitchedSound.setSize(pitchedSound.getNumChannels(), totalPitchedSamples + availableSamples, true);
        }
        // get output pointer
        float** outChannels = new float* [pitchedSound.getNumChannels()];
        for (auto ch = 0; ch < pitchedSound.getNumChannels(); ch++)
        {
            outChannels[ch] = pitchedSound.getWritePointer(ch, totalPitchedSamples);
        }
        // retrieve
        stretcher.retrieve(outChannels, availableSamples);
        delete[] outChannels;
        totalPitchedSamples += availableSamples;
    }

    // these temp variables are so that each channel is treated the same without modifying the overall context
    auto tempState = STOPPED;
    auto tempCurrentSample = 0;
    auto tempSmoothingSample = 0;
    auto tempPos = 0.f;
    for (auto ch = 0; ch < outputBuffer.getNumChannels(); ch++)
    {   
        tempState = state;
        tempCurrentSample = currentSample;
        tempSmoothingSample = smoothingSample;
        tempPos = pos;

        auto sampleChannel = ch;
        if (sampleChannel >= pitchedSound.getNumChannels())
            sampleChannel = pitchedSound.getNumChannels() - 1;
        for (auto i = startSample; i < startSample + numSamples; i++)
        {
            if (tempCurrentSample >= totalPitchedSamples)
            {
                tempState = STOPPED;
                break;
            }
            float sample = pitchedSound.getSample(ch, tempCurrentSample);
            sample = sin(tempPos);
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
            tempPos += phase / getSampleRate();
            tempCurrentSample++;
        }
    }
    state = tempState;
    currentSample = tempCurrentSample;
    smoothingSample = tempSmoothingSample;
    pos = tempPos;
}

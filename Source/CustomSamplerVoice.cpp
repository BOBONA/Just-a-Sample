/*
  ==============================================================================

    CustomSamplerVoice.cpp
    Created: 5 Sep 2023 3:35:03pm
    Author:  binya

  ==============================================================================
*/

#include "CustomSamplerVoice.h"

CustomSamplerVoice::CustomSamplerVoice(double sampleRate, int numChannels) : stretcher(Stretcher(sampleRate, numChannels,
    Stretcher::OptionProcessOffline | Stretcher::OptionEngineFiner | Stretcher::OptionPitchHighQuality))
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
        // stretcher processing, probably online should be used instead
        stretcher.reset();
        stretcher.setPitchScale(noteFreq / sampleSound->baseFreq);
        const float* const* samples = sampleSound->sample.getArrayOfReadPointers();
        stretcher.study(samples, sampleSound->sample.getNumSamples(), true);
        stretcher.process(samples, sampleSound->sample.getNumSamples(), true);
        shiftedSample.setSize(sampleSound->sample.getNumChannels(), sampleSound->sample.getNumSamples());
        stretcher.retrieve(shiftedSample.getArrayOfWritePointers(), stretcher.available());

        // sampleRatio = sampleSound->sampleRate / getSampleRate() * noteFreq / sampleSound->baseFreq;
        state = PLAYING;
        currentSample = 0;
    }
}

void CustomSamplerVoice::stopNote(float velocity, bool allowTailOff)
{
    if (allowTailOff)
    {
        state = STOPPING;
        stopSample = 0;
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
    auto& sound = sampleSound->sample;
    // these temp variables are so that each channel is treated the same without modifying the overall context
    auto tempState = STOPPED;
    auto tempCurrentSample = 0;
    auto tempStopSample = 0;
    for (auto ch = 0; ch < outputBuffer.getNumChannels(); ch++)
    {   
        tempState = state;
        tempCurrentSample = currentSample;
        tempStopSample = stopSample;

        auto sampleChannel = ch;
        if (sampleChannel >= sound.getNumChannels())
            sampleChannel = sound.getNumChannels() - 1;
        for (auto i = startSample; i < startSample + numSamples; i++)
        {
            int calculatedSampleIndex = sampleRatio * tempCurrentSample;
            if (calculatedSampleIndex >= sound.getNumSamples())
            {
                tempState = STOPPED;
                break;
            }
            float sample = shiftedSample.getSample(ch, calculatedSampleIndex);
            if (tempState == PLAYING)
            {
                outputBuffer.setSample(ch, i, outputBuffer.getSample(ch, i) + sample);
            }
            else if (tempState == STOPPING)
            {
                double factor = (NUM_STOP_SAMPLES - tempStopSample) / double(NUM_STOP_SAMPLES);
                outputBuffer.setSample(ch, i, outputBuffer.getSample(ch, i) + factor * sample);
                tempStopSample++;
                if (tempStopSample == NUM_STOP_SAMPLES) {
                    tempState = STOPPED;
                    clearCurrentNote();
                }
            }
            tempCurrentSample++;
        }
    }
    state = tempState;
    currentSample = tempCurrentSample;
    stopSample = tempStopSample;
}

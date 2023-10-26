/*
  ==============================================================================

    CustomSamplerVoice.cpp
    Created: 5 Sep 2023 3:35:03pm
    Author:  binya

  ==============================================================================
*/

#include "CustomSamplerVoice.h"

CustomSamplerVoice::CustomSamplerVoice(double sampleRate, int numChannels) : numChannels(numChannels), state(this)
{
}

CustomSamplerVoice::~CustomSamplerVoice()
{
    state.set(STOPPED);
    delete bufferPitcher;
}

int CustomSamplerVoice::getEffectiveLocation()
{
    if (!bufferPitcher)
    {
        return sampleSound->getSampleStart();
    }
    if (playbackMode == PluginParameters::PLAYBACK_MODES::ADVANCED)
    {
        return sampleSound->getSampleStart() + (currentSample - bufferPitcher->startDelay) / sampleRateConversion;
    }
    else
    {
        return sampleSound->getSampleStart() + currentSample * (noteFreq / sampleSound->baseFreq) / sampleRateConversion;
    }
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
        playbackMode = sampleSound->getPlaybackMode();
        sampleRateConversion = getSampleRate() / sampleSound->sampleRate;
        noteFreq = MidiMessage::getMidiNoteInHertz(midiNoteNumber);

        if (playbackMode == PluginParameters::PLAYBACK_MODES::ADVANCED)
        {
            if (newSound || !bufferPitcher)
            {
                delete bufferPitcher;
                bufferPitcher = new BufferPitcher(sampleSound->sample, getSampleRate(), numChannels, false);
            }
            bufferPitcher->setPitchScale(noteFreq / sampleSound->baseFreq / sampleRateConversion);
            bufferPitcher->setTimeRatio(sampleRateConversion);
            bufferPitcher->setSampleStart(sampleSound->getSampleStart());
            bufferPitcher->setSampleEnd(sampleSound->getSampleEnd());
            bufferPitcher->resetProcessing();

            currentSample = bufferPitcher->startDelay;
        }
        else
        {
            // for some reason, deleting bufferPitcher here causes an issue
            currentSample = 0;
        }
        
        smoothingSample = 0;
        state.set(STARTING);
    }
}

void CustomSamplerVoice::stopNote(float velocity, bool allowTailOff)
{
    if (allowTailOff)
    {
        state.set(STOPPING);
        smoothingSample = 0;
    }
    else
    {
        state.set(STOPPED);
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
    if (state.get() == STOPPED)
        return;
    auto note = getCurrentlyPlayingNote();

    if (playbackMode == PluginParameters::PLAYBACK_MODES::ADVANCED)
    {
        bufferPitcher->processSamples(currentSample, numSamples);
    }
    
    // these temp variables are so that each channel is treated the same without modifying the overall context
    auto tempState = STOPPED;
    auto tempCurrentSample = 0;
    auto tempSmoothingSample = 0;
    for (auto ch = 0; ch < outputBuffer.getNumChannels(); ch++)
    {   
        tempState = state.get();
        tempCurrentSample = currentSample;
        tempSmoothingSample = smoothingSample;
        for (auto i = startSample; i < startSample + numSamples; i++)
        {
            // fetch the appropriate sample depending on the mode
            float sample{ 0 };
            switch (playbackMode)
            {
            case PluginParameters::PLAYBACK_MODES::ADVANCED:
                if (tempCurrentSample >= bufferPitcher->totalPitchedSamples)
                {
                    tempState = STOPPED;
                    break;
                }
                sample = bufferPitcher->processedBuffer.getSample(ch, tempCurrentSample);
                break;
            case PluginParameters::PLAYBACK_MODES::BASIC:
                auto loc = sampleSound->getSampleStart() + tempCurrentSample * (noteFreq / sampleSound->baseFreq) / sampleRateConversion;
                if (loc >= sampleSound->getSampleEnd() || loc >= sampleSound->sample.getNumSamples())
                {
                    tempState = STOPPED;
                    break;
                }
                sample = sampleSound->sample.getSample(ch, loc);
                break;
            }
            // handle the states
            if (tempState == STOPPED)
            {
                break;
            }
            // linear smoothing
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
    state.set(tempState);
    currentSample = tempCurrentSample;
    smoothingSample = tempSmoothingSample;    
    if (state.get() == STOPPED)
    {
        clearCurrentNote();
    }
}
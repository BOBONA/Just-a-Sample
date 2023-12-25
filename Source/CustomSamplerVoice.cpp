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
    smoothingInitial.resize(numChannels);
    previousSample.resize(numChannels);
}

CustomSamplerVoice::~CustomSamplerVoice()
{
}

int CustomSamplerVoice::getEffectiveLocation()
{
    if (playbackMode == PluginParameters::ADVANCED)
    {
        if (!bufferPitcher)
        {
            return effectiveStart;
        }
        return effectiveStart + (currentSample - bufferPitcher->startDelay - effectiveStart) / sampleRateConversion;
    }
    else
    {
        return effectiveStart + (currentSample - effectiveStart) * (noteFreq / sampleSound->baseFreq) / sampleRateConversion;
    }
}

float CustomSamplerVoice::getSample(int channel, int sampleLocation, VoiceState voiceState=PLAYING)
{
    if (playbackMode == PluginParameters::BASIC)
    {
        return sampleSound->sample.getSample(channel, effectiveStart + (sampleLocation - effectiveStart) * (noteFreq / sampleSound->baseFreq) / sampleRateConversion);
    }
    else
    {
        // this is needed because of the multi-channel logic I use (otherwise I could just reference bufferPitcher)
        if (voiceState == PLAYING || voiceState == LOOPING)
        {
            return startBuffer->getSample(channel, sampleLocation - effectiveStart);
        }
        else if (voiceState == RELEASING)
        {
            return releaseBuffer->getSample(channel, sampleLocation - (sampleEnd + 1));
        }
    }
}

void CustomSamplerVoice::startSmoothing(bool zero=false)
{
    isSmoothing = true;
    smoothingSample = 0;
    for (int i = 0; i < numChannels; i++)
    {
        smoothingInitial.set(i, zero ? 0 : previousSample[i]);
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
        sampleRateConversion = getSampleRate() / sampleSound->sampleRate;
        noteFreq = MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        sampleStart = sampleSound->sampleStart.getValue();
        sampleEnd = sampleSound->sampleEnd.getValue();
        if (isLooping = sampleSound->isLooping.getValue())
        {
            if (loopingHasStart = sampleSound->loopingHasStart.getValue())
                loopStart = sampleSound->loopStart.getValue();
            if (loopingHasEnd = sampleSound->loopingHasEnd.getValue())
                loopEnd = sampleSound->loopEnd.getValue();
        }
        effectiveStart = (isLooping && loopingHasStart) ? loopStart : sampleStart;
        effectiveEnd = (isLooping && loopingHasEnd) ? loopEnd : sampleEnd;
        playbackMode = sampleSound->getPlaybackMode();

        if (playbackMode == PluginParameters::ADVANCED)
        {
            if (newSound || !bufferPitcher)
            {
                bufferPitcher = std::make_unique<BufferPitcher>(sampleSound->sample, getSampleRate(), numChannels, false);
            }
            bufferPitcher->setPitchScale(noteFreq / sampleSound->baseFreq / sampleRateConversion);
            bufferPitcher->setTimeRatio(sampleRateConversion);
            bufferPitcher->setSampleStart(effectiveStart);
            bufferPitcher->setSampleEnd(sampleEnd);
            bufferPitcher->resetProcessing();
            startBuffer = bufferPitcher->processedBuffer;

            currentSample = bufferPitcher->startDelay + effectiveStart;
        }
        else
        {
            // for some reason, deleting bufferPitcher here causes an issue
            currentSample = effectiveStart;
        }
        startSmoothing(true);
        
        state = isLooping && !loopingHasStart ? LOOPING : PLAYING;
    }
}

void CustomSamplerVoice::stopNote(float velocity, bool allowTailOff)
{
    if (allowTailOff)
    {
        if (state == RELEASING) // this is to handle juce calling stopNote multiple times
        {
            return;
        }
        if (isLooping && loopingHasEnd)
        {
            startSmoothing();
            state = RELEASING;
            if (playbackMode == PluginParameters::BASIC)
            {
                currentSample = effectiveStart + (sampleEnd + 1 - effectiveStart) / (noteFreq / sampleSound->baseFreq) * sampleRateConversion;
            }
            else
            {
                bufferPitcher->setSampleStart(sampleEnd + 1);
                bufferPitcher->setSampleEnd(loopEnd);
                bufferPitcher->resetProcessing();
                releaseBuffer = bufferPitcher->processedBuffer;
                effectiveStart = sampleEnd + 1;
                currentSample = effectiveStart + bufferPitcher->startDelay;
            }
        }
        else
        {
            startSmoothing();
            state = STOPPING;
        }
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

    if (playbackMode == PluginParameters::ADVANCED)
    {
        bufferPitcher->processSamples(currentSample - effectiveStart, numSamples);
    }
    
    // these temp variables are so that each channel is treated the same without modifying the overall context
    VoiceState tempState = STOPPED;
    int tempCurrentSample = 0;
    bool tempIsSmoothing = false;
    int tempSmoothingSample = 0;
    int tempEffectiveStart = 0;
    for (auto ch = 0; ch < outputBuffer.getNumChannels(); ch++)
    {   
        tempState = state;
        tempCurrentSample = currentSample;
        tempIsSmoothing = isSmoothing;
        tempSmoothingSample = smoothingSample;
        tempEffectiveStart = effectiveStart;
        auto effectiveCh = ch % sampleSound->sample.getNumChannels();
        for (auto i = startSample; i < startSample + numSamples; i++)
        {
            if (tempState == STOPPED)
            {
                break;
            }
            // fetch the appropriate sample depending on the mode
            float sample{ 0 };
            if (tempState != STOPPING) // a bit finnicky 
            {
                switch (playbackMode)
                {
                case PluginParameters::ADVANCED:
                    sample = getSample(effectiveCh, tempCurrentSample, tempState);
                    tempCurrentSample++;
                    // handle end of current buffer: 1. expected length has been reached, 2. bufferPitcher stopped at less than expected, 3. a sentinel 0 sample has been reached
                    if (tempCurrentSample - tempEffectiveStart - bufferPitcher->startDelay >= bufferPitcher->expectedOutputSamples || 
                        (tempCurrentSample - tempEffectiveStart >= bufferPitcher->totalPitchedSamples && i + 1 < startSample + numSamples) || 
                        sample == 0)
                    {
                        if (tempState == PLAYING && isLooping)
                        {
                            tempState = LOOPING;
                        } 
                        else if (tempState == LOOPING)
                        {
                            tempIsSmoothing = true;
                            tempSmoothingSample = 0;
                            smoothingInitial.set(ch, previousSample[ch]);
                            // the need for this type of logic makes me wonder about my decisions
                            tempCurrentSample = bufferPitcher->startDelay + effectiveStart + (sampleStart - effectiveStart) * sampleRateConversion;
                        }
                        else if (tempState == RELEASING || !isLooping) // end playback
                        {
                            tempState = STOPPING;
                            tempSmoothingSample = 0;
                            smoothingInitial.set(ch, previousSample[ch]);
                            break;
                        }
                    }
                    break;
                case PluginParameters::BASIC:
                    // we use effectiveStart since the temp thing is only needed in ADVANCED
                    auto loc = effectiveStart + (tempCurrentSample - effectiveStart) * (noteFreq / sampleSound->baseFreq) / sampleRateConversion;
                    if ((loc > effectiveEnd || loc >= sampleSound->sample.getNumSamples()) && !(isLooping && !loopingHasEnd))
                    {
                        tempState = STOPPING;
                        tempSmoothingSample = 0;
                        smoothingInitial.set(ch, previousSample[ch]);
                        break;
                    }
                    // handle loop states
                    if (isLooping)
                    {
                        if (tempState == PLAYING && loc >= sampleStart)
                        {
                            tempState = LOOPING;
                        }
                        else if (tempState == LOOPING && loc > sampleEnd)
                        {
                            tempIsSmoothing = true;
                            tempSmoothingSample = 0;
                            smoothingInitial.set(ch, previousSample[ch]);
                            tempCurrentSample = effectiveStart + (sampleStart - effectiveStart) / (noteFreq / sampleSound->baseFreq) * sampleRateConversion;
                            loc = effectiveStart + (tempCurrentSample - effectiveStart) * (noteFreq / sampleSound->baseFreq) / sampleRateConversion;
                        }
                    }
                    sample = sampleSound->sample.getSample(effectiveCh, loc);
                    tempCurrentSample++;
                    break;
                }
            }
            // handle smoothing
            if (tempIsSmoothing || tempState == STOPPING)
            {
                sample = float(SMOOTHING_SAMPLES - tempSmoothingSample) / SMOOTHING_SAMPLES * smoothingInitial[ch] +
                    float(tempSmoothingSample) / SMOOTHING_SAMPLES * (tempState == STOPPING ? 0 : sample);
                tempSmoothingSample++;
                if (tempSmoothingSample >= SMOOTHING_SAMPLES)
                {
                    tempIsSmoothing = false;
                    if (tempState == STOPPING)
                    {
                        tempState = STOPPED;
                        break;
                    }
                }
            }
            // gain scale
            sample = sample * Decibels::decibelsToGain(float(sampleSound->gain.getValue()));
            outputBuffer.addSample(ch, i, sample);
            previousSample.set(ch, sample);
        }
    }
    state = tempState;
    currentSample = tempCurrentSample;
    isSmoothing = tempIsSmoothing;
    smoothingSample = tempSmoothingSample;
    effectiveStart = tempEffectiveStart;
    if (state == STOPPED)
    {
        clearCurrentNote();
    }
}

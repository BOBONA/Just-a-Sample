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
    if (playbackMode == PluginParameters::PLAYBACK_MODES::ADVANCED)
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

void CustomSamplerVoice::startSmoothing(int smoothingInitial)
{
    isSmoothing = true;
    smoothingSample = 0;
    this->smoothingInitial = smoothingInitial;
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

        if (playbackMode == PluginParameters::PLAYBACK_MODES::ADVANCED)
        {
            if (newSound || !bufferPitcher)
            {
                delete bufferPitcher;
                bufferPitcher = new BufferPitcher(sampleSound->sample, getSampleRate(), numChannels, false);
            }
            bufferPitcher->setPitchScale(noteFreq / sampleSound->baseFreq / sampleRateConversion);
            bufferPitcher->setTimeRatio(sampleRateConversion);
            bufferPitcher->setSampleStart(effectiveStart);
            bufferPitcher->setSampleEnd(effectiveEnd);
            bufferPitcher->resetProcessing();

            currentSample = bufferPitcher->startDelay + effectiveStart;
        }
        else
        {
            // for some reason, deleting bufferPitcher here causes an issue
            currentSample = effectiveStart;
        }
        startSmoothing(-1);
        
        state = PLAYING;
    }
}

void CustomSamplerVoice::stopNote(float velocity, bool allowTailOff)
{
    if (allowTailOff)
    {
        if (state == RELEASING)
        {
            return;
        }
        if (isLooping && loopingHasEnd && playbackMode == PluginParameters::PLAYBACK_MODES::BASIC)
        {
            state = RELEASING;
            startSmoothing(currentSample);
            currentSample = effectiveStart + (sampleEnd + 1 - effectiveStart) / (noteFreq / sampleSound->baseFreq) * sampleRateConversion;
        }
        else
        {
            state = STOPPING;
            startSmoothing(currentSample - 1);
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

    if (playbackMode == PluginParameters::PLAYBACK_MODES::ADVANCED)
    {
        bufferPitcher->processSamples(currentSample - effectiveStart, numSamples);
    }
    
    // these temp variables are so that each channel is treated the same without modifying the overall context
    auto tempState = STOPPED;
    auto tempCurrentSample = 0;
    auto tempIsSmoothing = false;
    auto tempSmoothingSample = 0;
    auto tempSmoothingInitial = -1;
    for (auto ch = 0; ch < outputBuffer.getNumChannels(); ch++)
    {   
        tempState = state;
        tempCurrentSample = currentSample;
        tempIsSmoothing = isSmoothing;
        tempSmoothingSample = smoothingSample;
        tempSmoothingInitial = smoothingInitial;
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
                case PluginParameters::PLAYBACK_MODES::ADVANCED:
                    if (tempCurrentSample - effectiveStart >= bufferPitcher->totalPitchedSamples)
                    {
                        tempState = STOPPING;
                        tempSmoothingSample = 0;
                        tempSmoothingInitial = tempCurrentSample - 1;
                        break;
                    }
                    sample = bufferPitcher->processedBuffer.getSample(ch, tempCurrentSample - effectiveStart);
                    tempCurrentSample++;
                    break;
                case PluginParameters::PLAYBACK_MODES::BASIC:
                    auto loc = effectiveStart + (tempCurrentSample - effectiveStart) * (noteFreq / sampleSound->baseFreq) / sampleRateConversion;
                    if ((loc > effectiveEnd || loc >= sampleSound->sample.getNumSamples()) && !(isLooping && !loopingHasEnd))
                    {
                        tempState = STOPPING;
                        tempSmoothingSample = 0;
                        tempSmoothingInitial = tempCurrentSample;
                        break;
                    }
                    sample = sampleSound->sample.getSample(ch, loc);
                    tempCurrentSample++;
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
                            tempSmoothingInitial = tempCurrentSample - 1;
                            tempCurrentSample = effectiveStart + (sampleStart - effectiveStart) / (noteFreq / sampleSound->baseFreq) * sampleRateConversion;
                        }
                    }
                    break;
                }
            }
            // handle smoothing
            if (tempIsSmoothing || tempState == STOPPING)
            {
                float smoothingI;
                if (tempSmoothingInitial == -1)
                {
                    smoothingI = 0.f;
                }
                else if (playbackMode == PluginParameters::ADVANCED)
                {
                    smoothingI = bufferPitcher->processedBuffer.getSample(ch, tempSmoothingInitial - effectiveStart);
                }
                else
                {
                    int smoothingILoc = effectiveStart + (tempSmoothingInitial - effectiveStart) * (noteFreq / sampleSound->baseFreq) / sampleRateConversion;
                    smoothingI = sampleSound->sample.getSample(ch, jmin<int>(smoothingILoc, sampleSound->sample.getNumSamples() - 1));
                }
                sample = float(SMOOTHING_SAMPLES - tempSmoothingSample) / SMOOTHING_SAMPLES * smoothingI + 
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
            outputBuffer.setSample(ch, i, outputBuffer.getSample(ch, i) + sample);
        }
    }
    state = tempState;
    currentSample = tempCurrentSample;
    isSmoothing = tempIsSmoothing;
    smoothingSample = tempSmoothingSample;
    smoothingInitial = tempSmoothingInitial;
    if (state == STOPPED)
    {
        clearCurrentNote();
    }
}

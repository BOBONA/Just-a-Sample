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

float CustomSamplerVoice::getSample(int channel, int sampleLocation, VoiceState voiceState)
{
    if (playbackMode == PluginParameters::PLAYBACK_MODES::BASIC)
    {
        return sampleSound->sample.getSample(channel, effectiveStart + (sampleLocation - effectiveStart) * (noteFreq / sampleSound->baseFreq) / sampleRateConversion);
    }
    else
    {
        // sampleLocation represents the location relative to the total outputted buffer, as such fetching the sample depends on which state we're in
        if (voiceState == PLAYING)
        {
            return startBuffer->getSample(channel, sampleLocation - effectiveStart);
        }
        else if (voiceState == LOOPING)
        {
            return loopBuffer->getSample(channel, sampleLocation - sampleStart);
        }
        else if (voiceState == RELEASING)
        {

        }
    }
}

void CustomSamplerVoice::startSmoothing(float smoothingInitial)
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
                bufferPitcher = std::make_unique<BufferPitcher>(sampleSound->sample, getSampleRate(), numChannels, false);
            }
            bufferPitcher->setPitchScale(noteFreq / sampleSound->baseFreq / sampleRateConversion);
            bufferPitcher->setTimeRatio(sampleRateConversion);
            bufferPitcher->setSampleStart(effectiveStart);
            bufferPitcher->setSampleEnd(isLooping && loopingHasStart ? sampleStart - 1 : effectiveEnd);
            bufferPitcher->resetProcessing();

            startBuffer = bufferPitcher->processedBuffer;

            currentSample = bufferPitcher->startDelay + effectiveStart;
        }
        else
        {
            // for some reason, deleting bufferPitcher here causes an issue
            currentSample = effectiveStart;
        }
        startSmoothing(0);
        
        state = PLAYING;
    }
}

void CustomSamplerVoice::stopNote(float velocity, bool allowTailOff)
{
    if (allowTailOff)
    {
        if (state == RELEASING) // this is to handle Juce calling stopNote multiple times
        {
            return;
        }
        if (isLooping && loopingHasEnd && playbackMode == PluginParameters::PLAYBACK_MODES::BASIC) // TODO this should be even with ADVANCED
        {
            startSmoothing(getSample(0, currentSample, state));
            state = RELEASING;
            currentSample = effectiveStart + (sampleEnd + 1 - effectiveStart) / (noteFreq / sampleSound->baseFreq) * sampleRateConversion;
        }
        else
        {
            startSmoothing(getSample(0, currentSample - 1, state));
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

    if (playbackMode == PluginParameters::PLAYBACK_MODES::ADVANCED)
    {
        if (state == PLAYING)
        {
            bufferPitcher->processSamples(currentSample - effectiveStart, numSamples);
        } 
        else if (state == LOOPING)
        {
            bufferPitcher->processSamples(currentSample - sampleStart, numSamples);
        }
        else if (state == RELEASING)
        {

        }
    }
    
    // these temp variables are so that each channel is treated the same without modifying the overall context
    VoiceState tempState = STOPPED;
    int tempCurrentSample = 0;
    bool tempIsSmoothing = false;
    int tempSmoothingSample = 0;
    float tempSmoothingInitial = 0.f;
    int tempEffectiveStart = 0;
    for (auto ch = 0; ch < outputBuffer.getNumChannels(); ch++)
    {   
        tempState = state;
        tempCurrentSample = currentSample;
        tempIsSmoothing = isSmoothing;
        tempSmoothingSample = smoothingSample;
        tempSmoothingInitial = smoothingInitial;
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
                case PluginParameters::PLAYBACK_MODES::ADVANCED:
                    sample = getSample(effectiveCh, tempCurrentSample, tempState);
                    tempCurrentSample++;
                    // handle end of current buffer
                    if ((tempCurrentSample - tempEffectiveStart >= bufferPitcher->totalPitchedSamples && i + 1 < startSample + numSamples) || sample == 0)
                    {
                        float previousSample = sample != 0 ? sample :
                            bufferPitcher->processedBuffer->getSample(effectiveCh, tempCurrentSample - tempEffectiveStart - 1); // needed for smoothing
                        if (tempState == PLAYING && isLooping)
                        {
                            // TODO will need to change the play/loop/release buffer bounds for the SampleEditor
                            // need to process some looping samples and set up buffer pitcher
                            if (ch == 0)
                            {
                                bufferPitcher->setSampleStart(sampleStart);
                                bufferPitcher->setSampleEnd(sampleEnd);
                                bufferPitcher->resetProcessing();
                                loopBuffer = bufferPitcher->processedBuffer;
                                bufferPitcher->processSamples(bufferPitcher->startDelay, numSamples - i + 1);
                            }
                            tempCurrentSample = sampleStart + bufferPitcher->startDelay;
                            tempEffectiveStart = sampleStart;
                            tempState = LOOPING;
                            tempIsSmoothing = true;
                            tempSmoothingSample = 0;
                            tempSmoothingInitial = previousSample;
                        } 
                        else if (tempState == LOOPING)
                        {
                            tempIsSmoothing = true;
                            tempSmoothingSample = 0;
                            tempSmoothingInitial = previousSample;
                            tempCurrentSample = sampleStart + bufferPitcher->startDelay;
                        }
                        else if (tempState == RELEASING || !isLooping) // end playback
                        {
                            tempState = STOPPING;
                            tempSmoothingSample = 0;
                            tempSmoothingInitial = previousSample;
                            break;
                        }
                    }
                    break;
                case PluginParameters::PLAYBACK_MODES::BASIC:
                    // we use effectiveStart since the temp thing is only needed in ADVANCED
                    auto loc = effectiveStart + (tempCurrentSample - effectiveStart) * (noteFreq / sampleSound->baseFreq) / sampleRateConversion;
                    if ((loc > effectiveEnd || loc >= sampleSound->sample.getNumSamples()) && !(isLooping && !loopingHasEnd))
                    {
                        tempState = STOPPING;
                        tempSmoothingSample = 0;
                        tempSmoothingInitial = sampleSound->sample.getSample(effectiveCh, jmin<int>(loc, sampleSound->sample.getNumSamples() - 1));;
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
                            tempSmoothingInitial = sample;
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
                sample = float(SMOOTHING_SAMPLES - tempSmoothingSample) / SMOOTHING_SAMPLES * tempSmoothingInitial + 
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
            outputBuffer.addSample(ch, i, sample);
        }
    }
    state = tempState;
    currentSample = tempCurrentSample;
    isSmoothing = tempIsSmoothing;
    smoothingSample = tempSmoothingSample;
    smoothingInitial = tempSmoothingInitial;
    effectiveStart = tempEffectiveStart;
    if (state == STOPPED)
    {
        clearCurrentNote();
    }
}

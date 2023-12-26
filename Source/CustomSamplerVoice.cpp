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
    previousSample.resize(numChannels);
}

CustomSamplerVoice::~CustomSamplerVoice()
{
}

int CustomSamplerVoice::getEffectiveLocation()
{
    if (playbackMode == PluginParameters::ADVANCED)
    {
        if (!startBuffer)
        {
            return effectiveStart;
        }
        return effectiveStart + (currentSample - getBufferPitcher(state)->startDelay - effectiveStart) / sampleRateConversion;
    }
    else
    {
        return effectiveStart + (currentSample - effectiveStart) * (noteFreq / sampleSound->baseFreq) / sampleRateConversion;
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
            // initialize startBuffer
            if (newSound || !startBuffer)
            {
                startBuffer = std::make_unique<BufferPitcher>(sampleSound->sample, getSampleRate(), numChannels, false);
            }
            startBuffer->setPitchScale(noteFreq / sampleSound->baseFreq / sampleRateConversion);
            startBuffer->setTimeRatio(sampleRateConversion);
            startBuffer->setSampleStart(effectiveStart);
            startBuffer->setSampleEnd(sampleEnd);
            startBuffer->resetProcessing(!PluginParameters::PREPROCESS_STEP);

            // initialize releaseBuffer
            if (isLooping && loopingHasEnd)
            {
                if (newSound || !releaseBuffer)
                {
                    releaseBuffer = std::make_unique<BufferPitcher>(sampleSound->sample, getSampleRate(), numChannels, false);
                }
                releaseBuffer->setPitchScale(noteFreq / sampleSound->baseFreq / sampleRateConversion);
                releaseBuffer->setTimeRatio(sampleRateConversion);
                releaseBuffer->setSampleStart(sampleEnd + 1);
                releaseBuffer->setSampleEnd(loopEnd);
                releaseBuffer->resetProcessing(!PluginParameters::PREPROCESS_STEP);
            }

            currentSample = startBuffer->startDelay + effectiveStart;
            preprocessingSample = 0;
            preprocessingTotalSamples = startBuffer->startPad() + ((isLooping && loopingHasEnd) ? releaseBuffer->startPad() : 0);
            if (PluginParameters::PREPROCESS_STEP)
            {
                state = PREPROCESSING;
            }
            else
            {
                state = isLooping && !loopingHasStart ? LOOPING : PLAYING;
            }
        }
        else
        {
            currentSample = effectiveStart;
            state = isLooping && !loopingHasStart ? LOOPING : PLAYING;
        }
        midiReleased = false;
        midiReleasedSamples = 0;
        if (PluginParameters::SMOOTHING)
        {
            isSmoothingStart = true;
            isSmoothingLoop = false;
            isSmoothingRelease = false;
            isSmoothingEnd = false;
            smoothingStartSample = 0;
            smoothingLoopSample = 0;
            smoothingReleaseSample = 0;
            smoothingEndSample = 0;
        }
        doLoopSmoothing = PluginParameters::SMOOTHING && sampleEnd - sampleStart + 1 >= SMOOTHING_SAMPLES * 4;
    }
}

void CustomSamplerVoice::stopNote(float velocity, bool allowTailOff)
{
    if (allowTailOff)
    {
        midiReleased = true;
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
    // preprocessing step to reduce audio glitches (at the expense of latency)
    if (state == PREPROCESSING)
    {
        int startProcessSamples = juce::jmax(0, juce::jmin(numSamples, startBuffer->startPad() - preprocessingSample));
        if (startProcessSamples > 0)
        {
            startBuffer->processZeros(startProcessSamples);
            preprocessingSample += startProcessSamples;
            startSample += startProcessSamples;
            numSamples -= startProcessSamples;
        }
        if (isLooping && loopingHasEnd)
        {
            int releaseProcessSamples = juce::jmax(0, juce::jmin(numSamples - startProcessSamples, releaseBuffer->startPad() - (preprocessingSample - startBuffer->startPad())));
            if (releaseProcessSamples > 0)
            {
                releaseBuffer->processZeros(releaseProcessSamples);
                preprocessingSample += releaseProcessSamples;
                startSample += releaseProcessSamples;
                numSamples -= releaseProcessSamples;
            }
        }
        if (preprocessingSample >= preprocessingTotalSamples)
        {
            if (state == PREPROCESSING)
            {
                state = isLooping && !loopingHasStart ? LOOPING : PLAYING;
            }
            else
            {
                state = RELEASING;
                effectiveStart = sampleEnd + 1;
                currentSample = effectiveStart + releaseBuffer->startDelay;
            }
        }
        else
        {
            return;
        }
    }

    // handle midi release
    if (midiReleased && midiReleasedSamples == 0)
    {
        if (isLooping && loopingHasEnd)
        {
            if (PluginParameters::SMOOTHING)
            {
                isSmoothingRelease = true;
                // move this elsewhere binyamin!
                releaseBuffer->processSamples(releaseBuffer->startDelay, SMOOTHING_SAMPLES);
            }
            else
            {
                // this will need to change once i add the latency delay thing
                state = RELEASING;
                if (playbackMode == PluginParameters::BASIC)
                {
                    currentSample = effectiveStart + (sampleEnd + 1 - effectiveStart) / (noteFreq / sampleSound->baseFreq) * sampleRateConversion;
                }
                else
                {
                    effectiveStart = sampleEnd + 1;
                    currentSample = effectiveStart + releaseBuffer->startDelay;
                }
            }
        }
        else
        {
            if (PluginParameters::SMOOTHING)
            {
                isSmoothingEnd = true;
            }
            else
            {
                state = STOPPED;
                clearCurrentNote();
                return;
            }
        }
    }

    auto note = getCurrentlyPlayingNote();

    if (playbackMode == PluginParameters::ADVANCED)
    {
        if (midiReleased && releaseBuffer && state != RELEASING) // must handle the case where we start using the release buffer midway through
        {
            startBuffer->processSamples(currentSample - effectiveStart, juce::jmin(SMOOTHING_SAMPLES - midiReleasedSamples, numSamples));
        }
        else
        {
            getBufferPitcher(state)->processSamples(currentSample - effectiveStart, numSamples);
        }
    }
    
    // these temp variables are so that each channel is treated the same without modifying the overall context (I know it's ugly)
    VoiceState tempState = STOPPED;
    int tempCurrentSample = 0;
    int tempEffectiveStart = 0;
    int tempMidiReleasedSamples = 0;
    bool tempIsSmoothingStart = false;
    bool tempIsSmoothingLoop = false;
    bool tempIsSmoothingRelease = false;
    bool tempIsSmoothingEnd = false;
    int tempSmoothingStartSample = 0;
    int tempSmoothingLoopSample = 0;
    int tempSmoothingReleaseSample = 0;
    int tempSmoothingEndSample = 0;
    for (auto ch = 0; ch < outputBuffer.getNumChannels(); ch++)
    {   
        tempState = state;
        tempCurrentSample = currentSample;
        tempEffectiveStart = effectiveStart;
        tempMidiReleasedSamples = midiReleasedSamples;
        tempIsSmoothingStart = isSmoothingStart;
        tempIsSmoothingLoop = isSmoothingLoop;
        tempIsSmoothingRelease = isSmoothingRelease;
        tempIsSmoothingEnd = isSmoothingEnd;
        tempSmoothingStartSample = smoothingStartSample;
        tempSmoothingLoopSample = smoothingLoopSample;
        tempSmoothingReleaseSample = smoothingReleaseSample;
        tempSmoothingEndSample = smoothingEndSample;
        auto effectiveCh = ch % sampleSound->sample.getNumChannels();
        for (auto i = startSample; i < startSample + numSamples; i++)
        {
            if (tempState == STOPPED)
            {
                break;
            }
            if (midiReleased && tempState != RELEASING)
            {
                tempMidiReleasedSamples++;
            }
            // fetch the appropriate sample depending on the mode
            float sample{ 0 };
            if (playbackMode == PluginParameters::ADVANCED)
            {
                sample = getBufferPitcher(tempState)->processedBuffer.getSample(effectiveCh, tempCurrentSample - tempEffectiveStart);
                // handle next sample being outside of current buffer: 1. expected length has been reached, /*2. bufferPitcher stopped at less than expected*/
                auto& bufferPitcher = getBufferPitcher(tempState);
                if (tempCurrentSample - tempEffectiveStart - bufferPitcher->startDelay + 2 >= bufferPitcher->expectedOutputSamples/* ||
                    (tempCurrentSample - tempEffectiveStart >= bufferPitcher->totalPitchedSamples && i + 1 < startSample + numSamples)*/)
                {
                    if (tempState == PLAYING && isLooping)
                    {
                        tempState = LOOPING;
                    }
                    else if (tempState == LOOPING)
                    {
                        // the need for this type of logic makes me wonder about my decisions
                        tempCurrentSample = bufferPitcher->startDelay + tempEffectiveStart + (sampleStart - tempEffectiveStart) * sampleRateConversion - 1;
                        if (doLoopSmoothing)
                        {
                            tempCurrentSample += SMOOTHING_SAMPLES;
                        }
                    }
                    else if (tempState == RELEASING || !isLooping) // end playback after current sample
                    {
                        tempState = STOPPED;
                    }
                }
                // handle loop smoothing start
                else if (doLoopSmoothing && (tempState == PLAYING || tempState == LOOPING) && 
                    tempCurrentSample - tempEffectiveStart - bufferPitcher->startDelay == bufferPitcher->expectedOutputSamples - SMOOTHING_SAMPLES)
                {
                    tempSmoothingLoopSample = 0;
                    tempIsSmoothingLoop = true;
                }
                // handle release smoothing start
                else if (tempState == RELEASING && tempCurrentSample - tempEffectiveStart - bufferPitcher->startDelay == bufferPitcher->expectedOutputSamples - SMOOTHING_SAMPLES)
                {
                    tempIsSmoothingEnd = true;
                }
                tempCurrentSample++;
            }
            else if (playbackMode == PluginParameters::BASIC)
            {
                /*auto loc = tempEffectiveStart + (tempCurrentSample - tempEffectiveStart) * (noteFreq / sampleSound->baseFreq) / sampleRateConversion;
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
                tempCurrentSample++;*/
            }
            // handle smoothing
            if (PluginParameters::SMOOTHING)
            {
                if (tempIsSmoothingStart)
                {
                    sample *= float(tempSmoothingStartSample) / SMOOTHING_SAMPLES;
                    tempSmoothingStartSample++;
                    if (tempSmoothingStartSample >= SMOOTHING_SAMPLES)
                    {
                        tempIsSmoothingStart = false;
                    }
                }
                if (tempIsSmoothingLoop)
                {
                    float loopStartSample = playbackMode == PluginParameters::BASIC ? 0 : 
                        startBuffer->processedBuffer.getSample(effectiveCh, tempSmoothingLoopSample + startBuffer->startDelay + (sampleStart - tempEffectiveStart) * sampleRateConversion);
                    sample = sample * float(SMOOTHING_SAMPLES - tempSmoothingLoopSample) / SMOOTHING_SAMPLES + 
                             loopStartSample * float(tempSmoothingLoopSample) / SMOOTHING_SAMPLES;
                    tempSmoothingLoopSample++;
                    if (tempSmoothingLoopSample >= SMOOTHING_SAMPLES)
                    {
                        tempIsSmoothingLoop = false;
                    }
                }
                if (tempIsSmoothingRelease)
                {
                    float releaseSample = playbackMode == PluginParameters::BASIC ? 0 :
                        releaseBuffer->processedBuffer.getSample(effectiveCh, tempSmoothingReleaseSample + releaseBuffer->startDelay);
                    sample = sample * float(SMOOTHING_SAMPLES - tempSmoothingReleaseSample) / SMOOTHING_SAMPLES +
                        releaseSample * float(tempSmoothingReleaseSample) / SMOOTHING_SAMPLES;
                    tempSmoothingReleaseSample++;
                    if (tempSmoothingReleaseSample >= SMOOTHING_SAMPLES)
                    {
                        tempIsSmoothingRelease = false;
                        tempIsSmoothingLoop = false; // this is necessary for certain timings
                        tempState = RELEASING;
                        if (playbackMode == PluginParameters::BASIC)
                        {
                            tempCurrentSample = SMOOTHING_SAMPLES + tempEffectiveStart + (sampleEnd + 1 - effectiveStart) / (noteFreq / sampleSound->baseFreq) * sampleRateConversion;
                        }
                        else
                        {
                            tempEffectiveStart = sampleEnd + 1;
                            tempCurrentSample = SMOOTHING_SAMPLES + tempEffectiveStart + releaseBuffer->startDelay;
                            if (effectiveCh == 0)
                            {
                                releaseBuffer->processSamples(tempCurrentSample - tempEffectiveStart, startSample + numSamples - i);
                            }
                        }
                    }
                }
                if (tempIsSmoothingEnd)
                {
                    sample *= float(SMOOTHING_SAMPLES - tempSmoothingEndSample) / SMOOTHING_SAMPLES;
                    tempSmoothingEndSample++;
                    if (tempSmoothingEndSample >= SMOOTHING_SAMPLES)
                    {
                        tempState = STOPPED;
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
    effectiveStart = tempEffectiveStart;
    midiReleasedSamples = tempMidiReleasedSamples;
    isSmoothingStart = tempIsSmoothingStart;
    isSmoothingLoop = tempIsSmoothingLoop;
    isSmoothingRelease = tempIsSmoothingRelease;
    isSmoothingEnd = tempIsSmoothingEnd;
    smoothingStartSample = tempSmoothingStartSample;
    smoothingLoopSample = tempSmoothingLoopSample;
    smoothingReleaseSample = tempSmoothingReleaseSample;
    smoothingEndSample = tempSmoothingEndSample;
    if (state == STOPPED)
    {
        clearCurrentNote();
    }
}

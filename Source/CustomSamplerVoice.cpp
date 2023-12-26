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
        if (doStartStopSmoothing = sampleSound->doStartStopSmoothing)
        {
            startStopSmoothingSamples = sampleSound->startStopSmoothingSamples;
        }
        if (doCrossfadeSmoothing = sampleSound->doCrossfadeSmoothing)
        {
            crossfadeSmoothingSamples = juce::jmin(sampleSound->crossfadeSmoothingSamples, (sampleEnd - sampleStart + 1) / 4);
        }
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
            if (PluginParameters::PREPROCESS_STEP)
            {
                preprocessingSample = 0;
                preprocessingTotalSamples = startBuffer->startPad() + ((isLooping && loopingHasEnd) ? releaseBuffer->startPad() : 0);
                state = PREPROCESSING;
            }
            else
            {
                preprocessingTotalSamples = 0;
                state = isLooping && !loopingHasStart ? LOOPING : PLAYING;
            }
        }
        else
        {
            currentSample = effectiveStart;
            state = isLooping && !loopingHasStart ? LOOPING : PLAYING;
        }
        noteDuration = 0;
        midiReleased = false;
        midiReleasedSamples = 0;
        isSmoothingStart = true;
        isSmoothingLoop = false;
        isSmoothingRelease = false;
        isSmoothingEnd = false;
        smoothingStartSample = 0;
        smoothingLoopSample = 0;
        smoothingReleaseSample = 0;
        smoothingEndSample = 0;
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
            if (!midiReleased)
            {
                noteDuration += startProcessSamples;
            }
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
                if (!midiReleased)
                {
                    noteDuration += startProcessSamples;
                }
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

    // buffer pitching for numSamples
    if (playbackMode == PluginParameters::ADVANCED)
    {
        if (midiReleased && releaseBuffer && state != RELEASING) // must handle the case where we might start using the release buffer midway through
        {
            startBuffer->processSamples(currentSample - effectiveStart, juce::jmin(crossfadeSmoothingSamples - (midiReleasedSamples - preprocessingTotalSamples), numSamples));
        }
        else
        {
            getBufferPitcher(state)->processSamples(currentSample - effectiveStart, numSamples);
        }
    }
    
    auto note = getCurrentlyPlayingNote();
    // these temp variables are so that each channel is treated the same without modifying the overall context (I know it's ugly)
    VoiceState tempState = STOPPED;
    int tempCurrentSample = 0;
    int tempNoteDuration = 0;
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
        tempNoteDuration = noteDuration;
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
            tempNoteDuration++;
            if (tempState == STOPPED)
            {
                break;
            }
            // handle midi release
            if (midiReleased && tempMidiReleasedSamples == juce::jmin(preprocessingTotalSamples, tempNoteDuration)) // this effectively delays releases by the amount of samples used in preprocessing
            {
                if (isLooping && loopingHasEnd)
                {
                    if (doCrossfadeSmoothing)
                    {
                        tempIsSmoothingRelease = true;
                        // move this elsewhere binyamin!
                        releaseBuffer->processSamples(releaseBuffer->startDelay, crossfadeSmoothingSamples);
                    }
                    else
                    {
                        tempState = RELEASING;
                        if (playbackMode == PluginParameters::BASIC)
                        {
                            tempCurrentSample = tempEffectiveStart + (sampleEnd + 1 - tempEffectiveStart) / (noteFreq / sampleSound->baseFreq) * sampleRateConversion;
                        }
                        else
                        {
                            tempEffectiveStart = sampleEnd + 1;
                            tempCurrentSample = tempEffectiveStart + releaseBuffer->startDelay;
                        }
                    }
                }
                else
                {
                    if (doStartStopSmoothing)
                    {
                        tempIsSmoothingEnd = true;
                    }
                    else
                    {
                        tempState = STOPPED;
                        clearCurrentNote();
                        return;
                    }
                }
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
                // handle next sample being outside of current buffer: 1. expected length has been reached
                // 2. bufferPitcher stopped at less than expected (this case is theoretically possible and can't be handled easily, luckily I seem to be fine ignoring it)
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
                        if (doCrossfadeSmoothing)
                        {
                            tempCurrentSample += crossfadeSmoothingSamples;
                        }
                    }
                    else if (tempState == RELEASING || !isLooping) // end playback after current sample
                    {
                        tempState = STOPPED;
                    }
                }
                // handle loop smoothing start
                else if (doCrossfadeSmoothing && (tempState == PLAYING || tempState == LOOPING) && 
                    tempCurrentSample - tempEffectiveStart - bufferPitcher->startDelay == bufferPitcher->expectedOutputSamples - crossfadeSmoothingSamples)
                {
                    tempSmoothingLoopSample = 0;
                    tempIsSmoothingLoop = true;
                }
                // handle end smoothing start
                else if (doStartStopSmoothing && tempState == RELEASING && tempCurrentSample - tempEffectiveStart - bufferPitcher->startDelay == bufferPitcher->expectedOutputSamples - startStopSmoothingSamples)
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
            if (tempIsSmoothingStart)
            {
                sample *= float(tempSmoothingStartSample) / startStopSmoothingSamples;
                tempSmoothingStartSample++;
                if (tempSmoothingStartSample >= startStopSmoothingSamples)
                {
                    tempIsSmoothingStart = false;
                }
            }
            if (tempIsSmoothingLoop)
            {
                float loopStartSample = playbackMode == PluginParameters::BASIC ? 0 : 
                    startBuffer->processedBuffer.getSample(effectiveCh, tempSmoothingLoopSample + startBuffer->startDelay + (sampleStart - tempEffectiveStart) * sampleRateConversion);
                sample = sample * float(crossfadeSmoothingSamples - tempSmoothingLoopSample) / crossfadeSmoothingSamples + 
                            loopStartSample * float(tempSmoothingLoopSample) / crossfadeSmoothingSamples;
                tempSmoothingLoopSample++;
                if (tempSmoothingLoopSample >= crossfadeSmoothingSamples)
                {
                    tempIsSmoothingLoop = false;
                }
            }
            if (tempIsSmoothingRelease)
            {
                float releaseSample = playbackMode == PluginParameters::BASIC ? 0 :
                    releaseBuffer->processedBuffer.getSample(effectiveCh, tempSmoothingReleaseSample + releaseBuffer->startDelay);
                sample = sample * float(crossfadeSmoothingSamples - tempSmoothingReleaseSample) / crossfadeSmoothingSamples +
                    releaseSample * float(tempSmoothingReleaseSample) / crossfadeSmoothingSamples;
                tempSmoothingReleaseSample++;
                if (tempSmoothingReleaseSample >= crossfadeSmoothingSamples)
                {
                    tempIsSmoothingRelease = false;
                    tempIsSmoothingLoop = false; // this is necessary for certain timings
                    tempState = RELEASING;
                    if (playbackMode == PluginParameters::BASIC)
                    {
                        tempCurrentSample = crossfadeSmoothingSamples + tempEffectiveStart + (sampleEnd + 1 - effectiveStart) / (noteFreq / sampleSound->baseFreq) * sampleRateConversion;
                    }
                    else
                    {
                        tempEffectiveStart = sampleEnd + 1;
                        tempCurrentSample = crossfadeSmoothingSamples + tempEffectiveStart + releaseBuffer->startDelay;
                        if (effectiveCh == 0)
                        {
                            releaseBuffer->processSamples(tempCurrentSample - tempEffectiveStart, startSample + numSamples - i);
                        }
                    }
                }
            }
            if (tempIsSmoothingEnd)
            {
                sample *= float(startStopSmoothingSamples - tempSmoothingEndSample) / startStopSmoothingSamples;
                tempSmoothingEndSample++;
                if (tempSmoothingEndSample >= startStopSmoothingSamples)
                {
                    tempState = STOPPED;
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
    noteDuration = tempNoteDuration;
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

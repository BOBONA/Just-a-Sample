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
            return vc.effectiveStart;
        }
        return vc.effectiveStart + (vc.currentSample - getBufferPitcher(vc.state)->startDelay - vc.effectiveStart) / sampleRateConversion;
    }
    else
    {
        return vc.effectiveStart + (vc.currentSample - vc.effectiveStart) * (noteFreq / sampleSound->baseFreq) / sampleRateConversion;
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
        vc = VoiceContext();
        vc.effectiveStart = (isLooping && loopingHasStart) ? loopStart : sampleStart;
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
            startBuffer->setSampleStart(vc.effectiveStart);
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
            vc.currentSample = startBuffer->startDelay + vc.effectiveStart;
            if (PluginParameters::PREPROCESS_STEP)
            {
                preprocessingSample = 0;
                preprocessingTotalSamples = startBuffer->startPad() + ((isLooping && loopingHasEnd) ? releaseBuffer->startPad() : 0);
                vc.state = PREPROCESSING;
            }
            else
            {
                preprocessingTotalSamples = 0;
                vc.state = isLooping && !loopingHasStart ? LOOPING : PLAYING;
            }
        }
        else
        {
            vc.currentSample = vc.effectiveStart;
            vc.state = isLooping && !loopingHasStart ? LOOPING : PLAYING;
        }
        midiReleased = false;
        vc.isSmoothingStart = true;
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
        vc.state = STOPPED;
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
    if (vc.state == STOPPED)
        return;
    // preprocessing step to reduce audio glitches (at the expense of latency)
    if (vc.state == PREPROCESSING)
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
                vc.noteDuration += startProcessSamples;
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
                    vc.noteDuration += startProcessSamples;
                }
            }
        }
        if (preprocessingSample >= preprocessingTotalSamples)
        {
            if (vc.state == PREPROCESSING)
            {
                vc.state = isLooping && !loopingHasStart ? LOOPING : PLAYING;
            }
            else
            {
                vc.state = RELEASING;
                vc.effectiveStart = sampleEnd + 1;
                vc.currentSample = vc.effectiveStart + releaseBuffer->startDelay;
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
        if (midiReleased && releaseBuffer && vc.state != RELEASING) // must handle the case where we might start using the release buffer midway through
        {
            startBuffer->processSamples(vc.currentSample - vc.effectiveStart, juce::jmin(crossfadeSmoothingSamples - (vc.midiReleasedSamples - preprocessingTotalSamples), numSamples));
        }
        else
        {
            getBufferPitcher(vc.state)->processSamples(vc.currentSample - vc.effectiveStart, numSamples);
        }
    }
    
    auto note = getCurrentlyPlayingNote();
    VoiceContext con;
    for (auto ch = 0; ch < outputBuffer.getNumChannels(); ch++)
    {   
        con = vc; // so that work on one channel doesn't interfere with another channel
        auto effectiveCh = ch % sampleSound->sample.getNumChannels();
        for (auto i = startSample; i < startSample + numSamples; i++)
        {
            con.noteDuration++;
            if (con.state == STOPPED)
            {
                break;
            }
            // handle midi release
            if (midiReleased && con.midiReleasedSamples == juce::jmin(preprocessingTotalSamples, con.noteDuration)) // this effectively delays releases by the amount of samples used in preprocessing
            {
                if (isLooping && loopingHasEnd)
                {
                    if (doCrossfadeSmoothing)
                    {
                        con.isSmoothingRelease = true;
                        // move this elsewhere binyamin!
                        releaseBuffer->processSamples(releaseBuffer->startDelay, crossfadeSmoothingSamples);
                    }
                    else
                    {
                        con.state = RELEASING;
                        if (playbackMode == PluginParameters::BASIC)
                        {
                            con.currentSample = con.effectiveStart + (sampleEnd + 1 - con.effectiveStart) / (noteFreq / sampleSound->baseFreq) * sampleRateConversion;
                        }
                        else
                        {
                            con.effectiveStart = sampleEnd + 1;
                            con.currentSample = con.effectiveStart + releaseBuffer->startDelay;
                        }
                    }
                }
                else
                {
                    if (doStartStopSmoothing)
                    {
                        con.isSmoothingEnd = true;
                    }
                    else
                    {
                        con.state = STOPPED;
                        clearCurrentNote();
                        return;
                    }
                }
            }
            if (midiReleased && con.state != RELEASING)
            {
                con.midiReleasedSamples++;
            }
            // fetch the appropriate sample depending on the mode
            float sample{ 0 };
            if (playbackMode == PluginParameters::ADVANCED)
            {
                sample = getBufferPitcher(con.state)->processedBuffer.getSample(effectiveCh, con.currentSample - con.effectiveStart);
                // handle next sample being outside of current buffer: 1. expected length has been reached
                // 2. bufferPitcher stopped at less than expected (this case is theoretically possible and can't be handled easily, luckily I seem to be fine ignoring it)
                auto& bufferPitcher = getBufferPitcher(con.state);
                if (con.currentSample - con.effectiveStart - bufferPitcher->startDelay + 2 >= bufferPitcher->expectedOutputSamples/* ||
                    (con.currentSample - con.effectiveStart >= bufferPitcher->totalPitchedSamples && i + 1 < startSample + numSamples)*/)
                {
                    if (con.state == PLAYING && isLooping)
                    {
                        con.state = LOOPING;
                    }
                    else if (con.state == LOOPING)
                    {
                        // the need for this type of logic makes me wonder about my decisions
                        con.currentSample = bufferPitcher->startDelay + con.effectiveStart + (sampleStart - con.effectiveStart) * sampleRateConversion - 1;
                        if (doCrossfadeSmoothing)
                        {
                            con.currentSample += crossfadeSmoothingSamples;
                        }
                    }
                    else if (con.state == RELEASING || !isLooping) // end playback after current sample
                    {
                        con.state = STOPPED;
                    }
                }
                // handle loop smoothing start
                else if (doCrossfadeSmoothing && (con.state == PLAYING || con.state == LOOPING) && 
                    con.currentSample - con.effectiveStart - bufferPitcher->startDelay == bufferPitcher->expectedOutputSamples - crossfadeSmoothingSamples)
                {
                    con.smoothingLoopSample = 0;
                    con.isSmoothingLoop = true;
                }
                // handle end smoothing start
                else if (doStartStopSmoothing && con.state == RELEASING && con.currentSample - con.effectiveStart - bufferPitcher->startDelay == bufferPitcher->expectedOutputSamples - startStopSmoothingSamples)
                {
                    con.isSmoothingEnd = true;
                }
                con.currentSample++;
            }
            else if (playbackMode == PluginParameters::BASIC)
            {
                /*auto loc = con.effectiveStart + (con.currentSample - con.effectiveStart) * (noteFreq / sampleSound->baseFreq) / sampleRateConversion;
                if ((loc > effectiveEnd || loc >= sampleSound->sample.getNumSamples()) && !(isLooping && !loopingHasEnd))
                {
                    con.state = STOPPING;
                    tempSmoothingSample = 0;
                    smoothingInitial.set(ch, previousSample[ch]);
                    break;
                }
                // handle loop states
                if (isLooping)
                {
                    if (con.state == PLAYING && loc >= sampleStart)
                    {
                        con.state = LOOPING;
                    }
                    else if (con.state == LOOPING && loc > sampleEnd)
                    {
                        tempIsSmoothing = true;
                        tempSmoothingSample = 0;
                        smoothingInitial.set(ch, previousSample[ch]);
                        con.currentSample = effectiveStart + (sampleStart - effectiveStart) / (noteFreq / sampleSound->baseFreq) * sampleRateConversion;
                        loc = effectiveStart + (con.currentSample - effectiveStart) * (noteFreq / sampleSound->baseFreq) / sampleRateConversion;
                    }
                }
                sample = sampleSound->sample.getSample(effectiveCh, loc);
                con.currentSample++;*/
            }
            if (con.state == LOOPING && sampleEnd - sampleStart + 1 < 3) // stop tiny loops from outputting samples
            {
                sample = 0;
            }
            // handle smoothing
            if (con.isSmoothingStart)
            {
                sample *= float(con.smoothingStartSample) / startStopSmoothingSamples;
                con.smoothingStartSample++;
                if (con.smoothingStartSample >= startStopSmoothingSamples)
                {
                    con.isSmoothingStart = false;
                }
            }
            if (con.isSmoothingLoop)
            {
                float loopStartSample = playbackMode == PluginParameters::BASIC ? 0 : 
                    startBuffer->processedBuffer.getSample(effectiveCh, con.smoothingLoopSample + startBuffer->startDelay + (sampleStart - con.effectiveStart) * sampleRateConversion);
                sample = sample * float(crossfadeSmoothingSamples - con.smoothingLoopSample) / crossfadeSmoothingSamples + 
                            loopStartSample * float(con.smoothingLoopSample) / crossfadeSmoothingSamples;
                con.smoothingLoopSample++;
                if (con.smoothingLoopSample >= crossfadeSmoothingSamples)
                {
                    con.isSmoothingLoop = false;
                }
            }
            if (con.isSmoothingRelease)
            {
                float releaseSample = playbackMode == PluginParameters::BASIC ? 0 :
                    releaseBuffer->processedBuffer.getSample(effectiveCh, con.smoothingReleaseSample + releaseBuffer->startDelay);
                sample = sample * float(crossfadeSmoothingSamples - con.smoothingReleaseSample) / crossfadeSmoothingSamples +
                    releaseSample * float(con.smoothingReleaseSample) / crossfadeSmoothingSamples;
                con.smoothingReleaseSample++;
                if (con.smoothingReleaseSample >= crossfadeSmoothingSamples)
                {
                    con.isSmoothingRelease = false;
                    con.isSmoothingLoop = false; // this is necessary for certain timings
                    con.state = RELEASING;
                    if (playbackMode == PluginParameters::BASIC)
                    {
                        con.currentSample = crossfadeSmoothingSamples + con.effectiveStart + (sampleEnd + 1 - con.effectiveStart) / (noteFreq / sampleSound->baseFreq) * sampleRateConversion;
                    }
                    else
                    {
                        con.effectiveStart = sampleEnd + 1;
                        con.currentSample = crossfadeSmoothingSamples + con.effectiveStart + releaseBuffer->startDelay;
                        if (effectiveCh == 0)
                        {
                            releaseBuffer->processSamples(con.currentSample - con.effectiveStart, startSample + numSamples - i);
                        }
                    }
                }
            }
            if (con.isSmoothingEnd)
            {
                sample *= float(startStopSmoothingSamples - con.smoothingEndSample) / startStopSmoothingSamples;
                con.smoothingEndSample++;
                if (con.smoothingEndSample >= startStopSmoothingSamples)
                {
                    con.state = STOPPED;
                }
            }
            // gain scale
            sample = sample * Decibels::decibelsToGain(float(sampleSound->gain.getValue()));
            outputBuffer.addSample(ch, i, sample);
            previousSample.set(ch, sample);
        }
    }
    vc = con;
    if (vc.state == STOPPED)
    {
        clearCurrentNote();
    }
}

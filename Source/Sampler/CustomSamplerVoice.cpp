/*
  ==============================================================================

    CustomSamplerVoice.cpp
    Created: 5 Sep 2023 3:35:03pm
    Author:  binya

  ==============================================================================
*/

#include "CustomSamplerVoice.h"

CustomSamplerVoice::CustomSamplerVoice(int numChannels, int expectedBlockSize) : numChannels(numChannels), expectedBlockSize(expectedBlockSize)
{
    
}

CustomSamplerVoice::~CustomSamplerVoice()
{
}

bool CustomSamplerVoice::canPlaySound(SynthesiserSound* sound)
{
    return bool(dynamic_cast<CustomSamplerSound*>(sound));
}

void CustomSamplerVoice::startNote(int midiNoteNumber, float velocity, SynthesiserSound* sound, int currentPitchWheelPosition)
{   
    noteVelocity = velocity;
    auto check = dynamic_cast<CustomSamplerSound*>(sound);
    auto newSound = sampleSound != check;
    sampleSound = check;
    if (sampleSound)
    {
        tempOutputBuffer.setSize(sampleSound->sample.getNumChannels(), 0);
        previousSample.resize(sampleSound->sample.getNumChannels());
        sampleRateConversion = float(getSampleRate() / sampleSound->sampleRate);
        tuningRatio = PluginParameters::A4_HZ / pow(2.f, (float(sampleSound->semitoneTuning.getValue()) + float(sampleSound->centTuning.getValue()) / 100.f) / 12.f);
        speedFactor = sampleSound->speedFactor.getValue();
        noteFreq = float(MidiMessage::getMidiNoteInHertz(midiNoteNumber, PluginParameters::A4_HZ)) * pow(2.f, jmap<float>(float(currentPitchWheelPosition), 0.f, 16383.f, -1.f, 1.f) / 12.f);
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
        if ((doStartStopSmoothing = sampleSound->doStartStopSmoothing) == true) // == true is to shut up the compiler warnings
        {
            startStopSmoothingSamples = sampleSound->startStopSmoothingSamples;
        }
        if ((doCrossfadeSmoothing = sampleSound->doCrossfadeSmoothing) == true)
        {
            crossfadeSmoothingSamples = juce::jmin(sampleSound->crossfadeSmoothingSamples, (sampleEnd - sampleStart + 1) / 4);
        }
        playbackMode = sampleSound->getPlaybackMode();

        if (playbackMode == PluginParameters::ADVANCED)
        {
            // initialize startBuffer
            Stretcher::Options options = BufferPitcher::DEFAULT_OPTIONS;
            if (sampleSound->formantPreserved.getValue())
                options |= Stretcher::OptionFormantPreserved;
            if (newSound || !startBuffer)
            {
                startBuffer = std::make_unique<BufferPitcher>(sampleSound->sample, int(getSampleRate()), false, options);
            }
            startBuffer->setPitchScale(noteFreq / tuningRatio / sampleRateConversion);
            startBuffer->setTimeRatio(sampleRateConversion / speedFactor);
            startBuffer->setSampleStart(vc.effectiveStart);
            startBuffer->setSampleEnd(sampleEnd);
            startBuffer->resetProcessing(!PluginParameters::PREPROCESS_STEP);

            // initialize releaseBuffer
            if (isLooping && loopingHasEnd)
            {
                if (newSound || !releaseBuffer)
                {
                    releaseBuffer = std::make_unique<BufferPitcher>(sampleSound->sample, int(getSampleRate()), false, options);
                }
                releaseBuffer->setPitchScale(noteFreq / tuningRatio / sampleRateConversion);
                releaseBuffer->setTimeRatio(sampleRateConversion / speedFactor);
                releaseBuffer->setSampleStart(sampleEnd + 1);
                releaseBuffer->setSampleEnd(loopEnd);
                releaseBuffer->resetProcessing(!PluginParameters::PREPROCESS_STEP);
            }
            vc.currentSample = startBuffer->startDelay + vc.effectiveStart;
            if (PluginParameters::PREPROCESS_STEP)
            {
                preprocessingSample = 0;
                preprocessingTotalSamples = startBuffer->startPad();
                if (isLooping && loopingHasEnd)
                {
                    preprocessingTotalSamples += releaseBuffer->startPad();
                    if (PluginParameters::PREPROCESS_RELEASE_BUFFER) 
                        preprocessingTotalSamples += crossfadeSmoothingSamples;
                }
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
            preprocessingTotalSamples = 0;
        }
        midiReleased = false;
        vc.isSmoothingStart = true;

        effects.clear();
        updateParams = 0;
    }
}

void CustomSamplerVoice::stopNote(float, bool allowTailOff)
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

void CustomSamplerVoice::pitchWheelMoved(int)
{
}

void CustomSamplerVoice::controllerMoved(int, int)
{
}

void CustomSamplerVoice::renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (vc.state == STOPPED && (!doFxTailOff || !getCurrentlyPlayingSound()))
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
                    vc.noteDuration += releaseProcessSamples;
                }
            }
            if (PluginParameters::PREPROCESS_RELEASE_BUFFER)
            {
                int releaseBufferSamples = juce::jmax(0, juce::jmin(numSamples - startProcessSamples - releaseProcessSamples, crossfadeSmoothingSamples - (preprocessingSample - (startBuffer->startPad() + releaseBuffer->startPad()))));
                if (releaseBufferSamples > 0)
                {
                    releaseBuffer->processSamples(releaseBuffer->startDelay + preprocessingSample - (startBuffer->startPad() + releaseBuffer->startPad()), releaseBufferSamples);
                    preprocessingSample += releaseBufferSamples;
                    startSample += releaseBufferSamples;
                    numSamples -= releaseBufferSamples;
                    if (!midiReleased)
                    {
                        vc.noteDuration += releaseBufferSamples;
                    }
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

    if (numSamples == 0)
        return;

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

    // retrieve channels from sample
    if (tempOutputBuffer.getNumSamples() < numSamples)
    {
        tempOutputBuffer.setSize(tempOutputBuffer.getNumChannels(), numSamples);
    }
    tempOutputBuffer.clear();
    VoiceContext con;
    for (auto ch = 0; ch < sampleSound->sample.getNumChannels(); ch++)
    {   
        con = vc; // so that work on one channel doesn't interfere with another channel
        for (auto i = startSample; i < startSample + numSamples; i++)
        {
            con.noteDuration++;
            if (con.state == STOPPED)
            {
                con.samplesSinceStopped++;
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
                        if (playbackMode == PluginParameters::ADVANCED && !(PluginParameters::PREPROCESS_STEP && PluginParameters::PREPROCESS_RELEASE_BUFFER))
                        {
                            releaseBuffer->processSamples(releaseBuffer->startDelay, crossfadeSmoothingSamples);
                        }
                    }
                    else
                    {
                        con.state = RELEASING;
                        if (playbackMode == PluginParameters::BASIC)
                        {
                            con.currentSample = con.effectiveStart + int((sampleEnd + 1 - con.effectiveStart) / (noteFreq / tuningRatio) * sampleRateConversion);
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
                        vc.state = STOPPED;
                        break;
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
                sample = getBufferPitcher(con.state)->processedBuffer.getSample(ch, con.currentSample - con.effectiveStart);
                
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
                        con.currentSample = bufferPitcher->startDelay + con.effectiveStart + int((sampleStart - con.effectiveStart) * (sampleRateConversion / speedFactor)) - 1;
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
                else if (doCrossfadeSmoothing && isLooping && (con.state == PLAYING || con.state == LOOPING) && 
                    con.currentSample - con.effectiveStart - bufferPitcher->startDelay == bufferPitcher->expectedOutputSamples - crossfadeSmoothingSamples)
                {
                    con.smoothingLoopSample = 0;
                    con.isSmoothingLoop = true;
                }
                // handle end smoothing start
                else if (doStartStopSmoothing && (con.state == RELEASING || (con.state == PLAYING && !isLooping)) && con.currentSample - con.effectiveStart - bufferPitcher->startDelay == bufferPitcher->expectedOutputSamples - startStopSmoothingSamples)
                {
                    con.isSmoothingEnd = true;
                }
                con.currentSample++;
            }
            else if (playbackMode == PluginParameters::BASIC)
            {
                float loc = getBasicLoc(con.currentSample, con.effectiveStart);

                // handle loop states
                if (isLooping)
                {
                    if (con.state == PLAYING && loc >= sampleStart)
                    {
                        con.state = LOOPING;
                    }
                    else if (con.state == LOOPING && loc > sampleEnd)
                    {
                        con.currentSample = con.effectiveStart + int((sampleStart - con.effectiveStart) / (noteFreq / tuningRatio) * sampleRateConversion);
                        if (doCrossfadeSmoothing)
                        {
                            con.currentSample += crossfadeSmoothingSamples;
                        }
                        loc = getBasicLoc(con.currentSample, con.effectiveStart);
                    }
                }

                // handle general out of bounds (might not be necessary)
                if ((con.state == RELEASING || !isLooping) && (loc > effectiveEnd || loc >= sampleSound->sample.getNumSamples()))
                {
                    con.state = STOPPED;
                    break; // since this current sample is out of bounds
                }
                else if (doCrossfadeSmoothing && isLooping && !con.isSmoothingLoop && (con.state == PLAYING || con.state == LOOPING) && 
                    getBasicLoc(con.currentSample + crossfadeSmoothingSamples, con.effectiveStart) > sampleEnd)
                {
                    con.smoothingLoopSample = 0;
                    con.isSmoothingLoop = true;
                }
                else if (doStartStopSmoothing && !con.isSmoothingEnd && (con.state == RELEASING || (con.state == PLAYING && !isLooping)) &&
                    getBasicLoc(con.currentSample + startStopSmoothingSamples, con.effectiveStart) > effectiveEnd)
                {
                    con.isSmoothingEnd = true;
                }
                sample = getBasicSample(ch, loc);
                con.currentSample++;
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
                float loopStartSample = playbackMode == PluginParameters::BASIC ? 
                    getBasicSample(ch, getBasicLoc(con.smoothingLoopSample + con.effectiveStart + int((sampleStart - con.effectiveStart) / (noteFreq / tuningRatio) * sampleRateConversion), con.effectiveStart)) :
                    startBuffer->processedBuffer.getSample(ch, con.smoothingLoopSample + startBuffer->startDelay + int((sampleStart - con.effectiveStart) * (sampleRateConversion / speedFactor)));
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
                float releaseSample = playbackMode == PluginParameters::BASIC ?
                    getBasicSample(ch, getBasicLoc(con.smoothingReleaseSample + con.effectiveStart + int((sampleEnd + 1 - con.effectiveStart) / (noteFreq / tuningRatio) * sampleRateConversion), con.effectiveStart)) :
                    releaseBuffer->processedBuffer.getSample(ch, con.smoothingReleaseSample + releaseBuffer->startDelay);
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
                        con.currentSample = crossfadeSmoothingSamples + con.effectiveStart + int((sampleEnd + 1 - con.effectiveStart) / (noteFreq / tuningRatio) * sampleRateConversion);
                    }
                    else
                    {
                        con.effectiveStart = sampleEnd + 1;
                        con.currentSample = crossfadeSmoothingSamples + con.effectiveStart + releaseBuffer->startDelay;
                        if (ch == 0)
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
            tempOutputBuffer.setSample(ch, i - startSample, sample);
            previousSample.set(ch, sample);
        }
    }
    vc = con;

    // check for updated FX order
    if (updateParams == 0)
    {
        initializeFx();
    }

    // apply FX
    int reverbSampleDelay = int(1000.f + float(sampleSound->reverbPredelay.getValue()) * float(getSampleRate()) / 1000.f); // the 1000.f is approximate
    bool someFXEnabled{ true };
    for (auto& effect : effects)
    {
        // check for updated enablement
        bool enablement = effect.enablementSource.getValue();
        if (!effect.enabled && enablement)
        {
            effect.fx->initialize(sampleSound->sample.getNumChannels(), int(getSampleRate()));
        }
        effect.enabled = enablement;
        someFXEnabled = someFXEnabled || effect.enabled;
        if (effect.enabled && !effect.locallyDisabled)
        {
            // update params every UPDATE_PARAMS_LENGTH calls to process
            if (updateParams == 0)
            {
                effect.fx->updateParams(*sampleSound);
            }
            effect.fx->process(tempOutputBuffer, numSamples);

            // check if effect should be locally disabled
            if (con.state == STOPPED && numSamples > 10 && (effect.fxType != PluginParameters::REVERB || con.samplesSinceStopped > reverbSampleDelay)) 
            {
                bool disable{ true };
                for (int ch = 0; ch < tempOutputBuffer.getNumChannels(); ch++)
                {
                    float level = tempOutputBuffer.getRMSLevel(ch, 0, numSamples);
                    if (level > 0)
                    {
                        disable = false;
                        break;
                    }
                }
                if (disable)
                    effect.locallyDisabled = true;
            }
        }
    }
    doFxTailOff = PluginParameters::FX_TAIL_OFF && someFXEnabled;
    if (updateParams == 0)
    {
        updateParams = UPDATE_PARAMS_LENGTH;
    }
    updateParams--;

    // check RMS level to see if voice should be ended
    if (con.state == STOPPED && numSamples > 10 && (!sampleSound->reverbEnabled.getValue() || con.samplesSinceStopped > reverbSampleDelay))
    {
        bool end{ true };
        for (int ch = 0; ch < tempOutputBuffer.getNumChannels(); ch++)
        {
            float level = tempOutputBuffer.getRMSLevel(ch, 0, numSamples);
            if (level >= PluginParameters::FX_TAIL_OFF_MAX)
            {
                end = false;
                break;
            }
        }
        if (end)
        {
            clearCurrentNote();
            return;
        }
    }

    // copy the tempOutputBuffer channels into the actual output channels
    // this is certainly an over-complicated way of doing this but I think it's nice to have a completely variable number of channels
    if (sampleSound->monoOutput.getValue())
    {
        AudioBuffer<float> monoOutput{ 1, numSamples };
        for (int ch = 0; ch < tempOutputBuffer.getNumChannels(); ch++)
        {
            FloatVectorOperations::copyWithMultiply(monoOutput.getWritePointer(0), tempOutputBuffer.getReadPointer(ch), 1.f / tempOutputBuffer.getNumChannels(), numSamples);
        }
        for (int ch = 0; ch < outputBuffer.getNumChannels(); ch++)
        {
            outputBuffer.addFrom(ch, startSample, monoOutput.getReadPointer(0), numSamples);
        }
    }
    else if (outputBuffer.getNumChannels() < tempOutputBuffer.getNumChannels())
    {
        float ratio = float(tempOutputBuffer.getNumChannels()) / outputBuffer.getNumChannels();
        int outputChannel = 0;
        int processed = 0;
        for (float i = 1; i <= tempOutputBuffer.getNumChannels() + 0.01f; i += ratio)
        {
            int nextChannel = int(floorf(i + 0.01f)); // just in case floating errors
            for (int ch = processed; ch < nextChannel; ch++)
            {
                outputBuffer.addFrom(outputChannel, startSample, tempOutputBuffer.getReadPointer(ch), numSamples, 1.f / (nextChannel - processed));
            }
            processed = nextChannel;
            outputChannel++;
        }
    }
    else if (outputBuffer.getNumChannels() >= tempOutputBuffer.getNumChannels())
    {
        bool wrapsAround = outputBuffer.getNumChannels() % tempOutputBuffer.getNumChannels() != 0;
        int lastChannelMod = outputBuffer.getNumChannels() % tempOutputBuffer.getNumChannels();
        float levelFix = float(outputBuffer.getNumChannels() / tempOutputBuffer.getNumChannels()) /
            float(1 + int(outputBuffer.getNumChannels() / tempOutputBuffer.getNumChannels())); // to keep the relative levels the same
        for (int ch = 0; ch < outputBuffer.getNumChannels(); ch++)
        {
            float multiplier = 1.f;
            if (wrapsAround && ch % tempOutputBuffer.getNumChannels() < lastChannelMod)
                multiplier = levelFix;
            outputBuffer.addFrom(
                ch, startSample,
                tempOutputBuffer.getReadPointer(ch % tempOutputBuffer.getNumChannels()),
                numSamples,
                multiplier
            );
        }
    }

    if (vc.state == STOPPED && !doFxTailOff)
    {
        clearCurrentNote();
    }
}

int CustomSamplerVoice::getEffectiveLocation()
{
    if (playbackMode == PluginParameters::ADVANCED)
    {
        if (!startBuffer)
        {
            return vc.effectiveStart;
        }
        return vc.effectiveStart + int((vc.currentSample - getBufferPitcher(vc.state)->startDelay - vc.effectiveStart) / (sampleRateConversion / speedFactor));
    }
    else
    {
        return int(getBasicLoc(vc.currentSample, vc.effectiveStart));
    }
}

float CustomSamplerVoice::getBasicLoc(int currentSample, int effectiveStart) const
{
    return effectiveStart + (currentSample - effectiveStart) * (noteFreq / tuningRatio) / sampleRateConversion;
}

float CustomSamplerVoice::getBasicSample(int channel, float sampleIndex)
{
    if (sampleIndex < 0 || sampleIndex >= sampleSound->sample.getNumSamples())
    {
        return 0.f;
    }
    else
    {
        if (sampleSound->skipAntialiasing.getValue())
        {
            return sampleSound->sample.getSample(channel, int(sampleIndex));
        }
        else
        {
            return lanczosInterpolate(channel, sampleIndex);
        }
    }
}

void CustomSamplerVoice::initializeFx()
{
    auto fxOrder = sampleSound->getFxOrder();
    bool changed = false;
    for (int i = 0; i < fxOrder.size(); i++)
    {
        if (effects.size() <= i || effects[i].fxType != fxOrder[i])
        {
            changed = true;
            break;
        }
    }
    if (changed)
    {
        effects.clear();
        for (int i = 0; i < fxOrder.size(); i++)
        {
            switch (fxOrder[i])
            {
            case PluginParameters::DISTORTION:
                effects.emplace_back(PluginParameters::DISTORTION, std::make_unique<Distortion>(), sampleSound->distortionEnabled);
                break;
            case PluginParameters::REVERB:
                effects.emplace_back(PluginParameters::REVERB, std::make_unique<TriReverb>(), sampleSound->reverbEnabled);
                break;
            case PluginParameters::CHORUS:
                effects.emplace_back(PluginParameters::CHORUS, std::make_unique<Chorus>(expectedBlockSize), sampleSound->chorusEnabled);
                break;
            case PluginParameters::EQ:
                effects.emplace_back(PluginParameters::EQ, std::make_unique<BandEQ>(), sampleSound->eqEnabled);
                break;
            }
        }
    }
}

/** Thank god for Wikipedia, I really don't know what this is doing */
float CustomSamplerVoice::lanczosInterpolate(int channel, float index)
{
    int floorIndex = int(floorf(index));

    float result = 0.f;
    for (int i = -LANCZOS_SIZE + 1; i <= LANCZOS_SIZE; i++)
    {
        int iPlus = i + floorIndex;
        float sample = (0 <= iPlus && iPlus < sampleSound->sample.getNumSamples()) ? sampleSound->sample.getSample(channel, iPlus) : 0.f;
        float window = lanczosWindow(index - floorIndex - i);
        result += sample * window;
    }
    return result ;
}

float CustomSamplerVoice::lanczosWindow(float x)
{
    return x == 0 ? 1.f :
        (LANCZOS_SIZE * sinf(MathConstants<float>::pi * x) * sinf(MathConstants<float>::pi * x / LANCZOS_SIZE) * INVERSE_SIN_SQUARED / (x * x));
}

const dsp::LookupTableTransform<float> CustomSamplerVoice::lanczosLookup{ [](float i) { return lanczosWindow(i); }, -LANCZOS_SIZE, LANCZOS_SIZE, 1000 };
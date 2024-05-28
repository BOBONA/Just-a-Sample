/*
  ==============================================================================

    CustomSamplerVoice.cpp
    Created: 5 Sep 2023 3:35:03pm
    Author:  binya

  ==============================================================================
*/

#include "CustomSamplerVoice.h"

#include "../Utilities/readerwriterqueue/BufferUtils.h"
#include "effects/BandEQ.h"
#include "effects/Chorus.h"
#include "effects/Distortion.h"
#include "Effects/TriReverb.h"

CustomSamplerVoice::CustomSamplerVoice(int expectedBlockSize) : expectedBlockSize(expectedBlockSize)
{
}

bool CustomSamplerVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return bool(dynamic_cast<CustomSamplerSound*>(sound));
}

void CustomSamplerVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition)
{   
    noteVelocity = velocity;
    auto newSound = dynamic_cast<CustomSamplerSound*>(sound);
    auto loadingNewSound = sampleSound != newSound;
    if (newSound)
    {
        sampleSound = newSound;
        sampleRateConversion = float(sampleSound->sampleRate / getSampleRate());
        float noteFreq = float(juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber, PluginParameters::A4_HZ)) * pow(2.f, juce::jmap<float>(float(currentPitchWheelPosition), 0.f, 16383.f, -1.f, 1.f) / 12.f);
        float tuningRatio = PluginParameters::A4_HZ / pow(2.f, (float(sampleSound->semitoneTuning.getValue()) + float(sampleSound->centTuning.getValue()) / 100.f) / 12.f);

        playbackMode = sampleSound->getPlaybackMode();
        speedFactor = sampleSound->speedFactor.getValue();
        speed = noteFreq / tuningRatio * sampleRateConversion;  // Overwritten in ADVANCED mode
        
        sampleStart = sampleSound->sampleStart.getValue();
        sampleEnd = sampleSound->sampleEnd.getValue();
        isLooping = sampleSound->isLooping.getValue();
        loopingHasStart = isLooping && sampleSound->loopingHasStart.getValue() && sampleSound->loopStart.getValue() < sampleSound->sampleStart.getValue();
        loopStart = sampleSound->loopStart.getValue();
        loopingHasEnd = isLooping && sampleSound->loopingHasEnd.getValue() && sampleSound->loopEnd.getValue() > sampleSound->sampleEnd.getValue();
        loopEnd = sampleSound->loopEnd.getValue();
        attackSmoothing = sampleSound->attackSmoothingSamples;
        releaseSmoothing = sampleSound->releaseSmoothingSamples;
        if (loopingHasEnd)  // Keep release smoothing within end portion
            releaseSmoothing = juce::jmin(releaseSmoothing, loopEnd - sampleEnd);
        crossfade = juce::jmin(sampleSound->crossfadeSamples, (sampleEnd - sampleStart + 1) / 4);  // Keep crossfade within 1/4 of the sample

        /** Smoothing might need to be made proportional sometimes */
        if (playbackMode == PluginParameters::BASIC)
        {
            
        }

        effectiveStart = loopingHasStart ? loopStart : sampleStart;
        effectiveEnd = loopingHasEnd ? loopEnd : sampleEnd;

        vc = VoiceContext();
        midiReleased = false;
        tempOutputBuffer.setSize(sampleSound->sample.getNumChannels(), expectedBlockSize * 4);

        if (playbackMode == PluginParameters::ADVANCED)
        {
            mainStretcher.initialize(sampleSound->sample, effectiveStart, sampleSound->sampleRate, int(getSampleRate()), 
                noteFreq / tuningRatio, speedFactor);
            speed = mainStretcher.getPositionSpeed();
        }

        effects.clear();
        updateFXParamsTimer = UPDATE_PARAMS_LENGTH;

        // Set the initial state
        vc.currentPosition = float(effectiveStart);
        vc.state = PLAYING;
        vc.isSmoothingAttack = attackSmoothing > 0;
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

//==============================================================================
void CustomSamplerVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (vc.state == STOPPED && (!doFxTailOff || !getCurrentlyPlayingSound()))
    {
        clearCurrentNote();
        return;
    }

    // This resize is not expected to happen
    if (tempOutputBuffer.getNumSamples() < numSamples)
        tempOutputBuffer.setSize(tempOutputBuffer.getNumChannels(), numSamples);
    tempOutputBuffer.clear();

    VoiceContext con;
    for (auto ch = 0; ch < sampleSound->sample.getNumChannels(); ch++)
    {
        // This struct is used to easily process channel by channel 
        con = vc;
        for (auto i = 0; i < numSamples; i++)
        {
            if (con.state == STOPPED)
            {
                con.samplesSinceStopped++;
                continue;
            }

            // Fetch the sample according to the playback mode
            float sample = playbackMode == PluginParameters::BASIC ? fetchSample(ch, con.currentPosition) : nextSample(ch, &mainStretcher);

            // Smoothing / envelopes
            if (con.isSmoothingAttack)
            {
                float attackPosition = con.currentPosition - effectiveStart;
                if (attackPosition >= attackSmoothing)
                    con.isSmoothingAttack = false;
                else
                    sample *= attackPosition / attackSmoothing;
            }

            if (con.isCrossfadingLoop)
            {
                float crossfadePosition = con.currentPosition - sampleStart;
                if (crossfadePosition >= crossfade)
                {
                    con.isCrossfadingLoop = false;
                }
                else
                {
                    // Power preserving crossfade (https://www.youtube.com/watch?v=-5cB3rec2T0)
                    float crossfadeIncrease = std::sqrtf(0.5f + 0.5f * std::cosf(juce::MathConstants<float>::pi * crossfadePosition / crossfade + juce::MathConstants<float>::pi));
                    float crossfadeDecrease = std::sqrtf(0.5f + 0.5f * std::cosf(juce::MathConstants<float>::pi * crossfadePosition / crossfade));
                    sample = sample * crossfadeIncrease + fetchSample(ch, con.currentPosition + sampleEnd - sampleStart - crossfade) * crossfadeDecrease;
                }
            }

            if (con.isCrossfadingEnd)
            {
                float crossfadePosition = con.currentPosition - sampleEnd;
                if (crossfadePosition >= crossfade)
                {
                    con.isCrossfadingEnd = false;
                }
                else
                {
                    float crossfadeIncrease = std::sqrtf(0.5f + 0.5f * std::cosf(juce::MathConstants<float>::pi * crossfadePosition / crossfade + juce::MathConstants<float>::pi));
                    float crossfadeDecrease = std::sqrtf(0.5f + 0.5f * std::cosf(juce::MathConstants<float>::pi * crossfadePosition / crossfade));
                    sample = sample * crossfadeIncrease + fetchSample(ch, con.crossfadeEndPosition) * crossfadeDecrease;
                    con.crossfadeEndPosition += speed;
                }
            }

            if (con.isReleasing)
            {
                sample *= (releaseSmoothing - con.positionMovedSinceRelease) / releaseSmoothing;
            }

            // Update the position 
            con.currentPosition += speed;
            if (con.isReleasing)
                con.positionMovedSinceRelease += speed;

            // Handle transitions
            if (con.state == PLAYING && isLooping && con.currentPosition >= sampleEnd - crossfade)  // Loop crossfade
            {
                con.currentPosition -= sampleEnd - sampleStart - crossfade;
                con.isCrossfadingLoop = true;
            }

            if (midiReleased && !con.isReleasing && con.state == PLAYING)  // Midi release, end crossfade
            {
                if (loopingHasEnd)
                {
                    con.crossfadeEndPosition = con.currentPosition;
                    con.currentPosition = sampleEnd + 1;
                    con.state = PLAYING_END;
                    con.isCrossfadingEnd = true;
                }
                else
                {
                    con.isReleasing = true;
                }
            }

            if (((con.state == PLAYING && !isLooping) || con.state == PLAYING_END) && !con.isReleasing &&
                con.currentPosition > effectiveEnd - releaseSmoothing)  // Release smoothing
            {
                con.isReleasing = true;
            }

            if (con.currentPosition > effectiveEnd || (con.isReleasing && con.positionMovedSinceRelease >= releaseSmoothing))  // End of playback reached
                con.state = STOPPED;

            // Scale with standard velocity curve
            sample *= juce::Decibels::decibelsToGain(40 * log10f(noteVelocity));
            sample *= juce::Decibels::decibelsToGain(float(sampleSound->gain.getValue()));

            tempOutputBuffer.setSample(ch, i, sample);
        }
    }
    vc = con;

    // Check for updated FX order
    if (updateFXParamsTimer == UPDATE_PARAMS_LENGTH)
        initializeFx();

    // Apply FX
    int reverbSampleDelay = int(1000.f + float(sampleSound->reverbPredelay.getValue()) * float(getSampleRate()) / 1000.f);  // the 1000.f is approximate
    reverbSampleDelay = 0;
    bool someFXEnabled{ false };
    for (auto& effect : effects)
    {
        // Check for updated enablement
        bool enablement = effect.enablementSource.getValue();
        if (!effect.enabled && enablement)
            effect.fx->initialize(sampleSound->sample.getNumChannels(), int(getSampleRate()));
        effect.enabled = enablement;
        someFXEnabled = someFXEnabled || effect.enabled;

        if (effect.enabled && !effect.locallyDisabled)
        {
            // Update params every UPDATE_PARAMS_LENGTH calls to process
            if (updateFXParamsTimer == UPDATE_PARAMS_LENGTH)
                effect.fx->updateParams(*sampleSound);
            effect.fx->process(tempOutputBuffer, numSamples);

            // Check if an effect should be locally disabled. Note that reverb can only be disabled after a certain delay
            if (con.state == STOPPED && numSamples > 10 && !(effect.fxType == PluginParameters::REVERB && con.samplesSinceStopped <= reverbSampleDelay)) 
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

    updateFXParamsTimer--;
    if (updateFXParamsTimer == 0)
        updateFXParamsTimer = UPDATE_PARAMS_LENGTH;
    doFxTailOff = PluginParameters::FX_TAIL_OFF && someFXEnabled && !effects.empty();

    // Check RMS level to see if a voice should be ended despite tailing off effects
    if (con.state == STOPPED && someFXEnabled && numSamples > 10 && con.samplesSinceStopped > reverbSampleDelay)
    {
        bool end{ true };  // Whether all channels are below the threshold
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

    mixToBuffer(tempOutputBuffer, outputBuffer, startSample, numSamples, bool(sampleSound->monoOutput.getValue()));
}

float CustomSamplerVoice::fetchSample(int channel, float position) const
{
    if (0 > position || position >= float(sampleSound->sample.getNumSamples()))
        return 0.f;

    if (sampleSound->skipAntialiasing.getValue())
        return sampleSound->sample.getSample(channel, int(position));
    else
        return lanczosInterpolate(channel, position);
}

float CustomSamplerVoice::nextSample(int channel, BungeeStretcher* stretcher) const
{
    return stretcher->nextSample(channel);
}

//==============================================================================
void CustomSamplerVoice::initializeFx()
{
    auto fxOrder = sampleSound->getFxOrder();
    bool changed = false;
    for (size_t i = 0; i < fxOrder.size(); i++)
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
        for (auto& fxType : fxOrder)
        {
            switch (fxType)
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

//==============================================================================
/** Thank god for Wikipedia, I don't really know why this works */
float CustomSamplerVoice::lanczosInterpolate(int channel, float position) const
{
    int floorIndex = int(floorf(position));

    float result = 0.f;
    for (int i = -LANCZOS_WINDOW_SIZE + 1; i <= LANCZOS_WINDOW_SIZE; i++)
    {
        int iPlus = i + floorIndex;
        float sample = (0 <= iPlus && iPlus < sampleSound->sample.getNumSamples()) ? sampleSound->sample.getSample(channel, iPlus) : 0.f;
        float window = lanczosWindow(position - floorIndex - i);
        result += sample * window;
    }
    return result;
}

float CustomSamplerVoice::lanczosWindow(float x)
{
    return x == 0.f ? 1.f : (LANCZOS_WINDOW_SIZE * std::sinf(juce::MathConstants<float>::pi * x) * std::sinf(juce::MathConstants<float>::pi * x / LANCZOS_WINDOW_SIZE) * INVERSE_SIN_SQUARED / (x * x));
}
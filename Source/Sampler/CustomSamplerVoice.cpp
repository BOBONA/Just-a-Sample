/*
  ==============================================================================

    CustomSamplerVoice.cpp
    Created: 5 Sep 2023 3:35:03pm
    Author:  binya

  ==============================================================================
*/

#include "CustomSamplerVoice.h"

#include "../Utilities/BufferUtils.h"
#include "Effects/BandEQ.h"
#include "Effects/Chorus.h"
#include "Effects/Distortion.h"
#include "Effects/TriReverb.h"

CustomSamplerVoice::CustomSamplerVoice(const SamplerParameters& samplerSound, int expectedBlockSize) :
    expectedBlockSize(expectedBlockSize), sampleSound(samplerSound),
    mainStretcher(samplerSound.sample, samplerSound.sampleRate, getSampleRate()),
    loopStretcher(samplerSound.sample, samplerSound.sampleRate, getSampleRate()),
    endStretcher(samplerSound.sample, samplerSound.sampleRate, getSampleRate())
{
    tempOutputBuffer.setSize(sampleSound.sample.getNumChannels(), expectedBlockSize * 4);

    mainStretcherBuffer.setSize(sampleSound.sample.getNumChannels(), expectedBlockSize * 2);
    loopStretcherBuffer.setSize(sampleSound.sample.getNumChannels() - 1, expectedBlockSize * 2);
    endStretcherBuffer.setSize(sampleSound.sample.getNumChannels() - 1, expectedBlockSize * 2);
}

void CustomSamplerVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition)
{   
    noteVelocity = velocity;
    if (sound)
    {
        sampleRateConversion = float(sampleSound.sampleRate / getSampleRate());

        playbackMode = sampleSound.getPlaybackMode();
       
        sampleStart = sampleSound.sampleStart;  // Note this implicitly loads the atomic value
        sampleEnd = sampleSound.sampleEnd;
        isLooping = sampleSound.isLooping->get();
        loopingHasStart = isLooping && sampleSound.loopingHasStart->get() && sampleSound.loopStart < sampleSound.sampleStart;
        loopStart = sampleSound.loopStart;
        loopingHasEnd = isLooping && sampleSound.loopingHasEnd->get() && sampleSound.loopEnd > sampleSound.sampleEnd;
        loopEnd = sampleSound.loopEnd;
        if (loopingHasEnd)  // Keep release smoothing within end portion
            releaseSmoothing = juce::jmin<float>(releaseSmoothing, loopEnd - sampleEnd);
        crossfade = juce::jmin<float>(sampleSound.crossfadeSamples, (sampleEnd - sampleStart + 1) / 4);  // Keep crossfade within 1/4 of the sample

        effectiveStart = loopingHasStart ? loopStart : sampleStart;
        effectiveEnd = loopingHasEnd ? loopEnd : sampleEnd;

        vc = VoiceContext();
        midiReleased = false;

        updateSpeedAndPitch(midiNoteNumber, currentPitchWheelPosition);

        if (playbackMode == PluginParameters::ADVANCED)
        {
            mainStretcher.initialize(sampleSound.sample, effectiveStart, sampleSound.sampleRate, int(getSampleRate()), 
                tuning, speedFactor);
        }

        // Once speed has been set, the attack and release smoothing values can be calculated from the parameter (which is in ms)
        attackSmoothing = juce::jmax<int>(PluginParameters::MIN_SMOOTHING_SAMPLES, sampleSound.attack->get() * speed * getSampleRate() / 1000);
        releaseSmoothing = juce::jmax<int>(PluginParameters::MIN_SMOOTHING_SAMPLES, sampleSound.release->get() * speed * getSampleRate() / 1000);

        effects.clear();
        updateFXParamsTimer = UPDATE_PARAMS_LENGTH;

        // Set the initial state
        vc.currentPosition = effectiveStart;
        vc.state = PLAYING;
        vc.isSmoothingAttack = attackSmoothing > 0;
    }
}

void CustomSamplerVoice::stopNote(float /*velocity*/, bool allowTailOff)
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
    updateSpeedAndPitch(getCurrentlyPlayingNote(), newPitchWheelValue);
}

void CustomSamplerVoice::updateSpeedAndPitch(int currentNote, int pitchWheelPosition)
{
    float noteFreq = float(juce::MidiMessage::getMidiNoteInHertz(currentNote, PluginParameters::A4_HZ)) * pow(2.f, juce::jmap<float>(float(pitchWheelPosition), 0.f, 16383.f, -1.f, 1.f) / 12.f);
    float tuningRatio = PluginParameters::A4_HZ / pow(2.f, (sampleSound.semitoneTuning->get() + sampleSound.centTuning->get() / 100.f) / 12.f);
    tuning = noteFreq / tuningRatio;

    speedFactor = sampleSound.speedFactor->get();

    if (playbackMode == PluginParameters::BASIC)
        speed = tuning * sampleRateConversion;
    else
        speed = speedFactor * sampleRateConversion;
}

//==============================================================================
void CustomSamplerVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (vc.state == STOPPED && (!doFxTailOff || !getCurrentlyPlayingSound()))
    {
        clearCurrentNote();
        return;
    }

    // These resizes are not expected to happen
    if (tempOutputBuffer.getNumSamples() < numSamples)
        tempOutputBuffer.setSize(tempOutputBuffer.getNumChannels(), numSamples);
    tempOutputBuffer.clear();

    if (playbackMode == PluginParameters::ADVANCED && loopStretcherBuffer.getNumSamples() < numSamples)
    {
        mainStretcherBuffer.setSize(mainStretcherBuffer.getNumChannels(), numSamples);
        loopStretcherBuffer.setSize(loopStretcherBuffer.getNumChannels(), numSamples);
        endStretcherBuffer.setSize(endStretcherBuffer.getNumChannels(), numSamples);
    }

    // Main processing loop
    VoiceContext con;
    for (auto ch = 0; ch < sampleSound.sample.getNumChannels(); ch++)
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
            float sample = playbackMode == PluginParameters::BASIC ? 
                fetchSample(ch, con.currentPosition) :
                nextSample(ch, &mainStretcher, mainStretcherBuffer, i);

            // Crossfading
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
                    float next = playbackMode == PluginParameters::BASIC ? 
                        fetchSample(ch, con.currentPosition + sampleEnd - sampleStart - crossfade) :
                        nextSample(ch, &loopStretcher, loopStretcherBuffer, i);
                    sample = sample * crossfadeIncrease + next * crossfadeDecrease;
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
                    float next = playbackMode == PluginParameters::BASIC ? 
                        fetchSample(ch, con.crossfadeEndPosition) :
                        nextSample(ch, &endStretcher, endStretcherBuffer, i);
                    sample = sample * crossfadeIncrease + next * crossfadeDecrease;
                    con.crossfadeEndPosition += speed;
                }
            }

            // Attack and release envelopes
            if (con.isSmoothingAttack)
            {
                if (con.positionMovedSinceStart >= attackSmoothing)
                    con.isSmoothingAttack = false;
                else
                    sample *= con.positionMovedSinceStart / attackSmoothing;
            }

            if (con.isReleasing)
            {
                sample *= (std::expf(2 * (releaseSmoothing - con.positionMovedSinceRelease) / releaseSmoothing) - 1) / (std::expf(2) - 1);
            }

            // Update the position 
            con.currentPosition += speed;
            if (con.isSmoothingAttack)
                con.positionMovedSinceStart += speed;
            if (con.isReleasing)
                con.positionMovedSinceRelease += speed;

            // Handle transitions
            if (con.state == PLAYING && isLooping && con.currentPosition >= sampleEnd - crossfade)  // Loop crossfade
            {
                con.currentPosition -= sampleEnd - sampleStart - crossfade;
                con.isCrossfadingLoop = true;

                if (playbackMode == PluginParameters::ADVANCED && ch == 0)
                {
                    loopStretcher = std::move(mainStretcher);
                    mainStretcher.initialize(sampleSound.sample, con.currentPosition, sampleSound.sampleRate, int(getSampleRate()), 
                        tuning, speedFactor);
                }
            }

            if (midiReleased && !con.isReleasing && con.state == PLAYING)  // Midi release, end crossfade
            {
                if (loopingHasEnd)
                {
                    con.crossfadeEndPosition = con.currentPosition;
                    con.currentPosition = sampleEnd + 1;
                    con.state = PLAYING_END;
                    con.isCrossfadingEnd = true;

                    if (playbackMode == PluginParameters::ADVANCED && ch == 0)
                    {
                        endStretcher = std::move(mainStretcher);
                        mainStretcher.initialize(sampleSound.sample, con.currentPosition, sampleSound.sampleRate, int(getSampleRate()),
                            tuning, speedFactor);
                    }
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
            sample *= juce::Decibels::decibelsToGain(float(sampleSound.gain->get()));

            tempOutputBuffer.setSample(ch, i, sample);
        }
    }
    vc = con;

    // Check for updated FX order
    if (updateFXParamsTimer == UPDATE_PARAMS_LENGTH)
        initializeFx();

    // Apply FX
    int reverbSampleDelay = int(1000.f + sampleSound.reverbPredelay->get() * float(getSampleRate()) / 1000.f);  // the 1000.f is approximate
    reverbSampleDelay = 0;
    bool someFXEnabled{ false };
    for (auto& effect : effects)
    {
        // Check for updated enablement
        bool enablement = effect.enablementSource->get();
        if (!effect.enabled && enablement)
            effect.fx->initialize(sampleSound.sample.getNumChannels(), int(getSampleRate()));
        effect.enabled = enablement;
        someFXEnabled = someFXEnabled || effect.enabled;

        if (effect.enabled && !effect.locallyDisabled)
        {
            // Update params every UPDATE_PARAMS_LENGTH calls to process
            if (updateFXParamsTimer == UPDATE_PARAMS_LENGTH)
                effect.fx->updateParams(sampleSound);
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

    mixToBuffer(tempOutputBuffer, outputBuffer, startSample, numSamples, sampleSound.monoOutput->get());
}

float CustomSamplerVoice::fetchSample(int channel, long double position) const
{
    if (0 > position || position >= float(sampleSound.sample.getNumSamples()))
        return 0.f;

    if (sampleSound.skipAntialiasing->get())
        return sampleSound.sample.getSample(channel, int(position));
    else
        return lanczosInterpolate(channel, position);
}

float CustomSamplerVoice::nextSample(int channel, BungeeStretcher* stretcher, juce::AudioBuffer<float>& channelBuffer, int i) const
{
    if (channel == 0)
    {
        for (int ch = 1; ch < sampleSound.sample.getNumChannels(); ch++)
            channelBuffer.setSample(ch - 1, i, stretcher->nextSample(ch, false));
        return stretcher->nextSample(0);
    }
    else
    {
        return channelBuffer.getSample(channel - 1, i);
    }
}

float CustomSamplerVoice::getEnvelopeGain() const
{
    float gain = 1.f;
    if (vc.isSmoothingAttack)
        gain *= vc.positionMovedSinceStart / attackSmoothing;
    if (vc.isReleasing)
        gain *= (std::expf(2 * (releaseSmoothing - vc.positionMovedSinceRelease) / releaseSmoothing) - 1) / (std::expf(2) - 1);
    return gain;
}

//==============================================================================
void CustomSamplerVoice::initializeFx()
{
    auto fxOrder = sampleSound.getFxOrder();
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
                effects.emplace_back(PluginParameters::DISTORTION, std::make_unique<Distortion>(), sampleSound.distortionEnabled);
                break;
            case PluginParameters::REVERB:
                effects.emplace_back(PluginParameters::REVERB, std::make_unique<TriReverb>(), sampleSound.reverbEnabled);
                break;
            case PluginParameters::CHORUS:
                effects.emplace_back(PluginParameters::CHORUS, std::make_unique<Chorus>(expectedBlockSize), sampleSound.chorusEnabled);
                break;
            case PluginParameters::EQ:
                effects.emplace_back(PluginParameters::EQ, std::make_unique<BandEQ>(), sampleSound.eqEnabled);
                break;
            }
        }
    }
}

//==============================================================================
/** Thank god for Wikipedia, I don't really know why this works */
float CustomSamplerVoice::lanczosInterpolate(int channel, long double position) const
{
    int floorIndex = int(floorl(position));

    float result = 0.f;
    for (int i = -LANCZOS_WINDOW_SIZE + 1; i <= LANCZOS_WINDOW_SIZE; i++)
    {
        int iPlus = i + floorIndex;
        float sample = (0 <= iPlus && iPlus < sampleSound.sample.getNumSamples()) ? sampleSound.sample.getSample(channel, iPlus) : 0.f;
        float window = lanczosWindow(position - floorIndex - i);
        result += sample * window;
    }
    return result;
}

float CustomSamplerVoice::lanczosWindow(float x)
{
    return x == 0.f ? 1.f : (LANCZOS_WINDOW_SIZE * std::sinf(juce::MathConstants<float>::pi * x) * std::sinf(juce::MathConstants<float>::pi * x / LANCZOS_WINDOW_SIZE) * INVERSE_SIN_SQUARED / (x * x));
}
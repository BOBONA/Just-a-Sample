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
#include "Effects/Reverb.h"

CustomSamplerVoice::CustomSamplerVoice(const SamplerParameters& samplerSound, int expectedBlockSize) :
    expectedBlockSize(expectedBlockSize), sampleSound(samplerSound),
    mainStretcher(samplerSound.sample, samplerSound.sampleRate),
    loopStretcher(samplerSound.sample, samplerSound.sampleRate),
    endStretcher(samplerSound.sample, samplerSound.sampleRate)
{
    tempOutputBuffer.setSize(sampleSound.sample.getNumChannels(), expectedBlockSize * 2);
    gainBuffer.setSize(sampleSound.sample.getNumChannels(), expectedBlockSize * 2);

    mainStretcherBuffer.setSize(sampleSound.sample.getNumChannels(), expectedBlockSize * 2);
    loopStretcherBuffer.setSize(sampleSound.sample.getNumChannels() - 1, expectedBlockSize * 2);
    endStretcherBuffer.setSize(sampleSound.sample.getNumChannels() - 1, expectedBlockSize * 2);

    mainLowpass.clear();
    loopLowpass.clear();
    endLowpass.clear();
    for (int i = 0; i < sampleSound.sample.getNumChannels(); i++)
    {
        mainLowpass.emplace_back(std::make_unique<LowpassStream>(2 * LANCZOS_WINDOW_SIZE));
        loopLowpass.emplace_back(std::make_unique<LowpassStream>(2 * LANCZOS_WINDOW_SIZE));
        endLowpass.emplace_back(std::make_unique<LowpassStream>(2 * LANCZOS_WINDOW_SIZE));
    }
}

void CustomSamplerVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition)
{
    noteVelocity = velocity;
    if (sound)
    {
        sampleRateConversion = float(sampleSound.sampleRate / getSampleRate());

        sampleStart = sampleSound.sampleStart;  // Note this implicitly loads the atomic value
        sampleEnd = sampleSound.sampleEnd;

        wavetableMode = isWavetableMode(sampleSound.sampleRate, sampleStart, sampleEnd);

        playbackMode = wavetableMode ? PluginParameters::BASIC : sampleSound.getPlaybackMode();

        isLooping = sampleSound.isLooping->get() || wavetableMode;
        loopingHasStart = isLooping && sampleSound.loopingHasStart->get() && sampleSound.loopStart < sampleSound.sampleStart && !wavetableMode;
        loopStart = sampleSound.loopStart;
        loopingHasEnd = isLooping && sampleSound.loopingHasEnd->get() && sampleSound.loopEnd > sampleSound.sampleEnd && !wavetableMode;
        loopEnd = sampleSound.loopEnd;

        effectiveStart = loopingHasStart ? loopStart : sampleStart;
        effectiveEnd = loopingHasEnd ? loopEnd : sampleEnd;

        // While release can be applied before or after the FX, a minimum number of attack smoothing always needs to be applied to the sample before FX
        attackSmoothing = sampleSound.attack->get() * float(getSampleRate()) / 1000.f;
        releaseSmoothing = juce::jmax<float>(PluginParameters::MIN_SMOOTHING_SAMPLES, sampleSound.release->get() * float(getSampleRate()) / 1000.f);
        attackShape = sampleSound.attackShape->get();
        releaseShape = sampleSound.releaseShape->get();
        if (loopingHasEnd)  // Keep release smoothing within end portion, TODO this should be rethought
            releaseSmoothing = juce::jmin<float>(releaseSmoothing, loopEnd - sampleEnd);
        crossfade = juce::jmin<float>(sampleSound.crossfadeSamples, (sampleEnd - sampleStart + 1) / 4.f);  // Keep crossfade within 1/4 of the sample

        vc = VoiceContext();
        midiReleased = false;

        doLowpass = false;
        vc.currentPosition = effectiveStart;
        updateSpeedAndPitch(midiNoteNumber, currentPitchWheelPosition);

        if (playbackMode == PluginParameters::BUNGEE)
            mainStretcher.initialize(effectiveStart, tuning, speedFactor);

        effects.clear();
        initializeFx();
        for (auto& effect : effects)
        {
            effect.fx->initialize(sampleSound.sample.getNumChannels(), int(getSampleRate()));
            effect.fx->updateParams(sampleSound);
        }
     
        updateFXParamsTimer = 0;

        // Set the initial state (vc.currentPosition is set before updateSpeedAndPitch)
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
    pitchWheel = pitchWheelPosition;

    float noteFreq = float(juce::MidiMessage::getMidiNoteInHertz(currentNote, PluginParameters::A4_HZ)) * pow(2.f, juce::jmap<float>(float(pitchWheelPosition), 0.f, 16383.f, -1.f, 1.f) / 12.f);
    float tuningRatio = PluginParameters::A4_HZ / pow(2.f, (sampleSound.semitoneTuning->get() + sampleSound.centTuning->get() / 100.f) / 12.f);
    tuning = noteFreq / tuningRatio;

    speedFactor = sampleSound.speedFactor->get();

    speedFactor *= 1.f + (tuning - 1.f) * sampleSound.octaveSpeedFactor->get();

    if (playbackMode == PluginParameters::BASIC)
    {
        // In wavetable mode, the sample is treated as a single cycle
        // Otherwise, the sample is simply sped up according to the tuning
        float waveformTuning = pow(2.f, (sampleSound.waveformSemitoneTuning->get() + sampleSound.waveformCentTuning->get() / 100.f) / 12.f);
        speed = wavetableMode ? (noteFreq * waveformTuning) * (sampleEnd - sampleStart + 1 - crossfade) / float(getSampleRate()) / 2.f
                              : tuning * sampleRateConversion;

        // Configure the filters
        bool wasLowpass = doLowpass;
        doLowpass = speed > 1.f;
        if (!wasLowpass && doLowpass)
        {
            for (int ch = 0; ch < sampleSound.sample.getNumChannels(); ch++)
                mainLowpass[ch]->resetProcessing(vc.currentPosition);
        }
        if (doLowpass)
        {
            auto frequency = sampleSound.sampleRate / 2.f / speed;
            for (int ch = 0; ch < sampleSound.sample.getNumChannels(); ch++)
            {
                mainLowpass[ch]->setCoefficients(sampleSound.sampleRate, frequency);
                loopLowpass[ch]->setCoefficients(sampleSound.sampleRate, frequency);
                endLowpass[ch]->setCoefficients(sampleSound.sampleRate, frequency);
            }
        }
    }
    else
    {
        // Update the stretchers
        mainStretcher.setPitchAndSpeed(tuning, speedFactor);
        loopStretcher.setPitchAndSpeed(tuning, speedFactor);
        endStretcher.setPitchAndSpeed(tuning, speedFactor);

        speed = speedFactor * sampleRateConversion;
    }
}

void CustomSamplerVoice::setCurrentPlaybackSampleRate(double newRate)
{
    SynthesiserVoice::setCurrentPlaybackSampleRate(newRate);

    mainStretcher.setSampleRate(int(newRate));
    loopStretcher.setSampleRate(int(newRate));
    endStretcher.setSampleRate(int(newRate));
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
    {
        tempOutputBuffer.setSize(tempOutputBuffer.getNumChannels(), numSamples);
        gainBuffer.setSize(gainBuffer.getNumChannels(), numSamples);
    }
    tempOutputBuffer.clear();
    gainBuffer.clear();
    
    if (playbackMode == PluginParameters::BUNGEE && loopStretcherBuffer.getNumSamples() < numSamples)
    {
        mainStretcherBuffer.setSize(mainStretcherBuffer.getNumChannels(), numSamples);
        loopStretcherBuffer.setSize(loopStretcherBuffer.getNumChannels(), numSamples);
        endStretcherBuffer.setSize(endStretcherBuffer.getNumChannels(), numSamples);
    }
    
    updateSpeedAndPitch(getCurrentlyPlayingNote(), pitchWheel);

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
                fetchSample(ch, con.currentPosition, mainLowpass) :
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
                        fetchSample(ch, con.currentPosition + sampleEnd - sampleStart - crossfade, loopLowpass) :
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
                        fetchSample(ch, con.crossfadeEndPosition, endLowpass) :
                        nextSample(ch, &endStretcher, endStretcherBuffer, i);
                    sample = sample * crossfadeIncrease + next * crossfadeDecrease;
                    con.crossfadeEndPosition += speed;
                }
            }

            // Minimum smoothing at sample start
            if (con.speedMovedSinceStart < PluginParameters::MIN_SMOOTHING_SAMPLES)
                sample *= con.speedMovedSinceStart / PluginParameters::MIN_SMOOTHING_SAMPLES;

            // Attack and release envelopes
            gainBuffer.setSample(ch, i, 1.f);

            if (con.isSmoothingAttack)
            {
                if (con.speedMovedSinceStart >= attackSmoothing)
                    con.isSmoothingAttack = false;
                else
                    gainBuffer.setSample(ch, i, exponentialCurve(attackShape, con.speedMovedSinceStart / attackSmoothing));

            }
             
            if (con.isReleasing)
                gainBuffer.setSample(ch, i, gainBuffer.getSample(ch, i) * exponentialCurve(releaseShape, 1 - con.speedMovedSinceRelease / releaseSmoothing));

            // Update the position 
            con.currentPosition += speed;
            con.speedMovedSinceStart += 1;
            if (con.isReleasing)
                con.speedMovedSinceRelease += 1;

            // Handle transitions
            if (con.state == PLAYING && isLooping && con.currentPosition >= sampleEnd - crossfade)  // Loop crossfade
            {
                con.currentPosition -= (sampleEnd - sampleStart + 1) - crossfade;
                con.isCrossfadingLoop = true;

                if (playbackMode == PluginParameters::BUNGEE && ch == 0)
                {
                    std::swap(mainStretcher, loopStretcher);
                    mainStretcher.initialize(con.currentPosition, tuning, speedFactor);  // This could also be done at note start
                }
                else
                {
                    std::swap(mainLowpass[ch], loopLowpass[ch]);
                    mainLowpass[ch]->resetProcessing(int(con.currentPosition));
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

                    if (playbackMode == PluginParameters::BUNGEE && ch == 0)
                    {
                        std::swap(mainStretcher, endStretcher);
                        mainStretcher.initialize(con.currentPosition, tuning, speedFactor);  // This could also be done at note start
                    }
                    else
                    {
                        std::swap(mainLowpass[ch], endLowpass[ch]);
                        mainLowpass[ch]->resetProcessing(con.currentPosition);
                    }
                }
                else
                {
                    con.isReleasing = true;
                }
            }

            if (((con.state == PLAYING && !isLooping) || con.state == PLAYING_END) && !con.isReleasing &&
                con.currentPosition > effectiveEnd - releaseSmoothing * speed)  // Release smoothing
            {
                con.isReleasing = true;
            }

            if (con.currentPosition > effectiveEnd || (con.isReleasing && con.speedMovedSinceRelease >= releaseSmoothing))  // End of playback reached
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

    // Apply envelope here or after FX if PRE_FX is enabled
    if (!sampleSound.applyFXPre->get())
        for (int ch = 0; ch < tempOutputBuffer.getNumChannels(); ch++)
            juce::FloatVectorOperations::multiply(tempOutputBuffer.getWritePointer(ch), tempOutputBuffer.getReadPointer(ch), gainBuffer.getReadPointer(ch), numSamples);

    // Apply FX
    int reverbSampleDelay = int(1000.f + sampleSound.reverbPredelay->get() * float(getSampleRate()) / 1000.f);  // the 1000.f is approximate
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
                effect.fx->updateParams(sampleSound, true);
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

    if (sampleSound.applyFXPre->get())
        for (int ch = 0; ch < tempOutputBuffer.getNumChannels(); ch++)
            juce::FloatVectorOperations::multiply(tempOutputBuffer.getWritePointer(ch), tempOutputBuffer.getReadPointer(ch), gainBuffer.getReadPointer(ch), numSamples);

    updateFXParamsTimer--;
    if (updateFXParamsTimer <= 0)
        updateFXParamsTimer = UPDATE_PARAMS_LENGTH;
    doFxTailOff = !sampleSound.applyFXPre->get() && someFXEnabled && !effects.empty();

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

float CustomSamplerVoice::fetchSample(int channel, long double position, std::vector<std::unique_ptr<LowpassStream>>& lowpassStreams) const
{
    if (0 > position || position >= float(sampleSound.sample.getNumSamples()))
        return 0.f;

    if (sampleSound.skipAntialiasing->get())
    {
        return sampleSound.sample.getSample(channel, int(position));
    }
    else
    {
        return lanczosInterpolate(channel, position, lowpassStreams);
    }
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
        gain *= vc.speedMovedSinceStart / attackSmoothing;
    if (vc.isReleasing)
        gain *= (std::expf(2 * (releaseSmoothing - vc.speedMovedSinceRelease) / releaseSmoothing) - 1) / (std::expf(2) - 1);
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
                effects.emplace_back(PluginParameters::REVERB, std::make_unique<Reverb>(), sampleSound.reverbEnabled);
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
/** Thank god for Wikipedia, I don't really know why this works. https://en.wikipedia.org/wiki/Lanczos_resampling
    The technical details of resampling elude me, but JUCE's filters seem to work well enough for this... 
 */
float CustomSamplerVoice::lanczosInterpolate(int channel, long double position, std::vector<std::unique_ptr<LowpassStream>>& lowpassStreams) const
{
    // First, process the lowpass filter
    auto& lowpassStream = *lowpassStreams[channel];
    if (doLowpass && lowpassStream.getNextSample() < sampleSound.sample.getNumSamples())
    {
        int lastWindowSample = juce::jmin(int(std::floorl(position)) + LANCZOS_WINDOW_SIZE, sampleSound.sample.getNumSamples() - 1);
        lowpassStream.processSamples(sampleSound.sample.getReadPointer(channel, lowpassStream.getNextSample()), lastWindowSample - lowpassStream.getNextSample() + 1);
    }

    // Then, interpolate
    int floorIndex = int(std::floorl(position));

    float result = 0.f;
    for (int i = -LANCZOS_WINDOW_SIZE + 1; i <= LANCZOS_WINDOW_SIZE; i++)
    {
        int iPlus = i + floorIndex;

        float sample = 0.f;
        if (0 <= iPlus && iPlus < sampleSound.sample.getNumSamples())  // Bounds checking is a bit awkward here but handles some edge cases
        { 
            if (doLowpass)
            {
                if (iPlus >= lowpassStream.getStartSample())
                    sample = lowpassStream.getProcessedSample(iPlus);
            }
            else
            {
                sample = sampleSound.sample.getSample(channel, iPlus);
            }
        }

        float window = lanczosWindow(position - floorIndex - i);
        result += sample * window;
    }
    return result;
}

float CustomSamplerVoice::lanczosWindow(float x)
{
    return x == 0.f ? 1.f : (LANCZOS_WINDOW_SIZE * std::sinf(juce::MathConstants<float>::pi * x) * std::sinf(juce::MathConstants<float>::pi * x / LANCZOS_WINDOW_SIZE) * INVERSE_SIN_SQUARED / (x * x));
}
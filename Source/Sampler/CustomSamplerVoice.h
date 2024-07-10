/*
  ==============================================================================

    CustomSamplerVoice.h
    Created: 5 Sep 2023 3:35:03pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "SamplerParameters.h"
#include "effects/Effect.h"
#include "Stretcher.h"

/** This enum includes the different states a voice can be in */
enum VoiceState
{
    PLAYING,  // The voice is still before or during the loop
    PLAYING_END,  // The voice is continuing after the loop
    STOPPED
};

/** The context information for sample by sample processing is stored in its own struct. This
    is primarily to allow for easy multichannel processing but also encapsulates the state nicely.
    Note that the smoothing variables are an important part of the state transition logic.
*/
struct VoiceContext 
{
    VoiceState state{ STOPPED };
    long double currentPosition{ 0 };  // Fractional positions are necessary

    bool isSmoothingAttack{ false };  // The initial attack curve
    bool isCrossfadingLoop{ false };  
    bool isCrossfadingEnd{ false };  // Crossfading between looping and the end part of the sample
    bool isReleasing{ false };  // Active when the note is released or when it nears the end of the sample

    long double crossfadeEndPosition{ 0 };  // The current position of the end crossfade
    float speedMovedSinceStart{ 0 };  // Used to time the attack envelope, note this is in terms of time passed, not position
    float speedMovedSinceRelease{ 0 };  // Used to time the release envelope
    int samplesSinceStopped{ 0 };  // This is needed to time the RMS measurements for reverb tail off (since it has a delay)
};

/** This class is used to store the state of the lowpass filter for a channel / stream
    Because of our use case, a circular buffer is used to store past samples, large enough for the size of the lanczos window.
 */
class LowpassStream
{
public:
    explicit LowpassStream(int bufferSize) : intermediateBuffer(1, bufferSize) {}

    /** Reset the processing state of the stream to a new sample position */
    void resetProcessing(int nextSampleToProcess)
    {
        filter1.reset();
        filter2.reset();
        filter3.reset();
        filter4.reset();

        bufferLoc = 0;
        startSample = nextSampleToProcess;
        nextSample = nextSampleToProcess;
    }

    /** Process a block of samples, storing the recent result in the intermediate buffer.
        nextSample is the index of the next sample that should be processed.
    */
    void processSamples(const float* samples, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float processedSample = filter1.processSingleSampleRaw(samples[i]);
            processedSample = filter2.processSingleSampleRaw(processedSample);
            processedSample = filter3.processSingleSampleRaw(processedSample);
            processedSample = filter4.processSingleSampleRaw(processedSample);

            intermediateBuffer.setSample(0, bufferLoc, processedSample);
            nextSample++;
            bufferLoc = (bufferLoc + 1) % intermediateBuffer.getNumSamples();
        }
    }

    /** Get the processed sample at a given index. Asserts the sample is contained. */
    float getProcessedSample(int sampleIndex) const
    {
        jassert(sampleIndex >= startSample && sampleIndex < nextSample && sampleIndex >= nextSample - intermediateBuffer.getNumSamples());

        int bufferIndex = (sampleIndex - startSample) % intermediateBuffer.getNumSamples();
        return intermediateBuffer.getSample(0, bufferIndex);
    }

    int getNextSample() const { return nextSample; }

    /** Following juce::dsp::FilterDesign::designIIRLowpassHighOrderButterworthMethod(), this is theoretically -48db above 20khz */
    void setCoefficients(float sampleRate, float frequency)
    {
        float order = 8.f;
        filter1.setCoefficients(juce::IIRCoefficients::makeLowPass(sampleRate, frequency, 1.f / (2.f * std::cosf(1.f * juce::MathConstants<float>::pi / (order * 2.f)))));
        filter2.setCoefficients(juce::IIRCoefficients::makeLowPass(sampleRate, frequency, 1.f / (2.f * std::cosf(3.f * juce::MathConstants<float>::pi / (order * 2.f)))));
        filter3.setCoefficients(juce::IIRCoefficients::makeLowPass(sampleRate, frequency, 1.f / (2.f * std::cosf(5.f * juce::MathConstants<float>::pi / (order * 2.f)))));
        filter4.setCoefficients(juce::IIRCoefficients::makeLowPass(sampleRate, frequency, 1.f / (2.f * std::cosf(7.f * juce::MathConstants<float>::pi / (order * 2.f)))));
    }

    int getStartSample() const { return startSample; }

private:
    juce::SingleThreadedIIRFilter filter1;
    juce::SingleThreadedIIRFilter filter2;
    juce::SingleThreadedIIRFilter filter3;
    juce::SingleThreadedIIRFilter filter4;

    juce::AudioBuffer<float> intermediateBuffer;
    int startSample{ 0 };
    int bufferLoc{ 0 };  // Location in the buffer to write to (circular buffer)
    int nextSample{ 0 };  // The next sample to be processed
};

/** This struct serves to separate per instance enablement of effects from the effect classes themselves */
struct Fx
{
    Fx(PluginParameters::FxTypes fxType, std::unique_ptr<Effect> fx, juce::AudioParameterBool* enablementSource) :
        fxType(fxType), fx(std::move(fx)), enablementSource(enablementSource) {}

    PluginParameters::FxTypes fxType;
    std::unique_ptr<Effect> fx;
    juce::AudioParameterBool* enablementSource;
    bool enabled{ false };
    bool locallyDisabled{ false };  // used to avoid empty processing
};

//==============================================================================
/** The CustomSamplerVoice is the main DSP logic of this plugin. It can pitch shift directly or integrate with a 3rd party
    algorithm. It supports antialiasing, an FX chain, different looping modes, attack and release, and smooth crossfading. 
*/
class CustomSamplerVoice final : public juce::SynthesiserVoice
{
public:
    CustomSamplerVoice(const SamplerParameters& samplerSound, int expectedBlockSize);

    /** Updates the speed and pitch, setting stretchers and filter cutoffs correctly.
        Before calling this the first time, set doLowpass = false so that it resets the lowpass filters.
     */
    void updateSpeedAndPitch(int currentNote, int pitchWheelPosition);

    void setCurrentPlaybackSampleRate(double newRate) override;

    //==============================================================================
    /** Returns whether the voice is actively playing (not stopped or tailing off) */
    bool isPlaying() const { return getCurrentlyPlayingSound() && vc.state != STOPPED; }

    /** Get the effective location of the sampler voice relative to the original sample, not precise in ADVANCED mode */
    long double getPosition() const { return vc.currentPosition; }

    /** Get the current gain of the voice in the attack and release envelopes, for visualization */
    float getEnvelopeGain() const;

private:
    bool canPlaySound(juce::SynthesiserSound*) override { return true; }
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override {}
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    //==============================================================================
    /** Fetch a sample at a given position, in BASIC mode.
        Provide a vector of lowpass streams to apply lowpass filtering before interpolation (if doLowpass).
        This is necessary to avoid frequencies going above the Nyquist frequency. 
    */
    float fetchSample(int channel, long double position, std::vector<std::unique_ptr<LowpassStream>>& lowpassStreams) const;

    /** Fetches the next sample from a stretcher, in ADVANCED mode. Note that on channel 0, the stretcher
        advances and stores the other channels' output in the channel buffer at index i. Then it's fetched
        from there when nextSample is called with the later channel.
     */
    float nextSample(int channel, BungeeStretcher* stretcher, juce::AudioBuffer<float>& channelBuffer, int i) const;

    /** Use a Lanczos kernel to calculate fractional sample indices. Applies a lowpass filter beforehand, if doLowpass. */
    float lanczosInterpolate(int channel, long double position, std::vector<std::unique_ptr<LowpassStream>>& lowpassStreams) const;

    inline static float lanczosWindow(float x);
    static constexpr int LANCZOS_WINDOW_SIZE{ 5 };

    /** Initialize or updates (by reinitializing) the effect chain. This is not real-time safe, but I don't think reordering needs to be. */
    void initializeFx();

    //==============================================================================
    int expectedBlockSize;

    const SamplerParameters& sampleSound;
    float sampleRateConversion{ 0 };  // Loaded sample rate / application sample rate
    float speed{ 0 };  // Used in BASIC mode
    int effectiveStart{ 0 };
    int effectiveEnd{ 0 };

    /** "Wavetable mode" activates when the bounds are very short and can act as a waveform cycle. */
    bool wavetableMode{ false };

    // Unchanging sampler sound parameters
    PluginParameters::PLAYBACK_MODES playbackMode{ PluginParameters::PLAYBACK_MODES::BASIC };
    float tuning{ 0 };
    int pitchWheel{ 0 };
    float speedFactor{ 0 };  // Used in ADVANCED mode
    float noteVelocity{ 0 };

    bool isLooping{ false }, loopingHasStart{ false }, loopingHasEnd{ false };
    int sampleStart{ 0 }, sampleEnd{ 0 }, loopStart{ 0 }, loopEnd{ 0 };

    /** We call this "smoothing" but it's a normal attack/release envelope. */
    float attackSmoothing{ 0 }, releaseSmoothing{ 0 };
    float crossfade{ 0 };

    VoiceContext vc;
    bool midiReleased{ false };
    juce::AudioBuffer<float> tempOutputBuffer;

    BungeeStretcher mainStretcher;
    BungeeStretcher loopStretcher;
    BungeeStretcher endStretcher;

    // Since the stretchers process channels together, buffers are needed to store the output
    juce::AudioBuffer<float> mainStretcherBuffer;
    juce::AudioBuffer<float> loopStretcherBuffer;
    juce::AudioBuffer<float> endStretcherBuffer;

    bool doLowpass{ false };
    std::vector<std::unique_ptr<LowpassStream>> mainLowpass;
    std::vector<std::unique_ptr<LowpassStream>> loopLowpass;
    std::vector<std::unique_ptr<LowpassStream>> endLowpass;

    //==============================================================================
    bool doFxTailOff{ false };
    static constexpr int UPDATE_PARAMS_LENGTH{ 4 }; // after how many process calls should we query for FX params
    int updateFXParamsTimer{ 0 };
    std::vector<Fx> effects;
};

static constexpr float INVERSE_SIN_SQUARED{ 1.f / (juce::MathConstants<float>::pi * juce::MathConstants<float>::pi) };
/*
  ==============================================================================

    CustomSamplerVoice.h
    Created: 5 Sep 2023 3:35:03pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "CustomSamplerSound.h"
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
    float currentPosition{ 0 };  // Fractional positions are necessary

    bool isSmoothingAttack{ false };  // The initial attack curve
    bool isCrossfadingLoop{ false };  
    bool isCrossfadingEnd{ false };  // Crossfading between looping and the end part of the sample
    bool isReleasing{ false };  // Active when the note is released or when it nears the end of the sample

    float crossfadeEndPosition{ 0 };  // The current position of the end crossfade
    float positionMovedSinceRelease{ 0 };  // Used to time the release envelope
    int samplesSinceStopped{ 0 };  // This is needed to time the RMS measurements for reverb tail off (since it has a delay)
};

/** This struct serves to separate per instance enablement of effects from the effect classes themselves */
struct Fx
{
    Fx(PluginParameters::FxTypes fxType, std::unique_ptr<Effect> fx, juce::Value enablementSource) : fxType(fxType), fx(std::move(fx)), enablementSource(std::move(enablementSource))
    {
    }

    PluginParameters::FxTypes fxType;
    std::unique_ptr<Effect> fx;
    juce::Value enablementSource;
    bool enabled{ false };
    bool locallyDisabled{ false };  // used to avoid empty processing
};

/** The CustomSamplerVoice is the main DSP logic of this plugin. It can pitch shift directly or integrate with a 3rd party
    algorithm. It supports antialiasing, an FX chain, different looping modes, attack and release, and smooth crossfading. 
*/
class CustomSamplerVoice final : public juce::SynthesiserVoice
{
public:
    explicit CustomSamplerVoice(int expectedBlockSize);

    //==============================================================================
    /** Returns whether the voice is actively playing (not stopped or tailing off) */
    bool isPlaying() const { return getCurrentlyPlayingSound() && vc.state != STOPPED; }

    /** Get the effective location of the sampler voice relative to the original sample, not precise in ADVANCED mode */
    float getPosition() const { return vc.currentPosition; }

private:
    bool canPlaySound(juce::SynthesiserSound*) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override {}
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    //==============================================================================
    /** Fetch a sample at a given position, in BASIC mode. */
    float fetchSample(int channel, float position) const;

    /** Fetches the next sample from a stretcher, in ADVANCED mode. Note that on channel 0, the stretcher
        advances and stores the other channels' output in the channel buffer at index i. Then it's fetched
        from there when nextSample is called with the later channel.
     */
    float nextSample(int channel, BungeeStretcher* stretcher, juce::AudioBuffer<float>& channelBuffer, int i) const;

    /** Use a Lanczos kernel to calculate fractional sample indices */
    float lanczosInterpolate(int channel, float position) const;

    inline static float lanczosWindow(float x);
    static constexpr int LANCZOS_WINDOW_SIZE{ 5 };

    /** Initialize or updates (by reinitializing) the effect chain */
    void initializeFx();

    //==============================================================================
    int expectedBlockSize;

    CustomSamplerSound* sampleSound{ nullptr };
    float sampleRateConversion{ 0 };  // Loaded sample rate / application sample rate
    float speed{ 0 };  // Used in BASIC mode
    float effectiveStart{ 0 };
    float effectiveEnd{ 0 };

    // Unchanging sampler sound parameters
    PluginParameters::PLAYBACK_MODES playbackMode{ PluginParameters::PLAYBACK_MODES::BASIC };
    float tuning{ 0 };
    float speedFactor{ 0 };  // Used in ADVANCED mode
    float noteVelocity{ 0 };
    bool isLooping{ false }, loopingHasStart{ false }, loopingHasEnd{ false };
    float sampleStart{ 0 }, sampleEnd{ 0 }, loopStart{ 0 }, loopEnd{ 0 };
    float attackSmoothing{ 0 };
    float releaseSmoothing{ 0 };
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

    //==============================================================================
    bool doFxTailOff{ false };
    static constexpr int UPDATE_PARAMS_LENGTH{ 4 }; // after how many process calls should we query for FX params
    int updateFXParamsTimer{ 0 };
    std::vector<Fx> effects;
};

static constexpr float INVERSE_SIN_SQUARED{ 1.f / (juce::MathConstants<float>::pi * juce::MathConstants<float>::pi) };
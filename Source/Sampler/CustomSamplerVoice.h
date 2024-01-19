/*
  ==============================================================================

    CustomSamplerVoice.h
    Created: 5 Sep 2023 3:35:03pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <RubberBandStretcher.h>

#include "CustomSamplerSound.h"
#include "PitchShifter.h"
#include "effects/TriReverb.h"
#include "effects/Distortion.h"
#include "effects/BandEQ.h"
#include "effects/Chorus.h"

using namespace juce;

enum VoiceState
{
    PREPROCESSING,
    PLAYING, // this doubles as the loop start when that's enabled
    LOOPING,
    RELEASING, // for loop end portion
    STOPPED
};

struct VoiceContext {

    VoiceState state{ STOPPED };
    int effectiveStart{ 0 }; // this is used to minimize conditionals when dealing with the start and release buffers
    int currentSample{ 0 }; // includes bufferPitcher->startDelay when playbackMode == ADVANCED, includes effectiveStart (note that effectiveStart is in original sample rate)
    int noteDuration{ 0 };
    int samplesSinceStopped{ 0 }; // to keep track of whether the reverb has started outputting

    int midiReleasedSamples{ 0 };

    bool isSmoothingStart{ false };
    bool isSmoothingLoop{ false };
    bool isSmoothingRelease{ false };
    bool isSmoothingEnd{ false };

    int smoothingStartSample{ 0 };
    int smoothingLoopSample{ 0 };
    int smoothingReleaseSample{ 0 };
    int smoothingEndSample{ 0 };
};

/* This struct serves to separate per instance enablement of effects from the effect classes themselves */
struct Fx
{
    Fx(PluginParameters::FxTypes fxType, std::unique_ptr<Effect> fx, Value& enablementSource) : fxType(fxType), fx(std::move(fx)), enablementSource(enablementSource)
    {
    };

    PluginParameters::FxTypes fxType;
    std::unique_ptr<Effect> fx;
    Value enablementSource;
    bool enabled{ false };
    bool locallyDisabled{ false }; // used for avoiding empty processing
};

class CustomSamplerVoice : public SynthesiserVoice
{
public:
    CustomSamplerVoice(int numChannels, int expectedBlockSize);
    ~CustomSamplerVoice();
    // Inherited via SynthesiserVoice
    bool canPlaySound(SynthesiserSound*) override;
    void startNote(int midiNoteNumber, float velocity, SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    /** Initialize or updates (by reinitializing) the effect chain */
    void initializeFx();

    /** Use a Lanczos kernel to calculate fractional sample indices */
    float lanczosInterpolate(int channel, float index);
    static float lanczosWindow(float x);

    /** Get the effective location of the sampler voice relative to the original sample, not precise in ADVANCED mode */
    int getEffectiveLocation();

    /* In BASIC mode, get the current sample index (currentSample is more an indicator of how many samples have been processed so far) */
    float getBasicLoc(int currentSample, int effectiveStart) const;

    /** In BASIC mode, fetch a sample from its index (uses lanczosInterpolate if DO_ANTIALIASING) */
    float getBasicSample(int channel, float sampleIndex);

    PluginParameters::PLAYBACK_MODES& getPlaybackMode()
    {
        return playbackMode;
    }

    VoiceContext& getContext()
    {
        return vc;
    }

    std::unique_ptr<BufferPitcher>& getBufferPitcher(VoiceState state)
    {
        return state == RELEASING ? releaseBuffer : startBuffer;
    }

    float getSampleRateConversion() const
    {
        return sampleRateConversion;
    }

private:
    const static int LANCZOS_SIZE{ 5 };
    const static dsp::LookupTableTransform<float> lanczosLookup;

    CustomSamplerSound* sampleSound{ nullptr };
    int expectedBlockSize;
    float sampleRateConversion{ 0 };
    float tuningRatio{ 0 };
    float speedFactor{ 0 };
    float noteFreq{ 0 }; // accounts for pitch wheel
    float noteVelocity{ 0 };
    bool isLooping{ false }, loopingHasStart{ false }, loopingHasEnd{ false };
    int sampleStart{ 0 }, sampleEnd{ 0 }, loopStart{ 0 }, loopEnd{ 0 };
    PluginParameters::PLAYBACK_MODES playbackMode{ PluginParameters::PLAYBACK_MODES::BASIC };
    int numChannels;
    
    AudioBuffer<float> tempOutputBuffer;
    int effectiveEnd{ 0 };
    juce::Array<float> previousSample;
    std::unique_ptr<BufferPitcher> startBuffer; // assumption is these have the same startDelay
    std::unique_ptr<BufferPitcher> releaseBuffer;

    int preprocessingTotalSamples{ 0 };
    int preprocessingSample{ 0 };

    bool midiReleased{ false };

    bool doStartStopSmoothing{ false }; // this entails smoothing the start and delaying midi release to smooth the stop
    bool doCrossfadeSmoothing{ false }; // this entails crossfading between the loop end and beginning, and also crossfading to the release section
    int startStopSmoothingSamples{ 0 };
    int crossfadeSmoothingSamples{ 0 };

    VoiceContext vc;

    bool doFxTailOff{ false };
    const int UPDATE_PARAMS_LENGTH{ 4 }; // how many process calls to query for params
    int updateParams{ 0 };
    std::vector<Fx> effects;
};

const static float INVERSE_SIN_SQUARED{ 1.f / (MathConstants<float>::pi * MathConstants<float>::pi) };
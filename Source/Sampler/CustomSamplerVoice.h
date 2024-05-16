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

/** This enum includes the different states a voice can be in */
enum VoiceState
{
    PREPROCESSING,
    PLAYING,
    LOOPING, // when looping is enabled
    RELEASING, // for loop end portion
    STOPPED
};

/** The context information for sample by sample processing is stored in its own struct. This
    is primarily to allow for easy multi-channel processing but also encapsulates the state nicely.
*/
struct VoiceContext 
{
    VoiceState state{ STOPPED };
    int effectiveStart{ 0 };  // this is used to minimize conditionals when dealing with the start and release buffers
    int currentSample{ 0 };  // includes bufferPitcher->startDelay when playbackMode == ADVANCED, includes effectiveStart (note that effectiveStart is in original sample rate)
    int noteDuration{ 0 };
    int samplesSinceStopped{ 0 };  // to keep track of whether the reverb has started outputting

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

/** This struct serves to separate per instance enablement of effects from the effect classes themselves */
struct Fx
{
    Fx(PluginParameters::FxTypes fxType, std::unique_ptr<Effect> fx, juce::Value& enablementSource) : fxType(fxType), fx(std::move(fx)), enablementSource(enablementSource)
    {
    };

    PluginParameters::FxTypes fxType;
    std::unique_ptr<Effect> fx;
    juce::Value enablementSource;
    bool enabled{ false };
    bool locallyDisabled{ false }; // used for avoiding empty processing
};

/** The CustomSamplerVoice is the main DSP logic of this plugin. It can pitch shift directly or integrate with a 3rd party
    algorithm. It supports antialiasing, an FX chain, different looping modes, attack and release, and smooth crossfading. 
*/
class CustomSamplerVoice : public juce::SynthesiserVoice
{
public:
    CustomSamplerVoice(int numChannels, int expectedBlockSize);
    ~CustomSamplerVoice() override;

    //==============================================================================
    bool canPlaySound(juce::SynthesiserSound*) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    //==============================================================================
    /** Get the effective location of the sampler voice relative to the original sample, not precise in ADVANCED mode */
    int getEffectiveLocation();

    /** Returns the current VoiceState, for display purposes */
    VoiceState getVoiceState() const { return vc.state; }

private:
    /* In BASIC mode, get the current sample index (currentSample is more an indicator of how many samples have been processed so far) */
    float getBasicLoc(int currentSample, int effectiveStart) const;

    /** In BASIC mode, fetch a sample from its index (uses lanczosInterpolate if DO_ANTIALIASING) */
    float getBasicSample(int channel, float sampleIndex);

    /** In ADVANCED mode, fetch the active buffer pitcher */
    std::unique_ptr<BufferPitcher>& getBufferPitcher(VoiceState state) { return state == RELEASING ? releaseBuffer : startBuffer; }

    /** Initialize or updates (by reinitializing) the effect chain */
    void initializeFx();

    /** Use a Lanczos kernel to calculate fractional sample indices */
    float lanczosInterpolate(int channel, float index);
    static float lanczosWindow(float x);

    const static int LANCZOS_SIZE{ 5 };
    const static juce::dsp::LookupTableTransform<float> lanczosLookup;

    //==============================================================================
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

    bool doStartStopSmoothing{ false }; // this entails smoothing the start and delaying midi release to smooth the stop
    bool doCrossfadeSmoothing{ false }; // this entails crossfading between the loop end and beginning, and also crossfading to the release section
    int startStopSmoothingSamples{ 0 };
    int crossfadeSmoothingSamples{ 0 };

    //==============================================================================
    juce::AudioBuffer<float> tempOutputBuffer;
    int effectiveEnd{ 0 };
    juce::Array<float> previousSample;
    std::unique_ptr<BufferPitcher> startBuffer; // assumption is these have the same startDelay
    std::unique_ptr<BufferPitcher> releaseBuffer;

    int preprocessingTotalSamples{ 0 };
    int preprocessingSample{ 0 };

    bool midiReleased{ false };

    VoiceContext vc;

    //==============================================================================
    bool doFxTailOff{ false };
    const int UPDATE_PARAMS_LENGTH{ 4 }; // after how many process calls should we query for FX params
    int updateParams{ 0 };
    std::vector<Fx> effects;
};

const static float INVERSE_SIN_SQUARED{ 1.f / (juce::MathConstants<float>::pi * juce::MathConstants<float>::pi) };
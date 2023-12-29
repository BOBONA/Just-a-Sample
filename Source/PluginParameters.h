/*
  ==============================================================================

    PluginParameters.h
    Created: 4 Oct 2023 10:40:47am
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class PluginParameters
{
public:
    inline static const juce::String WIDTH{ "Width" };
    inline static const juce::String HEIGHT{ "Height" };

    inline static const juce::String FILE_PATH{ "File_Path" };
    // These two are managed by the SampleNavigator
    inline static const juce::String UI_VIEW_START{ "UI_View_Start" };
    inline static const juce::String UI_VIEW_END{ "UI_View_Stop" };

    inline static const juce::String SAMPLE_START{ "Sample_Start" };
    inline static const juce::String SAMPLE_END{ "Sample_End" };
    inline static const juce::String LOOP_START{ "Loop_Start" };
    inline static const juce::String LOOP_END{ "Loop_End" };
    
    inline static const juce::String IS_LOOPING{ "Is_Looping" };
    inline static const juce::String LOOPING_HAS_START{ "Looping_Has_Start" };
    inline static const juce::String LOOPING_HAS_END{ "Looping_Has_End" };

    inline static const juce::String PLAYBACK_MODE{ "Playback_Mode" };
    inline static const juce::StringArray PLAYBACK_MODE_LABELS{ "Pitch Shifting: Basic", "Pitch Shifting: Advanced" };
    inline static const enum PLAYBACK_MODES
    {
        BASIC,
        ADVANCED
    };
    inline static const PLAYBACK_MODES getPlaybackMode(float value)
    {
        return static_cast<PluginParameters::PLAYBACK_MODES>(int(value));
    }

    inline static const juce::String MASTER_GAIN{ "Master_Gain" };
    inline static const juce::NormalisableRange<float> MASTER_GAIN_RANGE_DB{ -15.f, 15.f, 0.1f, 0.5f, true };
    
    inline static const juce::String SEMITONE_TUNING{ "Semitone_Tuning" };
    inline static const juce::Range<int> SEMITONE_TUNING_RANGE{ -12, 12 };
    inline static const juce::String CENT_TUNING{ "Cent_Tuning" };
    inline static const juce::Range<int> CENT_TUNING_RANGE{ -100, 100 };

    inline static const float SPEED_FACTOR = 1.f; // a speed control for ADVANCED

    inline static const int NUM_VOICES = 8;
    inline static const bool PREPROCESS_STEP{ true };
    inline static const bool PREPROCESS_RELEASE_BUFFER{ false };
    inline static const bool DO_START_STOP_SMOOTHING{ true };
    inline static const bool DO_CROSSFADE_SMOOTHING{ true };
    inline static const int START_STOP_SMOOTHING{ 600 }; // this probably won't be customizable
    inline static const int CROSSFADE_SMOOTHING{ 1500 }; // this will be user-customizable

    inline static const float A4_HZ = 440;

    inline static const bool FX_TAIL_OFF{ true };
    inline static const bool REVERB_ENABLED{ true };
};
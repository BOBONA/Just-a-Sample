/*
  ==============================================================================

    PluginParameters.h
    Created: 4 Oct 2023 10:40:47am
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

using namespace juce;

class PluginParameters
{
public:
    inline static const String WIDTH{ "Width" };
    inline static const String HEIGHT{ "Height" };

    inline static const String FILE_PATH{ "File_Path" };

    // These two are managed by the SampleNavigator
    inline static const String UI_VIEW_START{ "UI_View_Start" };
    inline static const String UI_VIEW_END{ "UI_View_Stop" };

    // These (and the two above) are part of a separate ValueTree, mainly because their ranges are dynamic depending on the loaded file
    inline static const String SAMPLE_START{ "Sample_Start" };
    inline static const String SAMPLE_END{ "Sample_End" };
    inline static const String LOOP_START{ "Loop_Start" };
    inline static const String LOOP_END{ "Loop_End" };
    
    inline static const String IS_LOOPING{ "Is_Looping" };
    inline static const String LOOPING_HAS_START{ "Looping_Has_Start" };
    inline static const String LOOPING_HAS_END{ "Looping_Has_End" };

    inline static const String PLAYBACK_MODE{ "Playback_Mode" };
    inline static const StringArray PLAYBACK_MODE_LABELS{ "Pitch Shifting: Basic", "Pitch Shifting: Advanced" };
    inline static const enum PLAYBACK_MODES
    {
        BASIC,
        ADVANCED
    };
    inline static const PLAYBACK_MODES getPlaybackMode(float value)
    {
        return static_cast<PluginParameters::PLAYBACK_MODES>(int(value));
    }

    inline static const String MASTER_GAIN{ "Master_Gain" };
    inline static const NormalisableRange<float> MASTER_GAIN_RANGE_DB{ -15.f, 15.f, 0.1f, 0.5f, true };
    
    inline static const String SEMITONE_TUNING{ "Semitone_Tuning" };
    inline static const Range<int> SEMITONE_TUNING_RANGE{ -12, 12 };
    inline static const String CENT_TUNING{ "Cent_Tuning" };
    inline static const Range<int> CENT_TUNING_RANGE{ -100, 100 };

    inline static const String REVERB_ENABLED{ "Reverb_Enabled" };
    inline static const String REVERB_MIX{ "Reverb_Mix" };
    inline static const Range<float> REVERB_MIX_RANGE{ 0, 1 };
    inline static const String REVERB_SIZE{ "Reverb_Size" };
    inline static const Range<float> REVERB_SIZE_RANGE{ 0, 1};
    inline static const String REVERB_DAMPING{ "Reverb_Damping" };
    inline static const Range<float> REVERB_DAMPING_RANGE{ 0, 1 };
    inline static const String REVERB_LOWPASS{ "Reverb_Lowpass" };
    inline static const Range<float> REVERB_LOWPASS_RANGE{ 0, 1 };
    inline static const String REVERB_HIGHPASS{ "Reverb_Highpass" };
    inline static const Range<float> REVERB_HIGHPASS_RANGE{ 0, 1 };
    inline static const String REVERB_PREDELAY{ "Reverb_Predelay" };
    inline static const Range<float> REVERB_PREDELAY_RANGE{ 0, 1 };

    inline static const String DISTORTION_ENABLED{ "Distortion_Enabled" };
    inline static const String DISTORTION_DENSITY{ "Distortion_Density" };
    inline static const Range<float> DISTORTION_DENSITY_RANGE{ 0, 1 };
    inline static const String DISTORTION_HIGHPASS{ "Distortion_Highpass" };
    inline static const Range<float> DISTORTION_HIGHPASS_RANGE{ 0, 0.999 };
    inline static const String DISTORTION_OUTPUT{ "Distortion_Output" };
    inline static const Range<float> DISTORTION_OUTPUT_RANGE{ 0, 1 };
    inline static const String DISTORTION_MIX{ "Distortion_Mix" };
    inline static const Range<float> DISTORTION_MIX_RANGE{ 0, 1 };

    inline static const String EQ_ENABLED{ "EQ_Enabled" };
    inline static const String EQ_LOW_GAIN{ "EQ_Low_Gain" };
    inline static const Range<float> EQ_LOW_GAIN_RANGE{ -12, 12 }; // decibels
    inline static const String EQ_MID_GAIN{ "EQ_Mid_Gain" };
    inline static const Range<float> EQ_MID_GAIN_RANGE{ -12, 12 };
    inline static const String EQ_HIGH_GAIN{ "EQ_High_Gain" };
    inline static const Range<float> EQ_HIGH_GAIN_RANGE{ -12, 12 };
    inline static const String EQ_LOW_FREQ{ "EQ_Low_Freq" };
    inline static const Range<float> EQ_LOW_FREQ_RANGE{ 25, 600 };
    inline static const String EQ_HIGH_FREQ{ "EQ_High_Freq" };
    inline static const Range<float> EQ_HIGH_FREQ_RANGE{ 700, 15500 };

    inline static const String CHORUS_ENABLED{ "Chorus_Enabled" };
    inline static const String CHORUS_RATE{ "Chorus_Rate" }; 
    inline static const Range<float> CHORUS_RATE_RANGE{ 0.1, 99.9 }; // in hz
    inline static const String CHORUS_DEPTH{ "Chorus_Depth" }; 
    inline static const Range<float> CHORUS_DEPTH_RANGE{ 0.01, 1 };
    inline static const String CHORUS_FEEDBACK{ "Chorus_Feedback" }; 
    inline static const Range<float> CHORUS_FEEDBACK_RANGE{ -0.95, 0.95 };
    inline static const String CHORUS_CENTER_DELAY{ "Chorus_Center_Delay" };
    inline static const Range<float> CHORUS_CENTER_DELAY_RANGE{ 1, 99.9 }; // in ms
    inline static const String CHORUS_MIX{ "Chorus_Mix" };
    inline static const Range<float> CHORUS_MIX_RANGE{ 0, 1 };

    inline static const bool FX_TAIL_OFF{ true };
    inline static const float FX_TAIL_OFF_MAX{ 0.0001f };
    inline static const enum REVERB_TYPES
    {
        JUCE,
        GIN_SIMPLE,
        GIN_PLATE
    };
    inline static const REVERB_TYPES REVERB_TYPE{ GIN_SIMPLE };

    inline static const float SPEED_FACTOR{ 1.f }; // a speed control for ADVANCED
    inline static const int NUM_VOICES{ 32 };
    inline static const float A4_HZ{ 440 };
    inline static const bool MONO{ false };

    inline static const bool PREPROCESS_STEP{ true };
    inline static const bool PREPROCESS_RELEASE_BUFFER{ false };
    inline static const bool DO_START_STOP_SMOOTHING{ true };
    inline static const bool DO_CROSSFADE_SMOOTHING{ true };
    inline static const int START_STOP_SMOOTHING{ 600 }; // this probably won't be customizable
    inline static const int CROSSFADE_SMOOTHING{ 1500 }; // this will be user-customizable
};
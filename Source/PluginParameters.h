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

/** A header of many constant values needed for the APVTS and other parts of the plugin. Note that default values for the APVTS are currently in PluginProcessor. */
namespace PluginParameters
{
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
const enum PLAYBACK_MODES
{
    BASIC,
    ADVANCED
};
inline static const PLAYBACK_MODES getPlaybackMode(float value)
{
    return static_cast<PluginParameters::PLAYBACK_MODES>(int(value));
}
inline static const String SKIP_ANTIALIASING{ "Lofi_Pitching" };

inline static const String MASTER_GAIN{ "Master_Gain" };
inline static const NormalisableRange<float> MASTER_GAIN_RANGE_DB{ -15.f, 15.f, 0.1f, 0.5f, true };
inline static const int NUM_VOICES{ 16 };
inline static const float A4_HZ{ 440 };
inline static const String MONO_OUTPUT{ "Mono_Output" };

// some controls for advanced
inline static const String SPEED_FACTOR{ "Speed_Factor" };
inline static const NormalisableRange<float> SPEED_FACTOR_RANGE{ 0.2f, 5.f, 0.01f, 0.3f };
inline static const String FORMANT_PRESERVED{ "Formant_Preserved" };

inline static const bool PREPROCESS_STEP{ true };
inline static const bool PREPROCESS_RELEASE_BUFFER{ false };
inline static const bool DO_START_STOP_SMOOTHING{ true };
inline static const bool DO_CROSSFADE_SMOOTHING{ true };
inline static const int START_STOP_SMOOTHING{ 600 }; // this probably won't be customizable
inline static const int CROSSFADE_SMOOTHING{ 1500 }; // this will be user-customizable
    
inline static const String SEMITONE_TUNING{ "Semitone_Tuning" };
inline static const Range<int> SEMITONE_TUNING_RANGE{ -12, 12 };
inline static const String CENT_TUNING{ "Cent_Tuning" };
inline static const Range<int> CENT_TUNING_RANGE{ -100, 100 };

inline static const String REVERB_ENABLED{ "Reverb_Enabled" };
inline static const String REVERB_MIX{ "Reverb_Mix" };
inline static const Range<float> REVERB_MIX_RANGE{ 0.f, 1.f };
inline static const String REVERB_SIZE{ "Reverb_Size" };
inline static const Range<float> REVERB_SIZE_RANGE{ 5.f, 100.f}; 
inline static const String REVERB_DAMPING{ "Reverb_Damping" };
inline static const Range<float> REVERB_DAMPING_RANGE{ 0.f, 95.f }; 
inline static const String REVERB_LOWS{ "Reverb_Lows" }; // these controls map to filters built into gin's SimpleVerb
inline static const Range<float> REVERB_LOWS_RANGE{ 0.f, 1.f };
inline static const String REVERB_HIGHS{ "Reverb_Highs" };
inline static const Range<float> REVERB_HIGHS_RANGE{ 0.f, 1.f };
inline static const String REVERB_PREDELAY{ "Reverb_Predelay" };
inline static const NormalisableRange<float> REVERB_PREDELAY_RANGE{ 0.f, 500.f, 0.1f, 0.5f }; // in milliseconds

inline static const String DISTORTION_ENABLED{ "Distortion_Enabled" };
inline static const String DISTORTION_DENSITY{ "Distortion_Density" };
inline static const Range<float> DISTORTION_DENSITY_RANGE{ -0.5f, 1.f };
inline static const String DISTORTION_HIGHPASS{ "Distortion_Highpass" };
inline static const Range<float> DISTORTION_HIGHPASS_RANGE{ 0.f, 0.999f };
inline static const String DISTORTION_MIX{ "Distortion_Mix" };
inline static const Range<float> DISTORTION_MIX_RANGE{ 0.f, 1.f };

inline static const String EQ_ENABLED{ "EQ_Enabled" };
inline static const String EQ_LOW_GAIN{ "EQ_Low_Gain" };
inline static const Range<float> EQ_LOW_GAIN_RANGE{ -12.f, 12.f }; // decibels
inline static const String EQ_MID_GAIN{ "EQ_Mid_Gain" };
inline static const Range<float> EQ_MID_GAIN_RANGE{ -12.f, 12.f };
inline static const String EQ_HIGH_GAIN{ "EQ_High_Gain" };
inline static const Range<float> EQ_HIGH_GAIN_RANGE{ -12.f, 12.f };
inline static const String EQ_LOW_FREQ{ "EQ_Low_Freq" };
inline static const Range<float> EQ_LOW_FREQ_RANGE{ 25.f, 600.f };
inline static const String EQ_HIGH_FREQ{ "EQ_High_Freq" };
inline static const Range<float> EQ_HIGH_FREQ_RANGE{ 700.f, 15500.f };

inline static const String CHORUS_ENABLED{ "Chorus_Enabled" };
inline static const String CHORUS_RATE{ "Chorus_Rate" }; 
inline static const Range<float> CHORUS_RATE_RANGE{ 0.1f, 99.9f }; // in hz
inline static const String CHORUS_DEPTH{ "Chorus_Depth" }; 
inline static const Range<float> CHORUS_DEPTH_RANGE{ 0.01f, 1.f };
inline static const String CHORUS_FEEDBACK{ "Chorus_Feedback" }; 
inline static const Range<float> CHORUS_FEEDBACK_RANGE{ -0.95f, 0.95f };
inline static const String CHORUS_CENTER_DELAY{ "Chorus_Center_Delay" };
inline static const Range<float> CHORUS_CENTER_DELAY_RANGE{ 1.f, 99.9f }; // in ms
inline static const String CHORUS_MIX{ "Chorus_Mix" };
inline static const Range<float> CHORUS_MIX_RANGE{ 0.f, 1.f };

inline static const String FX_PERM{ "Fx_Perm" };
const enum FxTypes
{
    DISTORTION,
    CHORUS,
    REVERB,
    EQ
};
/** for converting between the parameter and the permutation */
inline static const std::array<FxTypes, 4> paramToPerm(int fxParam) 
{
    std::vector<FxTypes> types{ DISTORTION, CHORUS, REVERB, EQ };
    std::array<FxTypes, 4> perm{};
    int factorial = 6;
    for (int i = 0; i < 4; i++)
    {
        int index = fxParam / factorial;
        perm[i] = types[index];
        types.erase(types.begin() + index);
        fxParam %= factorial;
        if (i < 3)
            factorial /= 3 - i;
    }
    return perm;
}
inline static const int permToParam(std::array<FxTypes, 4> fxPerm) 
{
    std::vector<FxTypes> types{ DISTORTION, CHORUS, REVERB, EQ };
    int result = 0;
    int factorial = 6;
    for (int i = 0; i < 3; i++)
    {
        int type = fxPerm[i];
        int index;
        for (index = 0; index < types.size(); index++)
            if (type == types[index])
                break;
        result += factorial * index;
        types.erase(types.begin() + index);
        if (i < 3)
            factorial /= 3 - i;
    }
    return result;
}

inline static const bool FX_TAIL_OFF{ true };
inline static const float FX_TAIL_OFF_MAX{ 0.0001f };
const enum REVERB_TYPES
{
    JUCE,
    GIN_SIMPLE,
    GIN_PLATE
};
inline static const REVERB_TYPES REVERB_TYPE{ GIN_SIMPLE };
};
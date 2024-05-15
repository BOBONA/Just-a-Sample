/*
  ==============================================================================

    PluginParameters.h
    Created: 4 Oct 2023 10:40:47am
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

/** This namespace contains all APVTS parameter IDs, various plugin configuration settings, and the parameter layout */
namespace PluginParameters
{
using String = juce::String;
using StringArray = juce::StringArray;
using NormalisableRange = juce::NormalisableRange<float>;
using Range = juce::Range<float>;

// Layout
inline static const String WIDTH{ "Width" };
inline static const String HEIGHT{ "Height" };

inline static const int FRAME_RATE{ 60 };

// Sample storage
inline static const String FILE_PATH{ "File_Path" };  // stored in apvts.state
inline static const String SAMPLE_HASH{ "Sample_Hash" };  // apvts.state
inline static const String USING_FILE_REFERENCE{ "Using_File_Reference" };  // apvts.state, modified by the Editor
inline static const String RECENT_FILES{ "Recent_Files" };
inline static const String SAVED_DEVICE_SETTINGS{ "Saved_Device_Settings" };

inline static const bool USE_FILE_REFERENCE{ true };
inline static const int STORED_BITRATE{ 16 };
inline static const double MAX_FILE_SIZE{ 320000000.0 }; // in bits, 40MB

inline static const String UI_VIEW_START{ "UI_View_Start" };  // apvts.state, modified by SampleNavigator
inline static const String UI_VIEW_END{ "UI_View_Stop" };  // apvts.state, modified by SampleNavigator

// Sample ranges
inline static const String SAMPLE_START{ "Sample_Start" };  // apvts.state
inline static const String SAMPLE_END{ "Sample_End" };  // apvts.state
inline static const String LOOP_START{ "Loop_Start" };  // apvts.state
inline static const String LOOP_END{ "Loop_End" };  // apvts.state
    
// Sample playback
inline static const String IS_LOOPING{ "Is_Looping" };
inline static const String LOOPING_HAS_START{ "Looping_Has_Start" };
inline static const String LOOPING_HAS_END{ "Looping_Has_End" };

inline static const String PLAYBACK_MODE{ "Playback_Mode" };
inline static const StringArray PLAYBACK_MODE_LABELS{ "Pitch Shifting: Basic", "Pitch Shifting: Advanced" };  // for IDs and display

const enum PLAYBACK_MODES
{
    BASIC,
    ADVANCED
};

/** Returns an enum representation of a playback mode given a float (1 indexed) */
inline static const PLAYBACK_MODES getPlaybackMode(float value) { return static_cast<PluginParameters::PLAYBACK_MODES>(int(value)); }

/** Skipping anti-aliasing could be known as "Lo-fi mode" */
inline static const String SKIP_ANTIALIASING{ "Lofi_Pitching" };

inline static const String MASTER_GAIN{ "Master_Gain" };
inline static const String MONO_OUTPUT{ "Mono_Output" };

inline static const int NUM_VOICES{ 16 };
inline static const float A4_HZ{ 440 };

// some controls for advanced playback
inline static const String SPEED_FACTOR{ "Speed_Factor" };
inline static const String FORMANT_PRESERVED{ "Formant_Preserved" };

inline static const bool PREPROCESS_STEP{ true };
inline static const bool PREPROCESS_RELEASE_BUFFER{ false };
inline static const bool DO_START_STOP_SMOOTHING{ true };
inline static const bool DO_CROSSFADE_SMOOTHING{ true };
inline static const int START_STOP_SMOOTHING{ 600 };  // This will be broken up into an attack and release time
inline static const int CROSSFADE_SMOOTHING{ 1500 };  // This will be user-customizable
    
inline static const String SEMITONE_TUNING{ "Semitone_Tuning" };
inline static const String CENT_TUNING{ "Cent_Tuning" };

// FX parameters
inline static const String REVERB_ENABLED{ "Reverb_Enabled" };
inline static const String REVERB_MIX{ "Reverb_Mix" };
inline static const String REVERB_SIZE{ "Reverb_Size" };
inline static const Range REVERB_SIZE_RANGE{ 5.f, 100.f}; 
inline static const String REVERB_DAMPING{ "Reverb_Damping" };
inline static const Range REVERB_DAMPING_RANGE{ 0.f, 95.f }; 
inline static const String REVERB_LOWS{ "Reverb_Lows" };  // These controls map to filters built into Gin's SimpleVerb
inline static const Range REVERB_LOWS_RANGE{ 0.f, 1.f };
inline static const String REVERB_HIGHS{ "Reverb_Highs" };
inline static const Range REVERB_HIGHS_RANGE{ 0.f, 1.f };
inline static const String REVERB_PREDELAY{ "Reverb_Predelay" };

inline static const String DISTORTION_ENABLED{ "Distortion_Enabled" };
inline static const String DISTORTION_DENSITY{ "Distortion_Density" };
inline static const Range DISTORTION_DENSITY_RANGE{ -0.5f, 1.f };
inline static const String DISTORTION_HIGHPASS{ "Distortion_Highpass" };
inline static const Range DISTORTION_HIGHPASS_RANGE{ 0.f, 0.999f };
inline static const String DISTORTION_MIX{ "Distortion_Mix" };
inline static const Range DISTORTION_MIX_RANGE{ 0.f, 1.f };

inline static const String EQ_ENABLED{ "EQ_Enabled" };
inline static const String EQ_LOW_GAIN{ "EQ_Low_Gain" };
inline static const Range EQ_LOW_GAIN_RANGE{ -12.f, 12.f }; // decibels
inline static const String EQ_MID_GAIN{ "EQ_Mid_Gain" };
inline static const Range EQ_MID_GAIN_RANGE{ -12.f, 12.f };
inline static const String EQ_HIGH_GAIN{ "EQ_High_Gain" };
inline static const Range EQ_HIGH_GAIN_RANGE{ -12.f, 12.f };
inline static const String EQ_LOW_FREQ{ "EQ_Low_Freq" };
inline static const Range EQ_LOW_FREQ_RANGE{ 25.f, 600.f };
inline static const String EQ_HIGH_FREQ{ "EQ_High_Freq" };
inline static const Range EQ_HIGH_FREQ_RANGE{ 700.f, 15500.f };

inline static const String CHORUS_ENABLED{ "Chorus_Enabled" };
inline static const String CHORUS_RATE{ "Chorus_Rate" }; 
inline static const NormalisableRange CHORUS_RATE_RANGE{ 0.1f, 20.f, 0.1f, 0.7f }; // in hz, upper range could be extended to 100hz
inline static const String CHORUS_DEPTH{ "Chorus_Depth" }; 
inline static const NormalisableRange CHORUS_DEPTH_RANGE{ 0.01f, 1.f, 0.01f, 0.5f };
inline static const String CHORUS_FEEDBACK{ "Chorus_Feedback" }; 
inline static const Range CHORUS_FEEDBACK_RANGE{ -0.95f, 0.95f };
inline static const String CHORUS_CENTER_DELAY{ "Chorus_Center_Delay" };
inline static const Range CHORUS_CENTER_DELAY_RANGE{ 1.f, 99.9f }; // in ms
inline static const String CHORUS_MIX{ "Chorus_Mix" };
inline static const Range CHORUS_MIX_RANGE{ 0.f, 1.f };

inline static const String FX_PERM{ "Fx_Perm" };

const enum FxTypes
{
    DISTORTION,
    CHORUS,
    REVERB,
    EQ
};

inline static const bool FX_TAIL_OFF{ true };
inline static const float FX_TAIL_OFF_MAX{ 0.0001f };

/** Supported reverb types */
const enum REVERB_TYPES
{
    JUCE,
    GIN_SIMPLE,
    GIN_PLATE
};

inline static const REVERB_TYPES REVERB_TYPE{ GIN_SIMPLE };

//==============================================================================
/** Returns a permutation of FxTypes, given a representative integer */
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

/** Returns a representative integer, given a permutation of FxTypes */
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

//==============================================================================
inline void addInt(juce::AudioProcessorValueTreeState::ParameterLayout& layout, const juce::String& identifier, int defaultValue, juce::NormalisableRange<int> range) { layout.add(std::make_unique<juce::AudioParameterInt>(identifier, identifier, range.start, range.end, defaultValue)); };
inline void addFloat(juce::AudioProcessorValueTreeState::ParameterLayout& layout, const juce::String& identifier, float defaultValue, juce::NormalisableRange<float> range) { layout.add(std::make_unique<juce::AudioParameterFloat>(identifier, identifier, range, defaultValue)); };
inline void addBool(juce::AudioProcessorValueTreeState::ParameterLayout& layout, const juce::String& identifier, bool defaultValue) { layout.add(std::make_unique<juce::AudioParameterBool>(identifier, identifier, defaultValue)); };
inline void addChoice(juce::AudioProcessorValueTreeState::ParameterLayout& layout, const juce::String& identifier, int defaultIndex, const juce::StringArray& choicesToUse) { layout.add(std::make_unique<juce::AudioParameterChoice>(identifier, identifier, choicesToUse, defaultIndex)); };

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    addChoice(layout, PluginParameters::PLAYBACK_MODE, 1, PluginParameters::PLAYBACK_MODE_LABELS);
    addBool(layout, PluginParameters::SKIP_ANTIALIASING, false);
    addBool(layout, PluginParameters::IS_LOOPING, false);
    addFloat(layout, PluginParameters::MASTER_GAIN, 0.f, { -15.f, 15.f, 0.1f, 0.5f, true });
    addInt(layout, PluginParameters::SEMITONE_TUNING, 0, { -12, 12 });
    addInt(layout, PluginParameters::CENT_TUNING, 0, { -100, 100 });
    addInt(layout, PluginParameters::FX_PERM, PluginParameters::permToParam({ PluginParameters::DISTORTION, PluginParameters::CHORUS, PluginParameters::REVERB, PluginParameters::EQ }), {0, 23});
    addBool(layout, PluginParameters::MONO_OUTPUT, false);
    addFloat(layout, PluginParameters::SPEED_FACTOR, 1.f, { 0.2f, 5.f, 0.01f, 0.3f });
    addBool(layout, PluginParameters::FORMANT_PRESERVED, false);

    addBool(layout, PluginParameters::REVERB_ENABLED, false);
    addFloat(layout, PluginParameters::REVERB_MIX, 0.5f, { 0.f, 1.f });
    addFloat(layout, PluginParameters::REVERB_SIZE, 0.5f, PluginParameters::REVERB_SIZE_RANGE);
    addFloat(layout, PluginParameters::REVERB_DAMPING, 0.5f, PluginParameters::REVERB_DAMPING_RANGE);
    addFloat(layout, PluginParameters::REVERB_LOWS, 0.5f, PluginParameters::REVERB_LOWS_RANGE);
    addFloat(layout, PluginParameters::REVERB_HIGHS, 0.5f, PluginParameters::REVERB_HIGHS_RANGE);
    addFloat(layout, PluginParameters::REVERB_PREDELAY, 0.5f, { 0.f, 500.f, 0.1f, 0.5f });  // in milliseconds

    addBool(layout, PluginParameters::DISTORTION_ENABLED, false);
    addFloat(layout, PluginParameters::DISTORTION_MIX, 1.f, PluginParameters::DISTORTION_MIX_RANGE);
    addFloat(layout, PluginParameters::DISTORTION_HIGHPASS, 0.f, PluginParameters::DISTORTION_HIGHPASS_RANGE);
    addFloat(layout, PluginParameters::DISTORTION_DENSITY, 0.f, PluginParameters::DISTORTION_DENSITY_RANGE);

    addBool(layout, PluginParameters::EQ_ENABLED, false);
    addFloat(layout, PluginParameters::EQ_LOW_GAIN, 0.f, PluginParameters::EQ_LOW_GAIN_RANGE);
    addFloat(layout, PluginParameters::EQ_MID_GAIN, 0.f, PluginParameters::EQ_MID_GAIN_RANGE);
    addFloat(layout, PluginParameters::EQ_HIGH_GAIN, 0.f, PluginParameters::EQ_HIGH_GAIN_RANGE);
    addFloat(layout, PluginParameters::EQ_LOW_FREQ, 200.f, PluginParameters::EQ_LOW_FREQ_RANGE);
    addFloat(layout, PluginParameters::EQ_HIGH_FREQ, 2000.f, PluginParameters::EQ_HIGH_FREQ_RANGE);

    addBool(layout, PluginParameters::CHORUS_ENABLED, false);
    addFloat(layout, PluginParameters::CHORUS_RATE, 1.f, PluginParameters::CHORUS_RATE_RANGE);
    addFloat(layout, PluginParameters::CHORUS_DEPTH, 0.25f, PluginParameters::CHORUS_DEPTH_RANGE);
    addFloat(layout, PluginParameters::CHORUS_FEEDBACK, 0.f, PluginParameters::CHORUS_FEEDBACK_RANGE);
    addFloat(layout, PluginParameters::CHORUS_CENTER_DELAY, 7.f, PluginParameters::CHORUS_CENTER_DELAY_RANGE);
    addFloat(layout, PluginParameters::CHORUS_MIX, 0.5f, PluginParameters::CHORUS_MIX_RANGE);

    return layout;
}
}  // namespace PluginParameters
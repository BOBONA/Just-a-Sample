/*
  ==============================================================================

    PluginParameters.h
    Created: 4 Oct 2023 10:40:47am
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "Utilities/ListenableValue.h"

/** This namespace contains all APVTS parameter IDs, the other plugin state, various plugin configuration settings, and the parameter layout */
namespace PluginParameters
{
using String = juce::String;
using StringArray = juce::StringArray;
using NormalisableRange = juce::NormalisableRange<float>;
using Range = juce::Range<float>;

/** Stores the non-parameter state of the plugin */
struct State
{
    ListenableAtomic<int> width{ 0 };
    inline static const String WIDTH{ "Width" };
    ListenableAtomic<int> height{ 0 };
    inline static const String HEIGHT{ "Height" };

    ListenableMutex<String> filePath{ "" };
    inline static const String FILE_PATH{ "File_Path" };
    ListenableMutex<String> sampleHash{ "" };
    inline static const String SAMPLE_HASH{ "Sample_Hash" };
    ListenableAtomic<bool> usingFileReference{ false };
    inline static const String USING_FILE_REFERENCE{ "Using_File_Reference" };
    ListenableMutex<StringArray> recentFiles;
    inline static const String RECENT_FILES{ "Recent_Files" };
    inline static const String SAVED_DEVICE_SETTINGS{ "Saved_Device_Settings" };

    ListenableAtomic<int> viewStart{ 0 };
    inline static const String UI_VIEW_START{ "UI_View_Start" };
    ListenableAtomic<int> viewEnd{ 0 };
    inline static const String UI_VIEW_END{ "UI_View_End" };

    ListenableAtomic<int> sampleStart{ 0 };
    inline static const String SAMPLE_START{ "Sample_Start" };
    ListenableAtomic<int> sampleEnd{ 0 };
    inline static const String SAMPLE_END{ "Sample_End" };
    ListenableAtomic<int> loopStart{ 0 };
    inline static const String LOOP_START{ "Loop_Start" };
    ListenableAtomic<int> loopEnd{ 0 };
    inline static const String LOOP_END{ "Loop_End" };
};

// Layout
inline static constexpr int FRAME_RATE{ 60 };

// Sample storage
inline static constexpr bool USE_FILE_REFERENCE{ true };
inline static constexpr int STORED_BITRATE{ 16 };
inline static constexpr double MAX_FILE_SIZE{ 320000000.0 }; // in bits, 40MB
    
// Sample playback
inline static const String IS_LOOPING{ "Loop" };
inline static const String LOOPING_HAS_START{ "Loop With Start" };
inline static const String LOOPING_HAS_END{ "Loop With End" };

inline static const String PLAYBACK_MODE{ "Playback Mode" };
inline static const StringArray PLAYBACK_MODE_LABELS{ "Basic", "Advanced" };  // for IDs and display

enum PLAYBACK_MODES
{
    BASIC,
    ADVANCED,
};

/** Returns an enum representation of a playback mode given a float */
static PLAYBACK_MODES getPlaybackMode(int value) { return static_cast<PluginParameters::PLAYBACK_MODES>(value); }

/** Skipping antialiasing could be known as "Lo-fi mode" */
inline static const String SKIP_ANTIALIASING{ "Lo-fi Resampling (Basic only)" };

inline static const String MASTER_GAIN{ "Master Gain" };
inline static const String MONO_OUTPUT{ "Mono Output" };

inline static constexpr int NUM_VOICES{ 32 };
inline static constexpr float A4_HZ{ 440 };
inline static constexpr float WAVETABLE_CUTOFF_HZ{ 20 };  // The cutoff frequency for "wavetable mode"

// Some controls for advanced playback
inline static const String SPEED_FACTOR{ "Playback Speed (Advanced only)" };
inline static const String OCTAVE_SPEED_FACTOR{ "Octave Speed Factor (Advanced)" };

inline static const String ATTACK{ "Attack Time" };
inline static const String RELEASE{ "Release Time" };
inline static constexpr int MIN_SMOOTHING_SAMPLES{ 50 };
inline static constexpr int CROSSFADING{ 1000 };
    
inline static const String SEMITONE_TUNING{ "Semitone Tuning" };
inline static const String CENT_TUNING{ "Cent Tuning" };

// FX parameters
inline static const String REVERB_ENABLED{ "Reverb: Enabled" };
inline static const String REVERB_MIX{ "Reverb: Mix" };
inline static const String REVERB_SIZE{ "Reverb: Size" };
inline static constexpr Range REVERB_SIZE_RANGE{ 5.f, 100.f}; 
inline static const String REVERB_DAMPING{ "Reverb: Damping" };
inline static constexpr Range REVERB_DAMPING_RANGE{ 0.f, 95.f }; 
inline static const String REVERB_LOWS{ "Reverb: Lows" };  // These controls map to filters built into Gin's SimpleVerb
inline static constexpr Range REVERB_LOWS_RANGE{ 0.f, 1.f };
inline static const String REVERB_HIGHS{ "Reverb: Highs" };
inline static constexpr Range REVERB_HIGHS_RANGE{ 0.f, 1.f };
inline static const String REVERB_PREDELAY{ "Reverb: Predelay" };

inline static const String DISTORTION_ENABLED{ "Distortion: Enabled" };
inline static const String DISTORTION_DENSITY{ "Distortion: Density" };
inline static constexpr Range DISTORTION_DENSITY_RANGE{ -0.5f, 1.f };
inline static const String DISTORTION_HIGHPASS{ "Distortion: Highpass" };
inline static constexpr Range DISTORTION_HIGHPASS_RANGE{ 0.f, 0.999f };
inline static const String DISTORTION_MIX{ "Distortion: Mix" };
inline static constexpr Range DISTORTION_MIX_RANGE{ 0.f, 1.f };

inline static const String EQ_ENABLED{ "EQ: Enabled" };
inline static const String EQ_LOW_GAIN{ "EQ: Low Gain" };
inline static constexpr Range EQ_LOW_GAIN_RANGE{ -12.f, 12.f };  // decibels
inline static const String EQ_MID_GAIN{ "EQ: Mid Gain" };
inline static constexpr Range EQ_MID_GAIN_RANGE{ -12.f, 12.f };
inline static const String EQ_HIGH_GAIN{ "EQ: High Gain" };
inline static constexpr Range EQ_HIGH_GAIN_RANGE{ -12.f, 12.f };
inline static const String EQ_LOW_FREQ{ "EQ: Low Cutoff" };
inline static constexpr Range EQ_LOW_FREQ_RANGE{ 25.f, 600.f };
inline static const String EQ_HIGH_FREQ{ "EQ: High Cutoff" };
inline static constexpr Range EQ_HIGH_FREQ_RANGE{ 700.f, 15500.f };

inline static const String CHORUS_ENABLED{ "Chorus: Enabled" };
inline static const String CHORUS_RATE{ "Chorus: Rate" }; 
inline static const NormalisableRange CHORUS_RATE_RANGE{ 0.1f, 20.f, 0.1f, 0.7f };  // in hz, upper range could be extended to 100hz
inline static const String CHORUS_DEPTH{ "Chorus: Depth" }; 
inline static const NormalisableRange CHORUS_DEPTH_RANGE{ 0.01f, 1.f, 0.01f, 0.5f };
inline static const String CHORUS_FEEDBACK{ "Chorus: Feedback" }; 
inline static constexpr Range CHORUS_FEEDBACK_RANGE{ -0.95f, 0.95f };
inline static const String CHORUS_CENTER_DELAY{ "Chorus: Center Delay" };
inline static constexpr Range CHORUS_CENTER_DELAY_RANGE{ 1.f, 99.9f }; // in ms
inline static const String CHORUS_MIX{ "Chorus: Mix" };
inline static constexpr Range CHORUS_MIX_RANGE{ 0.f, 1.f };

inline static const String FX_PERM{ "FX Ordering" };

enum FxTypes
{
    DISTORTION,
    CHORUS,
    REVERB,
    EQ
};

inline static constexpr bool FX_TAIL_OFF{ true };
inline static constexpr float FX_TAIL_OFF_MAX{ 0.0001f };  // The cutoff RMS value for tailing off effects

/** Supported reverb types */
enum REVERB_TYPES
{
    JUCE,
    GIN_SIMPLE,
    GIN_PLATE
};

inline static constexpr REVERB_TYPES REVERB_TYPE{ GIN_SIMPLE };

//==============================================================================
/** Returns a permutation of FxTypes, given a representative integer */
static std::array<FxTypes, 4> paramToPerm(int fxParam)
{
    std::array types{ DISTORTION, CHORUS, REVERB, EQ };
    std::array<FxTypes, 4> perm{};
    int factorial = 6;  // 3!
    for (int i = 0; i < 4; i++)
    {
        int index = fxParam / factorial;
        perm[i] = types[index];
        for (int j = index; j < 3; j++)
            types[j] = types[j + 1];
        fxParam %= factorial;
        if (i < 3)
            factorial /= 3 - i;
    }
    return perm;
}

/** Returns a representative integer, given a permutation of FxTypes */
static int permToParam(std::array<FxTypes, 4> fxPerm)
{
    std::array types{ DISTORTION, CHORUS, REVERB, EQ };
    int result = 0;
    int factorial = 6;  // 3!
    for (int i = 0; i < 3; i++)
    {
        int type = fxPerm[i];
        int index;
        for (index = 0; index < types.size(); index++)
            if (type == types[index])
                break;
        result += factorial * index;
        for (int j = index; j < 3 - i; j++)
            types[j] = types[j + 1];
        if (i < 3)
            factorial /= 3 - i;
    }
    return result;
}

//==============================================================================
inline void addInt(juce::AudioProcessorValueTreeState::ParameterLayout& layout, const juce::String& identifier, int defaultValue, const juce::NormalisableRange<int>& range) { layout.add(std::make_unique<juce::AudioParameterInt>(identifier, identifier, range.start, range.end, defaultValue)); };
inline void addFloat(juce::AudioProcessorValueTreeState::ParameterLayout& layout, const juce::String& identifier, float defaultValue, const juce::NormalisableRange<float>& range) { layout.add(std::make_unique<juce::AudioParameterFloat>(identifier, identifier, range, defaultValue)); };
inline void addBool(juce::AudioProcessorValueTreeState::ParameterLayout& layout, const juce::String& identifier, bool defaultValue) { layout.add(std::make_unique<juce::AudioParameterBool>(identifier, identifier, defaultValue)); };
inline void addChoice(juce::AudioProcessorValueTreeState::ParameterLayout& layout, const juce::String& identifier, int defaultIndex, const juce::StringArray& choicesToUse) { layout.add(std::make_unique<juce::AudioParameterChoice>(identifier, identifier, choicesToUse, defaultIndex)); };

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    addChoice(layout, PluginParameters::PLAYBACK_MODE, 1, PluginParameters::PLAYBACK_MODE_LABELS);
    addBool(layout, PluginParameters::SKIP_ANTIALIASING, false);
    addBool(layout, PluginParameters::IS_LOOPING, false);
    addBool(layout, PluginParameters::LOOPING_HAS_START, false);
    addBool(layout, PluginParameters::LOOPING_HAS_END, false);
    addFloat(layout, PluginParameters::MASTER_GAIN, 0.f, { -15.f, 15.f, 0.1f, 0.5f, true });
    addInt(layout, PluginParameters::SEMITONE_TUNING, 0, { -12, 12 });
    addInt(layout, PluginParameters::CENT_TUNING, 0, { -100, 100 });
    addInt(layout, PluginParameters::FX_PERM, PluginParameters::permToParam({ PluginParameters::DISTORTION, PluginParameters::CHORUS, PluginParameters::REVERB, PluginParameters::EQ }), {0, 23});
    addBool(layout, PluginParameters::MONO_OUTPUT, false);
    addFloat(layout, PluginParameters::SPEED_FACTOR, 1.f, { 0.2f, 5.f, 0.01f, 0.3f });
    addFloat(layout, PluginParameters::OCTAVE_SPEED_FACTOR, 0.f, { 0.f, 0.6f, 0.15f });
    addInt(layout, PluginParameters::ATTACK, 0, { 0, 5000 });
    addInt(layout, PluginParameters::RELEASE, 0, {0, 5000});

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
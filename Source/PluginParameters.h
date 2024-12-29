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
    inline static const String FILE_PATH{ "File Path" };
    ListenableMutex<String> sampleHash{ "" };
    inline static const String SAMPLE_HASH{ "Sample Hash" };
    ListenableAtomic<bool> usingFileReference{ false };
    inline static const String USING_FILE_REFERENCE{ "Using File Reference" };
    ListenableMutex<StringArray> recentFiles;
    inline static const String RECENT_FILES{ "Recent Files" };
    inline static const String SAVED_DEVICE_SETTINGS{ "Saved Device Settings" };

    ListenableAtomic<int> viewStart{ 0 };
    inline static const String UI_VIEW_START{ "UI View Start" };
    ListenableAtomic<int> viewEnd{ 0 };
    inline static const String UI_VIEW_END{ "UI View End" };
    ListenableAtomic<bool> pinView{ false };
    inline static const String PIN_VIEW{ "Pin View" };

    ListenableAtomic<int> sampleStart{ 0 };
    inline static const String SAMPLE_START{ "Sample Start" };
    ListenableAtomic<int> sampleEnd{ 0 };
    inline static const String SAMPLE_END{ "Sample End" };
    ListenableAtomic<int> loopStart{ 0 };
    inline static const String LOOP_START{ "Loop Start" };
    ListenableAtomic<int> loopEnd{ 0 };
    inline static const String LOOP_END{ "Loop End" };

    ListenableAtomic<bool> showFX{ false };
    inline static const String SHOW_FX{ "Show FX" };

    inline static const String UI_DUMMY_PARAM{ "UI Update" };
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
inline static const StringArray PLAYBACK_MODE_LABELS{ "Basic", "Bungee" };  // for IDs and display

enum PLAYBACK_MODES : std::uint8_t
{
    BASIC,
    BUNGEE,
};

/** Returns an enum representation of a playback mode given a float */
inline PLAYBACK_MODES getPlaybackMode(int value) { return static_cast<PLAYBACK_MODES>(value); }

/** Skipping antialiasing can be an interesting effect */
inline static const String SKIP_ANTIALIASING{ "Lo-fi Resampling" };

inline static const String SAMPLE_GAIN{ "Sample Gain" };
inline static const String MONO_OUTPUT{ "Mono Output" };

inline static constexpr int NUM_VOICES{ 32 };
inline static constexpr float A4_HZ{ 440 };
inline static constexpr float WAVETABLE_CUTOFF_HZ{ 20 };  // The cutoff frequency for "wavetable mode"

// Some controls for advanced playback
inline static const String SPEED_FACTOR{ "Playback Speed" };
inline static const String OCTAVE_SPEED_FACTOR{ "Octave Speed Factor" };

inline static const String ATTACK{ "Attack Time" };
inline static const String RELEASE{ "Release Time" };
inline static const String ATTACK_SHAPE{ "Attack Curve Shape" };
inline static const String RELEASE_SHAPE{ "Release Curve Shape" };
inline static constexpr int MIN_SMOOTHING_SAMPLES{ 50 };
inline static constexpr int CROSSFADING{ 1000 };
    
inline static const String SEMITONE_TUNING{ "Semitone Tuning" };
inline static const String CENT_TUNING{ "Cent Tuning" };

inline static const String WAVEFORM_SEMITONE_TUNING{ "Waveform Semitone Tuning" };
inline static const String WAVEFORM_CENT_TUNING{ "Waveform Cent Tuning" };

// FX parameters
inline static const String REVERB_ENABLED{ "Reverb Enabled" };
inline static const String REVERB_MIX{ "Reverb Mix" };
inline static const String REVERB_SIZE{ "Reverb Size" };
inline static constexpr Range REVERB_SIZE_RANGE{ 5.f, 100.f}; 
inline static const String REVERB_DAMPING{ "Reverb Damping" };
inline static constexpr Range REVERB_DAMPING_RANGE{ 0.f, 95.f }; 
inline static const String REVERB_LOWS{ "Reverb Lows" };  // These controls map to filters built into Gin's SimpleVerb
inline static constexpr Range REVERB_LOWS_RANGE{ 0.f, 1.f };
inline static const String REVERB_HIGHS{ "Reverb Highs" };
inline static constexpr Range REVERB_HIGHS_RANGE{ 0.f, 1.f };
inline static const String REVERB_PREDELAY{ "Reverb Predelay" };

inline static const String DISTORTION_ENABLED{ "Distortion Enabled" };
inline static const String DISTORTION_DENSITY{ "Distortion Density" };
inline static constexpr Range DISTORTION_DENSITY_RANGE{ -0.5f, 1.f };
inline static const String DISTORTION_HIGHPASS{ "Distortion Highpass" };
inline static constexpr Range DISTORTION_HIGHPASS_RANGE{ 0.f, 0.999f };
inline static const String DISTORTION_MIX{ "Distortion Mix" };

inline static const String EQ_ENABLED{ "EQ Enabled" };
inline static const String EQ_LOW_GAIN{ "EQ Low Gain" };
inline static const NormalisableRange EQ_GAIN_RANGE{ -12.f, 12.f, 0.1f };  // Decibels
inline static const String EQ_MID_GAIN{ "EQ Mid Gain" };
inline static const String EQ_HIGH_GAIN{ "EQ High Gain" };
inline static const String EQ_LOW_FREQ{ "EQ Low Cutoff" };
inline static constexpr Range EQ_LOW_FREQ_RANGE{ 25.f, 600.f };
inline static constexpr float EQ_LOW_FREQ_DEFAULT{ 200.f };
inline static const String EQ_HIGH_FREQ{ "EQ High Cutoff" };
inline static constexpr Range EQ_HIGH_FREQ_RANGE{ 700.f, 15500.f };
inline static constexpr float EQ_HIGH_FREQ_DEFAULT{ 2000.f };

inline static const String CHORUS_ENABLED{ "Chorus Enabled" };
inline static const String CHORUS_RATE{ "Chorus Rate" }; 
inline static const NormalisableRange CHORUS_RATE_RANGE{ 0.1f, 20.f, 0.1f, 0.7f };  // in hz, upper range could be extended to 100hz
inline static const String CHORUS_DEPTH{ "Chorus Depth" }; 
inline static const NormalisableRange CHORUS_DEPTH_RANGE{ 0.01f, 1.f, 0.01f, 0.5f };
inline static const String CHORUS_FEEDBACK{ "Chorus Feedback" }; 
inline static constexpr Range CHORUS_FEEDBACK_RANGE{ -0.95f, 0.95f };
inline static const String CHORUS_CENTER_DELAY{ "Chorus Center Delay" };
inline static constexpr Range CHORUS_CENTER_DELAY_RANGE{ 0.f, 100.f }; // in ms
inline static const String CHORUS_MIX{ "Chorus Mix" };

inline static const String FX_PERM{ "FX Ordering" };

enum FxTypes : std::uint8_t
{
    DISTORTION,
    CHORUS,
    REVERB,
    EQ
};

inline static const String PRE_FX{ "FX Before Envelope" };
inline static constexpr float FX_TAIL_OFF_MAX{ 0.0001f };  // The cutoff RMS value for tailing off effects

//==============================================================================
/** Returns a permutation of FxTypes, given a representative integer */
inline std::array<FxTypes, 4> paramToPerm(int fxParam)
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
inline void addInt(juce::AudioProcessorValueTreeState::ParameterLayout& layout, const juce::String& identifier, int defaultValue, const juce::NormalisableRange<int>& range) { layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{ identifier, JUCE_VERSION }, identifier, range.start, range.end, defaultValue)); };
inline void addFloat(juce::AudioProcessorValueTreeState::ParameterLayout& layout, const juce::String& identifier, float defaultValue, const juce::NormalisableRange<float>& range) { layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ identifier, JUCE_VERSION }, identifier, range, defaultValue)); };
inline void addBool(juce::AudioProcessorValueTreeState::ParameterLayout& layout, const juce::String& identifier, bool defaultValue) { layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{ identifier, JUCE_VERSION }, identifier, defaultValue)); };
inline void addChoice(juce::AudioProcessorValueTreeState::ParameterLayout& layout, const juce::String& identifier, int defaultIndex, const juce::StringArray& choicesToUse) { layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ identifier, JUCE_VERSION }, identifier, choicesToUse, defaultIndex)); };

template <typename T> juce::NormalisableRange<T> addSkew(juce::NormalisableRange<T> range, T skewCenter)
{
    auto r = range.getRange();
    range.skew = std::log(0.5f) / std::log((float(skewCenter) - r.getStart()) / r.getLength());
    return range;
}

/** This inverts the proportions, such that "increasing" a slider value will decrease the parameter value */
template <typename T> juce::NormalisableRange<T> invertProportions(juce::NormalisableRange<T> range)
{
    auto convertFrom0To1Function = [](T rangeStart, T rangeEnd, T normalised) { return juce::jmap<float>(normalised, rangeEnd, rangeStart); };
    auto convertTo0From1Function = [](T rangeStart, T rangeEnd, T value) { return juce::jmap<float>(value, rangeEnd, rangeStart, 0.0f, 1.0f); };
    auto newRange = juce::NormalisableRange<T>{ range.start, range.end, convertFrom0To1Function, convertTo0From1Function };
    newRange.interval = range.interval;
    return newRange;
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    addInt(layout, SEMITONE_TUNING, 0, { -12, 12 });
    addInt(layout, CENT_TUNING, 0, { -100, 100 });
    addInt(layout, WAVEFORM_SEMITONE_TUNING, 0, { -12, 12 });
    addInt(layout, WAVEFORM_CENT_TUNING, 0, { -100, 100 });

    addBool(layout, SKIP_ANTIALIASING, false);
    addChoice(layout, PLAYBACK_MODE, 0, PLAYBACK_MODE_LABELS);
    addFloat(layout, SPEED_FACTOR, 1.f, addSkew({ 0.01f, 5.f, 0.01f }, 1.f));
    addFloat(layout, OCTAVE_SPEED_FACTOR, 0.f, { 0.f, 0.6f, 0.15f });

    addBool(layout, LOOPING_HAS_START, false);
    addBool(layout, IS_LOOPING, false);
    addBool(layout, LOOPING_HAS_END, false);

    addFloat(layout, SAMPLE_GAIN, 0.f, addSkew({ -32.f, 16.f, 0.1f }, 0.f));
    addBool(layout, MONO_OUTPUT, false);

    addInt(layout, FX_PERM, permToParam({ DISTORTION, CHORUS, REVERB, EQ }), { 0, 23 });
    addBool(layout, PRE_FX, false);

    addFloat(layout, ATTACK, 0, addSkew({ 0.f, 5000.f, 1.f }, 1000.f));
    addFloat(layout, RELEASE, 0, addSkew({ 0.f, 5000.f, 1.f }, 1000.f));
    addFloat(layout, ATTACK_SHAPE, 0.f, invertProportions(NormalisableRange{ -10.f, 10.f, 0.1f }));
    addFloat(layout, RELEASE_SHAPE, 2.f, { -10.f, 10.f, 0.1f });

    addBool(layout, REVERB_ENABLED, false);
    addFloat(layout, REVERB_MIX, 0.5f, { 0.f, 1.f, 0.01f });
    addFloat(layout, REVERB_SIZE, 0.5f, { REVERB_SIZE_RANGE, 1.f });
    addFloat(layout, REVERB_DAMPING, 0.5f, { REVERB_DAMPING_RANGE, 1.f });
    addFloat(layout, REVERB_LOWS, 0.5f, { REVERB_LOWS_RANGE, 0.01f });
    addFloat(layout, REVERB_HIGHS, 0.5f, { REVERB_HIGHS_RANGE, 0.01f });
    addFloat(layout, REVERB_PREDELAY, 0.5f, { 0.f, 500.f, 1.f, 0.5f });  // in milliseconds

    addBool(layout, DISTORTION_ENABLED, false);
    addFloat(layout, DISTORTION_MIX, 1.f, { 0.f, 1.f, 0.01f });
    addFloat(layout, DISTORTION_HIGHPASS, 0.f, { DISTORTION_HIGHPASS_RANGE, 0.01f });
    addFloat(layout, DISTORTION_DENSITY, 0.f, addSkew({ DISTORTION_DENSITY_RANGE, 0.01f }, 0.f));

    addBool(layout, EQ_ENABLED, false);
    addFloat(layout, EQ_LOW_GAIN, 0.f, EQ_GAIN_RANGE);
    addFloat(layout, EQ_MID_GAIN, 0.f, EQ_GAIN_RANGE);
    addFloat(layout, EQ_HIGH_GAIN, 0.f, EQ_GAIN_RANGE);
    addFloat(layout, EQ_LOW_FREQ, EQ_LOW_FREQ_DEFAULT, EQ_LOW_FREQ_RANGE);
    addFloat(layout, EQ_HIGH_FREQ, EQ_HIGH_FREQ_DEFAULT, EQ_HIGH_FREQ_RANGE);

    addBool(layout, CHORUS_ENABLED, false);
    addFloat(layout, CHORUS_RATE, 1.f, CHORUS_RATE_RANGE);
    addFloat(layout, CHORUS_DEPTH, 0.25f, CHORUS_DEPTH_RANGE);
    addFloat(layout, CHORUS_FEEDBACK, 0.f, { CHORUS_FEEDBACK_RANGE, 0.01f });
    addFloat(layout, CHORUS_CENTER_DELAY, 7.f, { CHORUS_CENTER_DELAY_RANGE, 1.f });
    addFloat(layout, CHORUS_MIX, 0.5f, { 0.f, 1.f, 0.01f });

    addBool(layout, State::UI_DUMMY_PARAM, true);  // This is a dummy parameter to notify the host of state changes

    return layout;
}
}  // namespace PluginParameters

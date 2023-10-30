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
};
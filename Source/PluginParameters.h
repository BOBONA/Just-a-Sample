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
    inline static const juce::String FILE_PATH{ "File_Path" };
    inline static const juce::String UI_VIEW_START{ "UI_View_Start" };
    inline static const juce::String UI_VIEW_END{ "UI_View_Stop" };
    inline static const juce::String SAMPLE_START{ "Sample_Start" };
    inline static const juce::String SAMPLE_END{ "Sample_End" };
};
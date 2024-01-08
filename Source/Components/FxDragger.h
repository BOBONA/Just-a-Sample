/*
  ==============================================================================

    FxDragger.h
    Created: 7 Jan 2024 12:21:57pm
    Author:  binya

  ==============================================================================
*/

#pragma once

class FxDragger
{
public:
    virtual ~FxDragger() {};

    virtual void dragStarted(const juce::String& moduleName, const juce::MouseEvent& event) = 0;
    virtual void dragEnded() = 0;
};
/*
  ==============================================================================

    FxDragTarget.h
    Created: 7 Jan 2024 12:21:57pm
    Author:  binya

  ==============================================================================
*/

#pragma once

/** This interface mostly exists separately to avoid some circular reference problems */
class FxDragTarget
{
public:
    virtual ~FxDragTarget() {}

    virtual void dragStarted(const juce::String& moduleName, const juce::MouseEvent& event) = 0;
    virtual void dragEnded() = 0;
};
/*
  ==============================================================================

    CustomComponent.h
    Created: 26 Sep 2023 11:49:53pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "../CustomLookAndFeel.h"
#include "../PluginParameters.h"

using APVTS = juce::AudioProcessorValueTreeState;

class CustomComponent : public juce::Component
{
public:
    CustomComponent() : lnf(dynamic_cast<CustomLookAndFeel&>(getLookAndFeel()))
    {

    }

    /** Easy way to get a disabled version of a color */
    juce::Colour disabled(juce::Colour color, bool disabledCondition) const
    {
        return disabledCondition ? (lnf.DISABLED.withBrightness((lnf.DISABLED.getBrightness() + color.getPerceivedBrightness()) / 2)) : color;
    }

    /** Easy way to get a disabled version of a color */
    juce::Colour disabled(juce::Colour color) const
    {
        return disabled(color, !isEnabled());
    }

    CustomLookAndFeel& lnf;
};

/** Some utility methods for dealing with different parts on a custom component. T is intended to be an enum of different parts of the component where the first enum value is NONE */
template <typename T>
struct CompPart
{
    T part;
    Rectangle<float> area;
    int priority;

    CompPart(T part, Rectangle<float> area, int priority) : part(part), area(area), priority(priority)
    {}

    float distanceTo(int x, int y) const
    {
        float dx = juce::jmax<float>(area.getX() - x, 0, x - area.getRight());
        float dy = juce::jmax<float>(area.getY() - y, 0, y - area.getBottom());
        return std::sqrtf(dx * dx + dy * dy);
    }

    static T getClosestInRange(juce::Array<CompPart<T>> targets, int x, int y, int snapAmount=0)
    {
        T closest = static_cast<T>(0);
        auto priority = -1;
        auto closestDistance = INFINITY;
        for (const auto& p : targets)
        {
            auto distance = p.distanceTo(x, y);
            if ((distance < closestDistance || p.priority > priority) && distance <= snapAmount)
            {
                closest = p.part;
                priority = p.priority;
                closestDistance = distance;
            }
        }
        return closest;
    }
};

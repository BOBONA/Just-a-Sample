/*
  ==============================================================================

    CustomAudioDeviceSelector.h
    Created: 18 Aug 2024 8:20:19pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "../Utilities/ComponentUtils.h"

/** Unfortunately, JUCE does not allow adequate customization of this component.
    I ended up basically rewriting it because I was not satisfied with their implementation.
 */
class CustomAudioDeviceSelector final : public CustomComponent, juce::ChangeListener
{
public:
    CustomAudioDeviceSelector(juce::AudioDeviceManager& deviceManager, int minAudioInputChannels, int maxAudioInputChannels, bool showChannelsAsStereoPairs);
    ~CustomAudioDeviceSelector() override;

    /** Sets the standard height used for items in the panel. */
    void setItemHeight(int itemHeight);

    /** Returns the standard height used for items in the panel. */
    int getItemHeight() const noexcept { return itemHeight; }

private:
    void paint(juce::Graphics& g) override;
    void resized() override;
    void childBoundsChanged(Component* child) override;

    void updateDeviceType();
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    void updateAllControls();

    //==============================================================================
    juce::Label settingsLabel, deviceTypeLabel;
    juce::ComboBox deviceTypes;
    std::unique_ptr<Component> audioDeviceSettingsComp;

    juce::AudioDeviceManager& deviceManager;
    juce::String deviceType;

    int itemHeight = 0;
    const int minInputChannels, maxInputChannels;
    const bool showChannelsAsStereoPairs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomAudioDeviceSelector)
};
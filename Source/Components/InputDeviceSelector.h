/*
  ==============================================================================

    InputDeviceSelector.h
    Created: 18 Aug 2024 8:20:19pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "../Utilities/ComponentUtils.h"

/** A simple meter that displays the input level of the active audio device. */
struct DeviceLevelMeter final : CustomComponent, juce::Timer
{
    explicit DeviceLevelMeter(juce::AudioDeviceManager& m) : manager(m)
    {
        startTimerHz(20);
        inputLevelGetter = manager.getInputLevelGetter();
    }

    void timerCallback() override
    {
		if (!isVisible())
			return;
		
        auto newLevel = float(inputLevelGetter->getCurrentLevel());
        if (std::abs(level - newLevel) > 0.005f)
            level = newLevel;

        auto* device = manager.getCurrentAudioDevice();
        if (!device || !manager.getAudioDeviceSetup().inputChannels.countNumberOfSetBits())
            level = 0;

        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        // Add a bit of skew
        getLookAndFeel().drawLevelMeter(g, getWidth(), getHeight(), float(std::exp(std::log(level) / 3.0)));
    }

    juce::AudioDeviceManager& manager;
    juce::AudioDeviceManager::LevelMeter::Ptr inputLevelGetter;
    float level = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeviceLevelMeter)
};

/** A simple component that allows the user to select the input channels of the active audio device. */
class ChannelSelectorListBox final : public juce::ListBox, juce::ListBoxModel, public juce::ChangeListener
{
public:
    ChannelSelectorListBox(juce::AudioDeviceManager& audioDeviceManager, int minNumInputChannels, int maxNumInputChannels, bool useStereoPairs);
    ~ChannelSelectorListBox() override;

    void changeListenerCallback(juce::ChangeBroadcaster*) override;

    int getNumRows() override;
    void listBoxItemClicked(int row, const juce::MouseEvent& e) override;
    void paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool) override;

private:
    void flipEnablement(int row) const;
    void setBit(juce::BigInteger& ch, int index, bool set) const;
    juce::String getNameForChannelPair(const juce::String& name1, const juce::String& name2) const;

    //==============================================================================
    juce::AudioDeviceManager& manager;
    int minInputChannels, maxInputChannels;
    bool useStereoPairs;

    juce::AudioIODevice* device{ nullptr };
    juce::StringArray channels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelSelectorListBox)
};

/** A cleaner version of JUCE's AudioDeviceSelectorComponent */
class InputDeviceSelector final : public CustomComponent, public juce::ChangeListener
{
public:
    InputDeviceSelector(juce::AudioDeviceManager& deviceManager, int minInputChannelsToUse, int maxInputChannelsToUse, bool showChannelsAsStereoPairs);
    ~InputDeviceSelector() override;

private:
    void paint(juce::Graphics& g) override;
    void resized() override;
    void lookAndFeelChanged() override;

    /** Update the choosers once the device updates */
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    /** Update the device manager when a user selects something */
    void settingsChanged();

    void updateEnablement();

    //==============================================================================
    juce::AudioDeviceManager& manager;
    juce::AudioIODevice* inputDevice{ nullptr };

    juce::Label titleLabel, warningLabel, settingsLabel, deviceTypeLabel, deviceLabel, channelsLabel, sampleRateLabel, bufferSizeLabel;
    juce::ComboBox deviceTypeChooser, deviceChooser, sampleRateChooser, bufferSizeChooser;
    DeviceLevelMeter levelMeter;
    ChannelSelectorListBox channelSelector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InputDeviceSelector)
};
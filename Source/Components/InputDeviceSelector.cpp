/*
    ==============================================================================

    InputDeviceSelector.cpp
    Created: 18 Aug 2024 8:20:19pm
    Author:  binya

    ==============================================================================
*/

#include "InputDeviceSelector.h"

ChannelSelectorListBox::ChannelSelectorListBox(juce::AudioDeviceManager& audioDeviceManager, int minNumInputChannels, int maxNumInputChannels, bool useStereoPairs)
    : manager(audioDeviceManager), minInputChannels(minNumInputChannels), maxInputChannels(maxNumInputChannels), useStereoPairs(useStereoPairs)
{
    setModel(this);
    audioDeviceManager.addChangeListener(this);

    changeListenerCallback(nullptr);
}

ChannelSelectorListBox::~ChannelSelectorListBox()
{
    manager.removeChangeListener(this);
}

void ChannelSelectorListBox::changeListenerCallback(juce::ChangeBroadcaster*)
{
    repaint();

    auto* currentDevice = manager.getCurrentAudioDevice();

    if (device == currentDevice)
        return;

    channels.clear();

    if (!currentDevice)
        return;

    channels = currentDevice->getInputChannelNames();
    device = currentDevice;

    if (useStereoPairs)
    {
        juce::StringArray pairs;

        int numPairs = channels.size() / 2;
        for (int i = 0; i < numPairs; i++)
            pairs.add(getNameForChannelPair(channels[i * 2], channels[i * 2 + 1]));

        if (channels.size() % 2)
            pairs.add(channels.end()->trim());

        channels = pairs;
    }

    updateContent();
}

int ChannelSelectorListBox::getNumRows()
{
    return channels.size();
}

void ChannelSelectorListBox::listBoxItemClicked(int row, const juce::MouseEvent& e)
{
    if (row < channels.size())
        flipEnablement(row);
}

void ChannelSelectorListBox::paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool)
{
    using namespace juce;

    if (row >= channels.size())
        return;

    auto item = channels[row];
    auto inputChannels = manager.getAudioDeviceSetup().inputChannels;

    // We need to actually check the device for enablement
    bool enabled = useStereoPairs ? inputChannels[row * 2] || inputChannels[row * 2 + 1] : inputChannels[row];

    Rectangle<float> bounds(width, height);
    bounds = bounds.reduced(bounds.getHeight() * 0.04f);

    // To draw the tick box, we create a temporary ToggleButton (due to how I implemented the lnf function)
    ToggleButton tempComp;
    tempComp.setBounds(bounds.removeFromLeft(bounds.getHeight()).toNearestInt());
    getLookAndFeel().drawTickBox(g, tempComp, 0, 0, 0, 0, enabled, true, true, false);

    bounds.removeFromLeft(bounds.getHeight() * 0.3f);

    g.setColour(findColour(textColourId));
    g.setFont(getInter().withHeight(bounds.getHeight() * 0.85f));
    g.drawText(channels[row], bounds, Justification::centredLeft);
}

void ChannelSelectorListBox::flipEnablement(int row) const
{
    auto config = manager.getAudioDeviceSetup();
    config.useDefaultInputChannels = false;

    if (useStereoPairs)
    {
        bool enabled = config.inputChannels[row * 2] || config.inputChannels[row * 2 + 1];
        setBit(config.inputChannels, row * 2 + 1, !enabled);
        setBit(config.inputChannels, row * 2, !enabled);
    }
    else
    {
        setBit(config.inputChannels, row, !config.inputChannels[row]);
    }

    manager.setAudioDeviceSetup(config, true);
}

void ChannelSelectorListBox::setBit(juce::BigInteger& ch, int index, bool set) const
{
    auto numActive = ch.countNumberOfSetBits();

    if (ch[index] == set || (!set && numActive <= minInputChannels))
        return;

    // Clear a bit if necessary
    if (set && numActive >= maxInputChannels)
    {
        auto firstActiveChan = ch.findNextSetBit(0);
        ch.clearBit(index > firstActiveChan ? firstActiveChan : ch.getHighestBit());
    }

    ch.setBit(index, set);
}

juce::String ChannelSelectorListBox::getNameForChannelPair(const juce::String& name1, const juce::String& name2) const
{
    juce::String commonBit;

    for (int j = 0; j < name1.length(); ++j)
        if (name1.substring(0, j).equalsIgnoreCase(name2.substring(0, j)))
            commonBit = name1.substring(0, j);

    // Make sure we only split the name at a space, because otherwise, things like "input 11" + "input 12" would become "input 11 + 2"
    while (commonBit.isNotEmpty() && !juce::CharacterFunctions::isWhitespace(commonBit.getLastCharacter()))
        commonBit = commonBit.dropLastCharacters(1);

    return name1.trim() + " + " + name2.substring(commonBit.length()).trim();
}

//==============================================================================
InputDeviceSelector::InputDeviceSelector(juce::AudioDeviceManager& deviceManager, int minInputChannelsToUse, int maxInputChannelsToUse, bool showChannelsAsStereoPairs) :
    manager(deviceManager),
    titleLabel("", "Input Device Settings"),
    warningLabel("", "No device opened, check DAW audio settings if issues persist"),
    deviceTypeLabel("", "Type: "),
    deviceLabel("", "Device: "),
    channelsLabel("", "Channels: "),
    sampleRateLabel("", "Sample rate: "),
    bufferSizeLabel("", "Buffer size: "),

    levelMeter(deviceManager),
    channelSelector(deviceManager, minInputChannelsToUse, maxInputChannelsToUse, showChannelsAsStereoPairs)
{
    jassert(minInputChannelsToUse >= 0 && minInputChannelsToUse <= maxInputChannelsToUse);

    titleLabel.setJustificationType(juce::Justification::centred);

    juce::Array labels = { &titleLabel, &deviceTypeLabel, &deviceLabel, &channelsLabel, &sampleRateLabel, &bufferSizeLabel };
    for (auto* label : labels)
        addAndMakeVisible(label);

    warningLabel.setJustificationType(juce::Justification::centred);
    warningLabel.setColour(juce::Label::textColourId, Colors::HIGHLIGHT);
    addChildComponent(warningLabel);

    const auto& types= manager.getAvailableDeviceTypes();
    for (int i = 0; i < types.size(); ++i)
        deviceTypeChooser.addItem(types.getUnchecked(i)->getTypeName(), i + 1);
    deviceTypeChooser.setTextWhenNoChoicesAvailable("No devices available");
    deviceTypeChooser.setSelectedId(1);

    juce::Array comboBoxes = { &deviceTypeChooser, &deviceChooser, &sampleRateChooser, &bufferSizeChooser };
    for (auto* box : comboBoxes)
    {
        box->onChange = [this] { settingsChanged(); };
        box->setColour(Colors::backgroundColorId, Colors::FOREGROUND);
        addAndMakeVisible(box);
    }

    addAndMakeVisible(levelMeter);
    addAndMakeVisible(channelSelector);

    manager.addChangeListener(this);
    settingsChanged();
}

InputDeviceSelector::~InputDeviceSelector()
{
    manager.removeChangeListener(this);
}

void InputDeviceSelector::paint(juce::Graphics& g)
{
    g.fillAll(Colors::DARK.withAlpha(0.3f));
    g.setColour(Colors::FOREGROUND);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.f);
}

void InputDeviceSelector::resized()
{
    auto bounds = getLocalBounds().toFloat();
    auto w = bounds.getWidth();

    const auto row = bounds.getHeight() * 0.07f;
    const auto padding = bounds.getHeight() * 0.035f;
    constexpr auto labelProp = 0.25f;

    bounds.reduce(w * 0.12f, w * 0.05f);

    auto titleBounds = bounds.removeFromTop(bounds.proportionOfHeight(0.1f));
    titleLabel.setFont(getInter().withHeight(titleBounds.getHeight()));
    titleLabel.setBounds(titleBounds.toNearestInt());
    bounds.removeFromTop(padding / 4);

    auto warningLabelHeight = std::round(row * 0.665f);
    warningLabel.setFont(getInter().withHeight(warningLabelHeight));
    warningLabel.setBounds(bounds.removeFromTop(warningLabelHeight).toNearestInt());
    bounds.removeFromTop(padding);

    auto deviceTypeBounds = bounds.removeFromTop(row);
    deviceTypeLabel.setFont(getInter().withHeight(std::round(row * 0.95f)));
    deviceTypeLabel.setBounds(deviceTypeBounds.removeFromLeft(deviceTypeBounds.proportionOfWidth(labelProp)).toNearestInt());
    deviceTypeChooser.setBounds(deviceTypeBounds.withHeight(row * 1.2f).toNearestInt());
    bounds.removeFromTop(padding);

    auto deviceBounds = bounds.removeFromTop(row);
    deviceLabel.setFont(getInter().withHeight(std::round(row * 0.95f)));
    deviceLabel.setBounds(deviceBounds.removeFromLeft(deviceBounds.proportionOfWidth(labelProp)).toNearestInt());
    deviceChooser.setBounds(deviceBounds.removeFromLeft(bounds.proportionOfWidth(0.56f)).withHeight(row * 1.2f).toNearestInt());
    levelMeter.setBounds(deviceBounds.removeFromRight(bounds.proportionOfWidth(0.15f)).toNearestInt());
    bounds.removeFromTop(padding);

    auto channelsBounds = bounds.removeFromTop(5 * row);
    channelsLabel.setFont(getInter().withHeight(std::round(row * 0.95f)));
    channelsLabel.setBounds(channelsBounds.removeFromLeft(channelsBounds.proportionOfWidth(labelProp)).removeFromTop(row).toNearestInt());
    channelSelector.setBounds(channelsBounds.toNearestInt());
    channelSelector.setRowHeight(int(std::round(row)));
    bounds.removeFromTop(padding);

    auto lastRow = row * 0.7f;
    auto sampleRateBounds = bounds.removeFromTop(lastRow);
    auto bufferSizeBounds = sampleRateBounds.removeFromRight(sampleRateBounds.proportionOfWidth(0.55f));

    auto sampleRateLabelBounds = sampleRateBounds.removeFromLeft(sampleRateBounds.proportionOfWidth(0.5f));
    sampleRateLabel.setFont(getInter().withHeight(std::round(lastRow * 0.95f)));
    sampleRateLabel.setBounds(sampleRateLabelBounds.toNearestInt());
    sampleRateChooser.setBounds(sampleRateBounds.withHeight(lastRow * 1.2f).toNearestInt());
    bufferSizeBounds.removeFromLeft(bounds.getWidth() * 0.05f);

    bufferSizeLabel.setFont(getInter().withHeight(std::round(lastRow * 0.95f)));
    bufferSizeLabel.setBounds(bufferSizeBounds.removeFromLeft(bufferSizeBounds.proportionOfWidth(0.5f)).toNearestInt());
    bufferSizeChooser.setBounds(bufferSizeBounds.withHeight(lastRow * 1.2f).toNearestInt());
}

void InputDeviceSelector::changeListenerCallback(juce::ChangeBroadcaster*)
{
    auto* device = manager.getCurrentAudioDevice();

    warningLabel.setVisible(!device);
    if (!device)
        return;
 
    inputDevice = device;
    deviceChooser.setText(device->getName());

    sampleRateChooser.clear();
    for (auto rate : device->getAvailableSampleRates())
        sampleRateChooser.addItem(juce::String(rate) + " hz", int(rate));
    sampleRateChooser.setText(juce::String(int(device->getCurrentSampleRate())) + " hz");

    bufferSizeChooser.clear();
    for (auto size : device->getAvailableBufferSizes())
        bufferSizeChooser.addItem(juce::String(size), int(size));
    bufferSizeChooser.setText(juce::String(device->getCurrentBufferSizeSamples()));

    updateEnablement();
}

void InputDeviceSelector::settingsChanged()
{
    juce::AudioDeviceManager::AudioDeviceSetup settings = manager.getAudioDeviceSetup();

    auto deviceType = manager.getCurrentAudioDeviceType();
    auto* selectedDeviceType = manager.getAvailableDeviceTypes()[deviceTypeChooser.getSelectedId() - 1];
    bool newDeviceSelected = false;
    if (deviceType != selectedDeviceType->getTypeName() || !bool(deviceChooser.getNumItems()))
    {
        manager.setCurrentAudioDeviceType(selectedDeviceType->getTypeName(), true);

        deviceChooser.clear();
        auto devices = selectedDeviceType->getDeviceNames(true);
        for (int i = 0; i < devices.size(); i++)
            deviceChooser.addItem(devices[i], i + 1);
        deviceChooser.setSelectedId(1);
        newDeviceSelected = true;
    }

    auto device = settings.inputDeviceName;
    auto selectedDevice = deviceChooser.getItemText(deviceChooser.getSelectedItemIndex());
    if (device != selectedDevice || newDeviceSelected)
    {
        settings.inputDeviceName = selectedDevice;
        settings.useDefaultInputChannels = true;
    }

    auto sampleRate = int(settings.sampleRate);
    auto selectedSampleRate = sampleRateChooser.getSelectedId();
    if (sampleRate != selectedSampleRate && !newDeviceSelected && selectedSampleRate)
        settings.sampleRate = selectedSampleRate;

    auto bufferSize = settings.bufferSize;
    auto selectedBufferSize = bufferSizeChooser.getSelectedId();
    if (bufferSize != selectedBufferSize && !newDeviceSelected && selectedBufferSize)
        settings.bufferSize = selectedBufferSize;

    manager.setAudioDeviceSetup(settings, true);

    updateEnablement();
}

void InputDeviceSelector::updateEnablement()
{
    deviceChooser.setEnabled(bool(deviceChooser.getNumItems()));
    sampleRateChooser.setEnabled(bool(sampleRateChooser.getNumItems()) && inputDevice && manager.getAudioDeviceSetup().inputChannels.countNumberOfSetBits());
    bufferSizeChooser.setEnabled(bool(bufferSizeChooser.getNumItems()) && inputDevice && manager.getAudioDeviceSetup().inputChannels.countNumberOfSetBits());

    levelMeter.setEnabled(bool(inputDevice));
    channelSelector.setVisible(bool(inputDevice));
}

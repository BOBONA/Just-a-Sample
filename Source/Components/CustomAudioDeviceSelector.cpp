/*
    ==============================================================================

    CustomAudioDeviceSelector.cpp
    Created: 18 Aug 2024 8:20:19pm
    Author:  binya

    ==============================================================================
*/

#include "CustomAudioDeviceSelector.h"

struct SimpleDeviceManagerInputLevelMeter final : CustomComponent, juce::Timer
{
    explicit SimpleDeviceManagerInputLevelMeter(juce::AudioDeviceManager& m) : manager(m)
    {
        startTimerHz(20);
        inputLevelGetter = manager.getInputLevelGetter();
    }

    void timerCallback() override
    {
        if (isShowing())
        {
            auto newLevel = float(inputLevelGetter->getCurrentLevel());

            if (std::abs(level - newLevel) > 0.005f)
            {
                level = newLevel;
                repaint();
            }
        }
        else
        {
            level = 0;
        }
    }

    void paint(juce::Graphics& g) override
    {
        // (add a bit of a skew to make the level more obvious)
        getLookAndFeel().drawLevelMeter(g, getWidth(), getHeight(),
            float(std::exp(std::log(level) / 3.0)));
    }

    juce::AudioDeviceManager& manager;
    juce::AudioDeviceManager::LevelMeter::Ptr inputLevelGetter;
    float level = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleDeviceManagerInputLevelMeter)
};

static void drawTextLayout(juce::Graphics& g, juce::Component& owner, juce::StringRef text, const juce::Rectangle<int>& textBounds, bool enabled)
{
    const auto textColour = owner.findColour(juce::ListBox::textColourId, true).withMultipliedAlpha(enabled ? 1.0f : 0.5f);

    juce::AttributedString attributedString{ text };
    attributedString.setColour(textColour);
    attributedString.setFont(getInter().withHeight(float(textBounds.getHeight()) * 0.6f));
    attributedString.setWordWrap(juce::AttributedString::WordWrap::none);

    juce::TextLayout textLayout;
    textLayout.createLayout(attributedString,
        float(textBounds.getWidth()),
        float(textBounds.getHeight()));
    textLayout.draw(g, textBounds.toFloat());
}

//==============================================================================
struct AudioDeviceSetupDetails
{
    juce::AudioDeviceManager* manager;
    int minNumInputChannels, maxNumInputChannels;
    bool useStereoPairs;
};

//==============================================================================
class AudioDeviceSettingsPanel final : public CustomComponent, juce::ChangeListener
{
public:
    AudioDeviceSettingsPanel(juce::AudioIODeviceType& t, AudioDeviceSetupDetails& setupDetails, CustomAudioDeviceSelector& p)
        : type(t), setup(setupDetails), parent(p)
    {
        type.scanForDevices();

        setup.manager->addChangeListener(this);

        updateAllControls();
    }

    ~AudioDeviceSettingsPanel() override
    {
        setup.manager->removeChangeListener(this);
    }

    void resized() override
    {
        juce::Rectangle<int> r(proportionOfWidth(0.35f), 0, proportionOfWidth(0.6f), 3000);
        
        const int maxListBoxHeight = 100;
        const int h = parent.getItemHeight();
        const int space = h / 4;


        if (inputDeviceDropDown != nullptr)
        {
            auto row = r.removeFromTop(h);

            inputLevelMeter->setBounds(row.removeFromRight(row.getWidth() / 6));
            row.removeFromRight(space);
            inputDeviceDropDown->setBounds(row);
            r.removeFromTop(space);
        }

        if (inputChanList != nullptr)
        {
            inputChanList->setRowHeight(juce::jmin(22, h));
            inputChanList->setBounds(r.removeFromTop(inputChanList->getBestHeight(maxListBoxHeight)));
            inputChanLabel->setBounds(0, inputChanList->getBounds().getCentreY() - h / 2, r.getX(), h);
            r.removeFromTop(space);
        }

        r.removeFromTop(space * 2);

        if (sampleRateDropDown != nullptr)
        {
            sampleRateDropDown->setVisible(true);
            sampleRateDropDown->setBounds(r.removeFromTop(h));
            r.removeFromTop(space);
        }

        if (bufferSizeDropDown != nullptr)
        {
            bufferSizeDropDown->setVisible(true);
            bufferSizeDropDown->setBounds(r.removeFromTop(h));
            r.removeFromTop(space);
        }

        r.removeFromTop(space);

        if (showUIButton != nullptr || resetDeviceButton != nullptr)
        {
            auto buttons = r.removeFromTop(h);

            if (showUIButton != nullptr)
            {
                showUIButton->setVisible(true);
                showUIButton->changeWidthToFitText(h);
                showUIButton->setBounds(buttons.removeFromLeft(showUIButton->getWidth()));
                buttons.removeFromLeft(space);
            }

            if (resetDeviceButton != nullptr)
            {
                resetDeviceButton->setVisible(true);
                resetDeviceButton->changeWidthToFitText(h);
                resetDeviceButton->setBounds(buttons.removeFromLeft(resetDeviceButton->getWidth()));
            }

            r.removeFromTop(space);
        }

        setSize(getWidth(), r.getY());
    }

    void updateConfig(bool updateInputDevice, bool updateSampleRate, bool updateBufferSize)
    {
        auto config = setup.manager->getAudioDeviceSetup();
        juce::String error;

        if (updateInputDevice)
        {
            if (inputDeviceDropDown != nullptr)
                config.inputDeviceName = inputDeviceDropDown->getSelectedId() < 0 ? juce::String()
                : inputDeviceDropDown->getText();

            if (!type.hasSeparateInputsAndOutputs())
                config.inputDeviceName = config.outputDeviceName;

            config.useDefaultInputChannels = true;

            error = setup.manager->setAudioDeviceSetup(config, true);

            updateSelectedInput();
            resized();
        }
        else if (updateSampleRate)
        {
            if (sampleRateDropDown->getSelectedId())
            {
                config.sampleRate = sampleRateDropDown->getSelectedId();
                error = setup.manager->setAudioDeviceSetup(config, true);
            }
        }
        else if (updateBufferSize)
        {
            if (bufferSizeDropDown->getSelectedId())
            {
                config.bufferSize = bufferSizeDropDown->getSelectedId();
                error = setup.manager->setAudioDeviceSetup(config, true);
            }
        }

        if (error.isNotEmpty())
            messageBox = juce::AlertWindow::showScopedAsync(juce::MessageBoxOptions().withIconType(juce::MessageBoxIconType::WarningIcon)
                                                                                     .withTitle("Error when trying to open audio device!")
                                                                                     .withMessage(error)
                                                                                     .withButton("Ok"),
                                                            nullptr);
    }

    void updateAllControls()
    {
        updateInputsComboBox();

        if (auto* currentDevice = setup.manager->getCurrentAudioDevice())
        {
            if (setup.maxNumInputChannels > 0
                && setup.minNumInputChannels < setup.manager->getCurrentAudioDevice()->getInputChannelNames().size())
            {
                if (inputChanList == nullptr)
                {
                    inputChanList = std::make_unique<ChannelSelectorListBox>(setup, "No input channels found");
                    addAndMakeVisible(inputChanList.get());
                    inputChanLabel = std::make_unique<juce::Label>("", "Channels: ");
                    inputChanLabel->setJustificationType(juce::Justification::centredRight);
                    inputChanLabel->setFont(getInter().withHeight(17.f));
                    inputChanLabel->attachToComponent(inputChanList.get(), true);
                }

                inputChanList->refresh();
            }
            else
            {
                inputChanLabel.reset();
                inputChanList.reset();
            }

            updateSampleRateComboBox(currentDevice);
            updateBufferSizeComboBox(currentDevice);
        }
        else
        {
            jassert(setup.manager->getCurrentAudioDevice() == nullptr); // not the correct device type!

            inputChanLabel.reset();
            sampleRateLabel.reset();
            bufferSizeLabel.reset();

            inputChanList.reset();
            sampleRateDropDown.reset();
            bufferSizeDropDown.reset();

            if (inputDeviceDropDown != nullptr)
                inputDeviceDropDown->setSelectedId(-1, juce::dontSendNotification);
        }

        sendLookAndFeelChange();
        resized();
        setSize(getWidth(), getLowestY() + 4);
    }

    void changeListenerCallback(juce::ChangeBroadcaster*) override
    {
        updateAllControls();
    }

    void resetDevice()
    {
        setup.manager->closeAudioDevice();
        setup.manager->restartLastAudioDevice();
    }

private:
    juce::AudioIODeviceType& type;
    const AudioDeviceSetupDetails setup;
    CustomAudioDeviceSelector& parent;

    std::unique_ptr<juce::ComboBox> inputDeviceDropDown, sampleRateDropDown, bufferSizeDropDown;
    std::unique_ptr<juce::Label> inputDeviceLabel, sampleRateLabel, bufferSizeLabel, inputChanLabel;
    std::unique_ptr<Component> inputLevelMeter;
    std::unique_ptr<juce::TextButton> showUIButton, resetDeviceButton;

    int findSelectedDeviceIndex(bool isInput) const
    {
        const auto device = setup.manager->getAudioDeviceSetup();
        const auto deviceName = isInput ? device.inputDeviceName : device.outputDeviceName;
        return type.getDeviceNames(isInput).indexOf(deviceName);
    }

    void updateSelectedInput()
    {
        showCorrectDeviceName(inputDeviceDropDown.get(), true);
    }

    void showCorrectDeviceName(juce::ComboBox* box, bool isInput)
    {
        if (box == nullptr)
            return;

        const auto index = findSelectedDeviceIndex(isInput);
        box->setSelectedId(index < 0 ? index : index + 1, juce::dontSendNotification);
    }

    void addNamesToDeviceBox(juce::ComboBox& combo, bool isInputs)
    {
        const juce::StringArray devs(type.getDeviceNames(isInputs));

        combo.clear(juce::dontSendNotification);

        for (int i = 0; i < devs.size(); ++i)
            combo.addItem(devs[i], i + 1);

        combo.addItem("None", -1);
        combo.setSelectedId(-1, juce::dontSendNotification);
    }

    int getLowestY() const
    {
        int y = 0;

        for (auto* c : getChildren())
            y = juce::jmax(y, c->getBottom());

        return y;
    }

    void updateInputsComboBox()
    {
        if (setup.maxNumInputChannels > 0 && type.hasSeparateInputsAndOutputs())
        {
            if (inputDeviceDropDown == nullptr)
            {
                inputDeviceDropDown = std::make_unique<juce::ComboBox>();
                inputDeviceDropDown->onChange = [this] { updateConfig(true, false, false); };
                addAndMakeVisible(inputDeviceDropDown.get());

                inputDeviceLabel = std::make_unique<juce::Label>("", "Device: ");
                inputDeviceLabel->setJustificationType(juce::Justification::centredRight);
                inputDeviceLabel->setFont(getInter().withHeight(17.f));
                inputDeviceLabel->attachToComponent(inputDeviceDropDown.get(), true);

                inputLevelMeter = std::make_unique<SimpleDeviceManagerInputLevelMeter>(*setup.manager);
                addAndMakeVisible(inputLevelMeter.get());
            }

            addNamesToDeviceBox(*inputDeviceDropDown, true);
        }

        updateSelectedInput();
    }

    void updateSampleRateComboBox(juce::AudioIODevice* currentDevice)
    {
        if (sampleRateDropDown == nullptr)
        {
            sampleRateDropDown = std::make_unique<juce::ComboBox>();
            addAndMakeVisible(sampleRateDropDown.get());

            sampleRateLabel = std::make_unique<juce::Label>("", "Sample rate: ");
            sampleRateLabel->setJustificationType(juce::Justification::centredRight);
            sampleRateLabel->setFont(getInter().withHeight(17.f));
            sampleRateLabel->attachToComponent(sampleRateDropDown.get(), true);
        }
        else
        {
            sampleRateDropDown->clear();
            sampleRateDropDown->onChange = nullptr;
        }

        for (auto rate : currentDevice->getAvailableSampleRates())
        {
            const auto intRate = juce::roundToInt(rate);
            sampleRateDropDown->addItem(juce::String(intRate), juce::roundToInt(rate));
        }

        const auto intRate = juce::roundToInt(currentDevice->getCurrentSampleRate());
        sampleRateDropDown->setText(juce::String(intRate), juce::dontSendNotification);

        sampleRateDropDown->onChange = [this] { updateConfig(false, true, false); };
    }

    void updateBufferSizeComboBox(juce::AudioIODevice* currentDevice)
    {
        if (bufferSizeDropDown == nullptr)
        {
            bufferSizeDropDown = std::make_unique<juce::ComboBox>();
            addAndMakeVisible(bufferSizeDropDown.get());

            bufferSizeLabel = std::make_unique<juce::Label>("", "Buffer size: ");
            bufferSizeLabel->setJustificationType(juce::Justification::centredRight);
            bufferSizeLabel->setFont(getInter().withHeight(17.f));
            bufferSizeLabel->attachToComponent(bufferSizeDropDown.get(), true);
        }
        else
        {
            bufferSizeDropDown->clear();
            bufferSizeDropDown->onChange = nullptr;
        }

        auto currentRate = currentDevice->getCurrentSampleRate();

        if (juce::exactlyEqual(currentRate, 0.0))
            currentRate = 48000.0;

        for (auto bs : currentDevice->getAvailableBufferSizes())
            bufferSizeDropDown->addItem(juce::String(bs), bs);

        bufferSizeDropDown->setSelectedId(currentDevice->getCurrentBufferSizeSamples(), juce::dontSendNotification);
        bufferSizeDropDown->onChange = [this] { updateConfig(false, false, true); };
    }

public:
    //==============================================================================
    class ChannelSelectorListBox final : public juce::ListBox, juce::ListBoxModel
    {
    public:
        //==============================================================================
        ChannelSelectorListBox(const AudioDeviceSetupDetails& setupDetails, juce::String noItemsText)
            : ListBox({}, nullptr), setup(setupDetails), noItemsMessage(std::move(noItemsText))
        {
            refresh();
            setModel(this);
            setOutlineThickness(1);
        }

        void refresh()
        {
            items.clear();

            if (auto* currentDevice = setup.manager->getCurrentAudioDevice())
            {
                items = currentDevice->getInputChannelNames();
             
                if (setup.useStereoPairs)
                {
                    juce::StringArray pairs;

                    for (int i = 0; i < items.size(); i += 2)
                    {
                        auto& name = items[i];

                        if (i + 1 >= items.size())
                            pairs.add(name.trim());
                        else
                            pairs.add(getNameForChannelPair(name, items[i + 1]));
                    }

                    items = pairs;
                }
            }

            updateContent();
            repaint();
        }

        int getNumRows() override
        {
            return items.size();
        }

        void paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool) override
        {
            if (juce::isPositiveAndBelow(row, items.size()))
            {
                g.fillAll(findColour(backgroundColourId));

                auto item = items[row];
                bool enabled;
                auto config = setup.manager->getAudioDeviceSetup();

                if (setup.useStereoPairs)
                    enabled = config.inputChannels[row * 2] || config.inputChannels[row * 2 + 1];
                else
                    enabled = config.inputChannels[row];

                auto x = getTickX();
                auto tickW = float(height) * 0.75f;

                juce::ToggleButton temp;
                float bx = float(x) - tickW;
                float by = (float(height) - tickW) * 0.5f;
                temp.setBounds(bx, by, tickW, tickW);
                getLookAndFeel().drawTickBox(g, temp, bx, by, tickW, tickW, enabled, true, true, false);

                drawTextLayout(g, *this, item, { x + 5, 0, width - x - 5, height }, enabled);
            }
        }

        void listBoxItemClicked(int row, const juce::MouseEvent& e) override
        {
            selectRow(row);

            if (e.x < getTickX())
                flipEnablement(row);
        }

        void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override
        {
            flipEnablement(row);
        }

        void returnKeyPressed(int row) override
        {
            flipEnablement(row);
        }

        void paint(juce::Graphics& g) override
        {
            juce::ListBox::paint(g);

            if (items.isEmpty())
            {
                g.setColour(juce::Colours::grey);
                g.setFont(0.5f * (float)getRowHeight());
                g.drawText(noItemsMessage,
                    0, 0, getWidth(), getHeight() / 2,
                    juce::Justification::centred, true);
            }
        }

        int getBestHeight(int maxHeight)
        {
            return getRowHeight() * juce::jlimit(2, juce::jmax(2, maxHeight / getRowHeight()),
                                                 getNumRows())
                + getOutlineThickness() * 2;
        }

    private:
        //==============================================================================
        const AudioDeviceSetupDetails setup;
        const juce::String noItemsMessage;
        juce::StringArray items;

        static juce::String getNameForChannelPair(const juce::String& name1, const juce::String& name2)
        {
            juce::String commonBit;

            for (int j = 0; j < name1.length(); ++j)
                if (name1.substring(0, j).equalsIgnoreCase(name2.substring(0, j)))
                    commonBit = name1.substring(0, j);

            // Make sure we only split the name at a space, because otherwise, things
            // like "input 11" + "input 12" would become "input 11 + 2"
            while (commonBit.isNotEmpty() && !juce::CharacterFunctions::isWhitespace(commonBit.getLastCharacter()))
                commonBit = commonBit.dropLastCharacters(1);

            return name1.trim() + " + " + name2.substring(commonBit.length()).trim();
        }

        void flipEnablement(int row)
        {
            if (juce::isPositiveAndBelow(row, items.size()))
            {
                auto config = setup.manager->getAudioDeviceSetup();

                if (setup.useStereoPairs)
                {
                    juce::BigInteger bits;
                    auto& original = config.inputChannels;

                    for (int i = 0; i < 256; i += 2)
                        bits.setBit(i / 2, original[i] || original[i + 1]);

                    config.useDefaultInputChannels = false;
                    flipBit(bits, row, setup.minNumInputChannels / 2, setup.maxNumInputChannels / 2);
                   
                    for (int i = 0; i < 256; ++i)
                        original.setBit(i, bits[i / 2]);
                }
                else
                {
                    config.useDefaultInputChannels = false;
                    flipBit(config.inputChannels, row, setup.minNumInputChannels, setup.maxNumInputChannels);
                }

                setup.manager->setAudioDeviceSetup(config, true);
            }
        }

        static void flipBit(juce::BigInteger& chans, int index, int minNumber, int maxNumber)
        {
            auto numActive = chans.countNumberOfSetBits();

            if (chans[index])
            {
                if (numActive > minNumber)
                    chans.setBit(index, false);
            }
            else
            {
                if (numActive >= maxNumber)
                {
                    auto firstActiveChan = chans.findNextSetBit(0);
                    chans.clearBit(index > firstActiveChan ? firstActiveChan : chans.getHighestBit());
                }

                chans.setBit(index, true);
            }
        }

        int getTickX() const
        {
            return getRowHeight();
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelSelectorListBox)
    };

private:
    std::unique_ptr<ChannelSelectorListBox> inputChanList;
    juce::ScopedMessageBox messageBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioDeviceSettingsPanel)
};


//==============================================================================
CustomAudioDeviceSelector::CustomAudioDeviceSelector(juce::AudioDeviceManager& dm, int minInputChannelsToUse, int maxInputChannelsToUse, bool showChannelsAsStereoPairsToUse) :
    deviceManager(dm),
    deviceTypesLabel("", "Type: "),
    itemHeight(24),
  
    minInputChannels(minInputChannelsToUse),
    maxInputChannels(maxInputChannelsToUse),
    showChannelsAsStereoPairs(showChannelsAsStereoPairsToUse)
{
    jassert(minInputChannels >= 0 && minInputChannels <= maxInputChannels);

    const juce::OwnedArray<juce::AudioIODeviceType>& types = deviceManager.getAvailableDeviceTypes();

    for (int i = 0; i < types.size(); ++i)
        deviceTypes.addItem(types.getUnchecked(i)->getTypeName(), i + 1);
    deviceTypes.setTextWhenNoChoicesAvailable("No devices available");
    deviceTypes.onChange = [this] { updateDeviceType(); };
    addAndMakeVisible(deviceTypes);

    deviceTypesLabel.setJustificationType(juce::Justification::centredRight);
    deviceTypesLabel.setFont(getInter().withHeight(17.f));
    deviceTypesLabel.attachToComponent(&deviceTypes, true);

    deviceManager.addChangeListener(this);
    updateAllControls();
}

CustomAudioDeviceSelector::~CustomAudioDeviceSelector()
{
    deviceManager.removeChangeListener(this);
}

void CustomAudioDeviceSelector::setItemHeight(int newItemHeight)
{
    itemHeight = newItemHeight;
    resized();
}

void CustomAudioDeviceSelector::paint(juce::Graphics& g)
{
    g.fillAll(Colors::DARK.withAlpha(0.3f));
    g.setColour(Colors::FOREGROUND);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.f);
}

void CustomAudioDeviceSelector::resized()
{
    juce::Rectangle r(proportionOfWidth(0.35f), 15, proportionOfWidth(0.6f), 3000);
    auto space = itemHeight / 4;

 
    deviceTypes.setBounds(r.removeFromTop(itemHeight));
    r.removeFromTop(space * 3);

    if (audioDeviceSettingsComp != nullptr)
    {
        audioDeviceSettingsComp->resized();
        audioDeviceSettingsComp->setBounds(r.removeFromTop(audioDeviceSettingsComp->getHeight())
            .withX(0).withWidth(getWidth()));
        r.removeFromTop(space);
    }

    r.removeFromTop(itemHeight);
    setSize(getWidth(), r.getY());
}

void CustomAudioDeviceSelector::childBoundsChanged(Component* child)
{
    if (child == audioDeviceSettingsComp.get())
        resized();
}

void CustomAudioDeviceSelector::updateDeviceType()
{
    if (auto* type = deviceManager.getAvailableDeviceTypes()[deviceTypes.getSelectedId() - 1])
    {
        audioDeviceSettingsComp.reset();
        deviceManager.setCurrentAudioDeviceType(type->getTypeName(), true);
        updateAllControls(); // needed in case the type hasn't actually changed
    }
}

void CustomAudioDeviceSelector::changeListenerCallback(juce::ChangeBroadcaster*)
{
    updateAllControls();
}

void CustomAudioDeviceSelector::updateAllControls()
{
    deviceTypes.setText(deviceManager.getCurrentAudioDeviceType(), juce::dontSendNotification);

    if (audioDeviceSettingsComp == nullptr || audioDeviceSettingsCompType != deviceManager.getCurrentAudioDeviceType())
    {
        audioDeviceSettingsCompType = deviceManager.getCurrentAudioDeviceType();
        audioDeviceSettingsComp.reset();

        if (auto* type = deviceManager.getAvailableDeviceTypes()[deviceTypes.getSelectedId() - 1])
        {
            AudioDeviceSetupDetails details{&deviceManager, minInputChannels, maxInputChannels, showChannelsAsStereoPairs};

            audioDeviceSettingsComp = std::make_unique<AudioDeviceSettingsPanel>(*type, details, *this);
            addAndMakeVisible(audioDeviceSettingsComp.get());
        }
    }

    resized();
}

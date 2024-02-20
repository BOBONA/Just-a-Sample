#include "PluginProcessor.h"
#include "PluginEditor.h"

JustaSampleAudioProcessorEditor::JustaSampleAudioProcessorEditor(JustaSampleAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p), synthVoices(p.getSynthVoices()), lnf(dynamic_cast<CustomLookAndFeel&>(getLookAndFeel())),
    sampleEditor(processor.apvts, synthVoices),
    sampleNavigator(processor.apvts, synthVoices),
    semitoneSliderAttachment(processor.apvts, PluginParameters::SEMITONE_TUNING, semitoneSlider),
    centSliderAttachment(processor.apvts, PluginParameters::CENT_TUNING, centSlider),
    playbackOptionsAttachment(processor.apvts, PluginParameters::PLAYBACK_MODE, playbackOptions),
    loopToggleButtonAttachment(processor.apvts, PluginParameters::IS_LOOPING, isLoopingButton),
    masterGainSliderAttachment(processor.apvts, PluginParameters::MASTER_GAIN, masterGainSlider),
    audioDeviceSettings(p.deviceManager, 0, 2, 0, 0, false, false, true, false),
    filenameComponent("File_Chooser", {}, true, false, false, p.formatManager.getWildcardForAllFormats(), "", "Select a file to load..."),
    storeFileButton("Store_File", Colours::white, Colours::lightgrey, Colours::darkgrey),
    magicPitchButton("Detect_Pitch", Colours::white, Colours::lightgrey, Colours::darkgrey),
    recordButton("Record_Sound", Colours::white, Colours::lightgrey, Colours::darkgrey),
    deviceSettingsButton("Device_Settings", Colours::white, Colours::lightgrey, Colours::darkgrey),
    haltButton("Halt_Sound", Colours::white, Colours::lightgrey, Colours::darkgrey),
    fxChain(p)
{
    if (hostType.isReaper())
    {
        openGLContext.attachTo(*getTopLevelComponent());
    }

    p.apvts.state.addListener(this);

    setResizeLimits(250, 200, 1000, 800);
    setResizable(true, false);
    int width = p.apvts.state.getProperty(PluginParameters::WIDTH);
    int height = p.apvts.state.getProperty(PluginParameters::HEIGHT);
    if (250 > width || width > 1000 || 200 > height || height > 800)
    {
        setSize(500, 400);
    }
    else
    {
        setSize(width, height);
    }
    
    auto recentFiles{ p.apvts.state.getProperty(PluginParameters::RECENT_FILES) };
    for (int i = recentFiles.size() - 1; i >= 0; i--)
    {
        filenameComponent.addRecentlyUsedFile(File(recentFiles[i]));
    }
    filenameComponent.addListener(this);
    addAndMakeVisible(filenameComponent);

    Path storeIcon;
    storeIcon.loadPathFromData(PathData::saveIcon, sizeof(PathData::saveIcon));
    storeIcon.scaleToFit(0, 0, 13, 13, true);
    storeFileButton.setShape(storeIcon, true, true, false);
    storeFileButton.setClickingTogglesState(true);
    storeFileButton.shouldUseOnColours(true);
    storeFileButton.setOnColours(Colours::darkgrey, Colours::lightgrey, Colours::white);
    storeFileButton.onClick = [this]() -> void {
        processor.usingFileReference = !storeFileButton.getToggleState();
        };
    sampleRequiredControls.add(&storeFileButton);
    addAndMakeVisible(storeFileButton);

    tuningLabel.setText("Tuning: ", dontSendNotification);
    addAndMakeVisible(tuningLabel);

    semitoneSlider.setSliderStyle(Slider::LinearBar);
    sampleRequiredControls.add(&semitoneSlider);
    addAndMakeVisible(semitoneSlider);
    
    centSlider.setSliderStyle(Slider::LinearBar);
    sampleRequiredControls.add(&centSlider);
    addAndMakeVisible(centSlider);

    magicPitchButtonShape.addStar(Point(15.f, 15.f), 5, 8.f, 3.f, 0.45f);
    magicPitchButton.setShape(magicPitchButtonShape, true, true, false);
    magicPitchButton.onClick = [this]() -> void {
        if (!boundsSelectRoutine && playbackOptions.isEnabled())
        {
            // This changes the state until a portion of the sample is selected, afterwards the pitch detection will happen
            sampleEditor.boundsSelectPrompt("Drag to select a portion of the sample to analyze.");
            magicPitchButton.setEnabled(false);
            magicPitchButton.setColours(Colours::white, Colours::white, Colours::white);
            boundsSelectRoutine = true;
            showPromptBackground({ &sampleEditor, &sampleNavigator });
        }
        };
    sampleRequiredControls.add(&magicPitchButton);
    addAndMakeVisible(magicPitchButton);

    Path recordIcon;
    recordIcon.loadPathFromData(PathData::recordIcon, sizeof(PathData::recordIcon));
    recordIcon.scaleToFit(0, 0, 13, 13, true);
    recordButton.setShape(recordIcon, true, true, false);
    recordButton.setClickingTogglesState(true);
    recordButton.shouldUseOnColours(true);
    recordButton.setOnColours(Colours::darkgrey, Colours::lightgrey, Colours::white);
    recordButton.onClick = [this]() -> void {
        
        };
    addChildComponent(recordButton);

    Path deviceSettingsIcon;
    deviceSettingsIcon.loadPathFromData(PathData::settingsIcon, sizeof(PathData::settingsIcon));
    deviceSettingsIcon.scaleToFit(0, 0, 6, 6, true);
    deviceSettingsButton.setShape(deviceSettingsIcon, true, true, false);
    deviceSettingsButton.onClick = [this]() -> void {
        audioDeviceSettings.setVisible(true);
        showPromptBackground({ &audioDeviceSettings });
        };
    addAndMakeVisible(deviceSettingsButton);

    playbackOptions.addItemList(PluginParameters::PLAYBACK_MODE_LABELS, 1);
    playbackOptions.setSelectedItemIndex(processor.apvts.getParameterAsValue(PluginParameters::PLAYBACK_MODE).getValue());
    sampleRequiredControls.add(&playbackOptions);
    addAndMakeVisible(playbackOptions);

    isLoopingLabel.setText("Loop:", dontSendNotification);
    addAndMakeVisible(isLoopingLabel);

    sampleRequiredControls.add(&isLoopingButton);
    addAndMakeVisible(isLoopingButton);

    masterGainSlider.setSliderStyle(Slider::LinearBar);
    masterGainSlider.setTextValueSuffix(" db");
    addAndMakeVisible(masterGainSlider);

    Path stopPath;
    stopPath.loadPathFromData(PathData::stopIcon, sizeof(PathData::stopIcon));
    stopPath.scaleToFit(0, 0, 13, 13, true);
    haltButton.setShape(stopPath, true, true, false);
    haltButton.onClick = [this]() -> void {
        processor.haltVoices();
        };
    sampleRequiredControls.add(&haltButton);
    addAndMakeVisible(haltButton);

    addAndMakeVisible(fxChain);

    for (Component* comp : sampleRequiredControls)
    {
        if (comp)
        {
            comp->setEnabled(false);
        }
    }

    sampleEditor.addBoundsSelectListener(this);
    addAndMakeVisible(sampleEditor);
    addAndMakeVisible(sampleNavigator);
    updateWorkingSample();

    promptBackground.setAlpha(0.35f);
    promptBackground.setInterceptsMouseClicks(true, false);
    addAndMakeVisible(promptBackground);
    
    addChildComponent(audioDeviceSettings);

    tooltipWindow.setAlwaysOnTop(true);
    addAndMakeVisible(tooltipWindow);

    setWantsKeyboardFocus(true);
    addMouseListener(this, true);
    startTimerHz(60);
}

JustaSampleAudioProcessorEditor::~JustaSampleAudioProcessorEditor()
{
    sampleEditor.removeBoundsSelectListener(this);
    processor.apvts.state.removeListener(this);
    for (const auto& param : listeningParameters)
    {
        processor.apvts.removeParameterListener(param, this);
    }
}

void JustaSampleAudioProcessorEditor::paint(Graphics& g)
{
    g.fillAll(lnf.BACKGROUND_COLOR);
}

void JustaSampleAudioProcessorEditor::resized()
{
    processor.editorWidth = getWidth();
    processor.editorHeight = getHeight();
    auto bounds = getLocalBounds();

    if (promptBackgroundVisible)
        promptBackground.setRectangle(Parallelogram<float>(getLocalBounds().toFloat()));
    else
        promptBackground.setRectangle(Parallelogram<float>());

    if (audioDeviceSettings.isVisible())
        audioDeviceSettings.setBounds(bounds.reduced(jmin<int>(bounds.getWidth(), bounds.getHeight()) / 4));
    else
        audioDeviceSettings.setBounds(0, 0, 0, 0);

    FlexBox topControls{ FlexBox::Direction::row, FlexBox::Wrap::wrap, FlexBox::AlignContent::stretch, 
        FlexBox::AlignItems::stretch, FlexBox::JustifyContent::flexEnd };
    topControls.items.add(FlexItem(filenameComponent).withFlex(1).withMinWidth(float(getWidth() - 15.f)));
    topControls.items.add(FlexItem(storeFileButton).withMinWidth(15));
    topControls.items.add(FlexItem(tuningLabel).withMinWidth(50));
    topControls.items.add(FlexItem(semitoneSlider).withMinWidth(30));
    topControls.items.add(FlexItem(centSlider).withMinWidth(40));
    topControls.items.add(FlexItem(magicPitchButton).withMinWidth(15));
    topControls.items.add(FlexItem(recordButton).withMinWidth(15));
    topControls.items.add(FlexItem(deviceSettingsButton).withMinWidth(6));
    topControls.items.add(FlexItem(playbackOptions).withMinWidth(150));
    topControls.items.add(FlexItem(isLoopingLabel).withMinWidth(35));
    topControls.items.add(FlexItem(isLoopingButton).withMinWidth(20));
    topControls.items.add(FlexItem(masterGainSlider).withMinWidth(60));
    topControls.items.add(FlexItem(haltButton).withMinWidth(15));
    float totalWidth = 0;
    for (const FlexItem& item : topControls.items)
    {
        totalWidth += item.minWidth;
    }
    auto top = bounds.removeFromTop(15 * int(ceilf(totalWidth / getWidth())));
    topControls.performLayout(top);

    auto editor = bounds.removeFromTop(int(bounds.getHeight() * 0.66f));
    auto navigator = bounds.removeFromTop(int(bounds.getHeight() * 0.2f));

    sampleEditor.setBounds(editor);
    sampleNavigator.setBounds(navigator);
    fxChain.setBounds(bounds);
}

void JustaSampleAudioProcessorEditor::timerCallback()
{
    // Update Navigator and Editor
    bool wasPlaying = currentlyPlaying;
    currentlyPlaying = false;
    for (CustomSamplerVoice* voice : synthVoices)
    {
        if (voice->getCurrentlyPlayingSound())
        {
            currentlyPlaying = true;
            break;
        }
    }
    if ((wasPlaying && !currentlyPlaying) || currentlyPlaying)
    {
        sampleEditor.repaintUI();
        sampleNavigator.repaintUI();
    }

    // pitch detection UI changes
    if (boundsSelectRoutine) // little animation
    {
        magicPitchButtonShape.applyTransform(AffineTransform::rotation(MathConstants<float>::pi / 180));
        magicPitchButton.setShape(magicPitchButtonShape, false, true, false);
    }
}

bool JustaSampleAudioProcessorEditor::isInterestedInFileDrag(const String& file)
{
    return processor.canLoadFileExtension(file);
}

bool JustaSampleAudioProcessorEditor::isInterestedInFileDrag(const StringArray& files)
{
    for (const String& file : files)
    {
        if (isInterestedInFileDrag(file))
        {
            return true;
        }
    }
    return false;
}

void JustaSampleAudioProcessorEditor::filesDropped(const StringArray& files, int, int)
{
    for (const String& file : files)
    {
        if (isInterestedInFileDrag(file))
        {
            processor.loadFileAndReset(file);
            break;
        }
    }
}

void JustaSampleAudioProcessorEditor::valueTreePropertyChanged(ValueTree&, const Identifier& property)
{
    if (property.toString() == PluginParameters::FILE_PATH)
    {
        updateWorkingSample();
    }
}

void JustaSampleAudioProcessorEditor::updateWorkingSample()
{
    if (processor.getSample().getNumSamples() > 0)
    {
        for (Component* comp : sampleRequiredControls)
        {
            if (comp)
            {
                comp->setEnabled(true);
            }
        }
        filenameComponent.setCurrentFile(File(processor.p(PluginParameters::FILE_PATH)), true, dontSendNotification);
        if (processor.sampleBufferNeedsReference())
        {
            storeFileButton.setEnabled(false);
        }
        storeFileButton.setToggleState(!processor.usingFileReference, dontSendNotification);
        sampleEditor.setSample(processor.getSample(), processor.resetParameters);
        sampleNavigator.setSample(processor.getSample(), processor.resetParameters);
    }
    else
    {
        for (Component* comp : sampleRequiredControls)
        {
            if (comp)
            {
                comp->setEnabled(false);
            }
        }
    }
}

void JustaSampleAudioProcessorEditor::boundsSelected(int startSample, int endSample)
{
    hidePromptBackground();
    processor.pitchDetectionRoutine(startSample, endSample);
}

bool JustaSampleAudioProcessorEditor::keyPressed(const KeyPress& key)
{
    if (promptBackgroundVisible)
    {
        if (key == KeyPress::escapeKey || key == KeyPress::spaceKey)
        {
            hidePromptBackground();
            return true;
        }
    }

    return false;
}

void JustaSampleAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    if (promptBackgroundVisible)
    {
        if (event.eventComponent == &promptBackground)
        {
            hidePromptBackground();
        }
    }
}

void JustaSampleAudioProcessorEditor::mouseUp(const MouseEvent& event)
{
    if (promptBackgroundVisible)
    {
        if (firstMouseUp)
        {
            auto e = event.getEventRelativeTo(this);
            bool fromVisibleComponent = false;
            for (Component* comp : promptVisibleComponents)
            {
                if (comp->getBoundsInParent().contains(e.getPosition()))
                {
                    fromVisibleComponent = true;
                    break;
                }
            }
            if (!fromVisibleComponent)
            {
                hidePromptBackground();
            }
        }
        else
        {
            firstMouseUp = true;
        }
    }
}

void JustaSampleAudioProcessorEditor::showPromptBackground(Array<Component*> visibleComponents)
{
    firstMouseUp = false;
    promptBackground.setVisible(true);
    promptBackground.toFront(true);
    promptBackgroundVisible = true;
    promptVisibleComponents = visibleComponents;
    for (Component* comp : visibleComponents)
    {
        comp->toFront(true);
    }
    resized();
}

void JustaSampleAudioProcessorEditor::hidePromptBackground()
{
    onPromptExit();
    promptBackgroundVisible = false;
    promptBackground.setVisible(false);
    resized();
}

void JustaSampleAudioProcessorEditor::onPromptExit()
{
    if (boundsSelectRoutine)
    {
        boundsSelectRoutine = false;
        sampleEditor.endBoundsSelectPrompt();
        magicPitchButton.setEnabled(true);
        magicPitchButton.setColours(Colours::white, Colours::lightgrey, Colours::darkgrey);
    }
    else if (audioDeviceSettings.isVisible())
    {
        audioDeviceSettings.setVisible(false);
    }
}

void JustaSampleAudioProcessorEditor::filenameComponentChanged(FilenameComponent* fileComponentThatHasChanged)
{
    File file = fileComponentThatHasChanged->getCurrentFile();
    bool fileLoaded = processor.loadFileAndReset(file.getFullPathName());
    if (fileLoaded)
    {
        processor.apvts.state.setProperty(PluginParameters::RECENT_FILES, fileComponentThatHasChanged->getRecentlyUsedFilenames(), &processor.undoManager);
    }
    else
    {
        fileComponentThatHasChanged->setCurrentFile(File(processor.p(PluginParameters::FILE_PATH)), false, dontSendNotification);
        auto recentFiles = fileComponentThatHasChanged->getRecentlyUsedFilenames();
        recentFiles.removeString(file.getFullPathName());
        fileComponentThatHasChanged->setRecentlyUsedFilenames(recentFiles);
    }
}

void JustaSampleAudioProcessorEditor::parameterChanged(const String&, float)
{
}

void JustaSampleAudioProcessorEditor::addListeningParameters(std::vector<String> parameters)
{
    for (const auto& param : parameters)
    {
        processor.apvts.addParameterListener(param, this);
        listeningParameters.push_back(param);
    }
}

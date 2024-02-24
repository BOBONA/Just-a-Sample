#include "PluginProcessor.h"
#include "PluginEditor.h"

JustaSampleAudioProcessorEditor::JustaSampleAudioProcessorEditor(JustaSampleAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p), synthVoices(p.samplerVoices), lnf(dynamic_cast<CustomLookAndFeel&>(getLookAndFeel())),
    sampleEditor(processor.apvts, synthVoices),
    sampleNavigator(processor.apvts, synthVoices),
    semitoneSliderAttachment(processor.apvts, PluginParameters::SEMITONE_TUNING, semitoneSlider),
    centSliderAttachment(processor.apvts, PluginParameters::CENT_TUNING, centSlider),
    playbackOptionsAttachment(processor.apvts, PluginParameters::PLAYBACK_MODE, playbackOptions),
    loopToggleButtonAttachment(processor.apvts, PluginParameters::IS_LOOPING, isLoopingButton),
    masterGainSliderAttachment(processor.apvts, PluginParameters::MASTER_GAIN, masterGainSlider),
    audioDeviceSettings(p.deviceManager, 0, 2, 0, 0, false, false, true, false),
    filenameComponent("File_Chooser", {}, true, false, false, p.formatManager.getWildcardForAllFormats(), "", "Select a file to load..."),
    storeSampleToggle("Store_File", Colours::white, Colours::lightgrey, Colours::darkgrey),
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
    addListeningParameters({ PluginParameters::MASTER_GAIN });

    setResizeLimits(250, 200, 1000, 800);
    setResizable(true, false);
    int width = p.p(PluginParameters::WIDTH);
    int height = p.p(PluginParameters::HEIGHT);
    if (250 > width || width > 1000 || 200 > height || height > 800)
    {
        setSize(500, 400);
    }
    else
    {
        setSize(width, height);
    }
    
    auto recentFiles{ p.p(PluginParameters::RECENT_FILES) };
    for (int i = recentFiles.size() - 1; i >= 0; i--)
    {
        filenameComponent.addRecentlyUsedFile(File(recentFiles[i]));
    }
    filenameComponent.addListener(this);
    addAndMakeVisible(filenameComponent);

    Path storeIcon;
    storeIcon.loadPathFromData(PathData::saveIcon, sizeof(PathData::saveIcon));
    storeIcon.scaleToFit(0, 0, 13, 13, true);
    storeSampleToggle.setShape(storeIcon, true, true, false);
    storeSampleToggle.setClickingTogglesState(true);
    storeSampleToggle.shouldUseOnColours(true);
    storeSampleToggle.setOnColours(Colours::darkgrey, Colours::lightgrey, Colours::white);
    storeSampleToggle.onClick = [this]() -> void {
        if (!processor.usingFileReference && processor.p(PluginParameters::FILE_PATH).toString().isEmpty()) // See if a file prompt is necessary
        {
            processor.openFileChooser("Save the sample to a file",
                FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles, [this](const FileChooser& chooser) -> void {
                    File file = chooser.getResult();
                    std::unique_ptr<FileOutputStream> stream = std::make_unique<FileOutputStream>(file);
                    if (file.hasWriteAccess() && stream->openedOk())
                    {
                        // Write the sample to file
                        stream->setPosition(0);
                        stream->truncate();

                        WavAudioFormat wavFormat;
                        std::unique_ptr<AudioFormatWriter> formatWriter{ wavFormat.createWriterFor(
                            &*stream, processor.bufferSampleRate, processor.sampleBuffer.getNumChannels(), 
                            PluginParameters::STORED_BITRATE, {}, 0) };
                        formatWriter->writeFromAudioSampleBuffer(processor.sampleBuffer, 0, processor.sampleBuffer.getNumSamples());
                        stream.release();

                        const String& filename = file.getFullPathName();
                        processor.samplePath = filename;
                        processor.apvts.state.setPropertyExcludingListener(this, PluginParameters::FILE_PATH, filename, &processor.undoManager);
                        storeSampleToggle.setToggleState(false, dontSendNotification);
                        filenameComponent.setCurrentFile(file, true, dontSendNotification);
                        processor.usingFileReference = !storeSampleToggle.getToggleState();
                    }
                }, true);
            storeSampleToggle.setToggleState(true, dontSendNotification);
        }
        else
        {
            processor.usingFileReference = !storeSampleToggle.getToggleState();
        }
        };
    sampleRequiredControls.add(&storeSampleToggle);
    addAndMakeVisible(storeSampleToggle);

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
        if (!processor.shouldRecord)
        {
            if (processor.deviceManager.getCurrentAudioDevice() && processor.deviceManager.getCurrentAudioDevice()->getActiveInputChannels().countNumberOfSetBits())
            {
                recordingPrompt = false;
                processor.shouldRecord = true;
                sampleEditor.setRecordingMode(true);
                sampleNavigator.setRecordingMode(true);
                showPromptBackground({ &sampleEditor, &sampleNavigator, &recordButton });
            }
            else if (!recordingPrompt)
            {
                recordingPrompt = true;
                deviceSettingsButton.triggerClick();
            }
            else
            {
                recordingPrompt = false;
                recordButton.setToggleState(false, dontSendNotification);
            }
        }
        else
        {
            hidePromptBackground();
        }
        };
    addAndMakeVisible(recordButton);

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
    playbackOptions.setSelectedItemIndex(processor.p(PluginParameters::PLAYBACK_MODE));
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
    startTimerHz(PluginParameters::FRAME_RATE);
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
    topControls.items.add(FlexItem(storeSampleToggle).withMinWidth(15));
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
    // These are to deal with the parameter callbacks happening on the audio thread (from the processor's changes)
    if (needsResize)
    {
        needsResize = false;
        resized();
    }

    if (needsSampleUpdate)
    {
        updateWorkingSample(sampleUpdateShouldReset);
        needsSampleUpdate = false;
        sampleUpdateShouldReset = false;
    }

    // Update navigator and editor
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

    // Pitch detection UI changes
    if (boundsSelectRoutine) // little animation
    {
        magicPitchButtonShape.applyTransform(AffineTransform::rotation(MathConstants<float>::pi / 180));
        magicPitchButton.setShape(magicPitchButtonShape, false, true, false);
    }

    // Recording changes
    if (processor.isRecording)
    {
        RecordingBufferChange* bufferChange = processor.recordingBufferQueue.peek();

        // These variables are to deal with the GUI sample updates
        int previousSampleSize = recordingBufferSize;
        int previousBufferSize = pendingRecordingBuffer.getNumSamples();
        int iterations = 0;
        while (bufferChange && iterations < 5) // 5 is a safety measure
        {
            iterations++;
            switch (bufferChange->type)
            {
            case RecordingBufferChange::CLEAR:
                pendingRecordingBuffer.setSize(1, 0);
                recordingBufferSize = 0;
                break;
            case RecordingBufferChange::ADD:
                pendingRecordingBuffer.setSize(
                    1,
                    pendingRecordingBuffer.getNumSamples() < recordingBufferSize + bufferChange->addedBuffer.getNumSamples() ?
                    jmax<int>(recordingBufferSize + bufferChange->addedBuffer.getNumSamples(), 2 * pendingRecordingBuffer.getNumSamples()) :
                    pendingRecordingBuffer.getNumSamples(),
                    true, true, false
                );
                pendingRecordingBuffer.copyFrom(0, recordingBufferSize, bufferChange->addedBuffer, 0, 0, bufferChange->addedBuffer.getNumSamples());
                recordingBufferSize += bufferChange->addedBuffer.getNumSamples();
                break;
            }
            processor.recordingBufferQueue.pop();
            bufferChange = processor.recordingBufferQueue.peek();
        }
        if (previousBufferSize != pendingRecordingBuffer.getNumSamples())
        {
            sampleEditor.setSample(pendingRecordingBuffer, false);
            sampleNavigator.setSample(pendingRecordingBuffer, false);
        }
        else if (previousSampleSize != recordingBufferSize)
        {
            sampleEditor.sampleUpdated(previousSampleSize, recordingBufferSize);
            sampleNavigator.sampleUpdated(previousSampleSize, recordingBufferSize);
        }
    }
}

bool JustaSampleAudioProcessorEditor::isInterestedInFileDrag(const String& file)
{
    return processor.canLoadFileExtension(file) && !promptBackgroundVisible;
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
            processor.loadSampleAndReset(file);
            break;
        }
    }
}

void JustaSampleAudioProcessorEditor::valueTreePropertyChanged(ValueTree&, const Identifier& property)
{
    if (property.toString() == PluginParameters::FILE_PATH)
    {
        hidePromptBackground();
        needsSampleUpdate = true;
        sampleUpdateShouldReset = processor.resetParameters;
    }
}

void JustaSampleAudioProcessorEditor::updateWorkingSample(bool resetUI)
{
    if (processor.sampleBuffer.getNumSamples() > 0)
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
            storeSampleToggle.setEnabled(false);
        }
        storeSampleToggle.setToggleState(!processor.usingFileReference, dontSendNotification);
        sampleEditor.setSample(processor.sampleBuffer, resetUI);
        sampleNavigator.setSample(processor.sampleBuffer, resetUI);
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
    needsResize = true;
}

void JustaSampleAudioProcessorEditor::hidePromptBackground()
{
    promptBackgroundVisible = false;
    promptBackground.setVisible(false);
    onPromptExit();
    needsResize = true;
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
        if (recordingPrompt)
            recordButton.onClick();
    }
    else if (processor.shouldRecord)
    {
        processor.shouldRecord = false;
        sampleEditor.setRecordingMode(false);
        sampleNavigator.setRecordingMode(false);
        recordButton.setToggleState(false, dontSendNotification);
    }
}

void JustaSampleAudioProcessorEditor::filenameComponentChanged(FilenameComponent* fileComponentThatHasChanged)
{
    File file = fileComponentThatHasChanged->getCurrentFile();
    bool fileLoaded = processor.loadSampleAndReset(file.getFullPathName());
    if (fileLoaded)
    {
        processor.pv(PluginParameters::RECENT_FILES) = fileComponentThatHasChanged->getRecentlyUsedFilenames();
    }
    else
    {
        fileComponentThatHasChanged->setCurrentFile(File(processor.p(PluginParameters::FILE_PATH)), false, dontSendNotification);
        auto recentFiles = fileComponentThatHasChanged->getRecentlyUsedFilenames();
        recentFiles.removeString(file.getFullPathName());
        fileComponentThatHasChanged->setRecentlyUsedFilenames(recentFiles);
    }
}

void JustaSampleAudioProcessorEditor::parameterChanged(const String& parameterID, float newValue)
{
    if (parameterID == PluginParameters::MASTER_GAIN)
    {
        float gainValue = Decibels::decibelsToGain(newValue);
        sampleEditor.setGain(gainValue);
        sampleNavigator.setGain(gainValue);
    }
}

void JustaSampleAudioProcessorEditor::addListeningParameters(std::vector<String> parameters)
{
    for (const auto& param : parameters)
    {
        processor.apvts.addParameterListener(param, this);
        listeningParameters.push_back(param);
    }
}

/*
  ==============================================================================

    PluginProcessor.cpp
    Created: 5 Sep 2023
    Author:  binya

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

JustaSampleAudioProcessorEditor::JustaSampleAudioProcessorEditor(JustaSampleAudioProcessor& processor)
    : AudioProcessorEditor(&processor), p(processor), synthVoices(p.getSamplerVoices()), lnf(dynamic_cast<CustomLookAndFeel&>(getLookAndFeel())),
    sampleEditor(p.APVTS(), synthVoices),
    sampleNavigator(p.APVTS(), synthVoices),
    semitoneSliderAttachment(p.APVTS(), PluginParameters::SEMITONE_TUNING, semitoneSlider),
    centSliderAttachment(p.APVTS(), PluginParameters::CENT_TUNING, centSlider),
    playbackOptionsAttachment(p.APVTS(), PluginParameters::PLAYBACK_MODE, playbackOptions),
    loopToggleButtonAttachment(p.APVTS(), PluginParameters::IS_LOOPING, isLoopingButton),
    masterGainSliderAttachment(p.APVTS(), PluginParameters::MASTER_GAIN, masterGainSlider),
    audioDeviceSettings(p.getDeviceManager(), 0, 2, 0, 0, false, false, true, false),
    filenameComponent("File_Chooser", {}, true, false, false, p.getWildcardFilter(), "", "Select a file to load..."),
    storeSampleToggle("Store_File", Colours::white, Colours::lightgrey, Colours::darkgrey),
    magicPitchButton("Detect_Pitch", Colours::white, Colours::lightgrey, Colours::darkgrey),
    recordButton("Record_Sound", Colours::white, Colours::lightgrey, Colours::darkgrey),
    deviceSettingsButton("Device_Settings", Colours::white, Colours::lightgrey, Colours::darkgrey),
    haltButton("Halt_Sound", Colours::white, Colours::lightgrey, Colours::darkgrey),
    fxChain(p)
{
    // This is to fix a rendering issue found with Reaper
    if (hostType.isReaper())
        openGLContext.attachTo(*getTopLevelComponent());

    p.APVTS().state.addListener(this);

    // Set the plugin sizing
    int width = p.sp(PluginParameters::WIDTH);
    int height = p.sp(PluginParameters::HEIGHT);
    if (250 > width || width > 1000 || 200 > height || height > 800)
        setSize(500, 400);
    else
        setSize(width, height);
    setResizeLimits(250, 200, 1000, 800);
    setResizable(true, false);
    
    auto recentFiles{ p.sp(PluginParameters::RECENT_FILES) };
    for (int i = recentFiles.size() - 1; i >= 0; i--)
        filenameComponent.addRecentlyUsedFile(File(recentFiles[i]));
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
        if (!p.sp(PluginParameters::USING_FILE_REFERENCE) && 
            (p.sp(PluginParameters::FILE_PATH).toString().isEmpty() || !juce::File(p.sp(PluginParameters::FILE_PATH)).existsAsFile()))  // If the file doesn't exist, prompt the user to save it
        {
            p.openFileChooser("Save the sample to a file",
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
                            &*stream, p.getBufferSampleRate(), p.getSampleBuffer().getNumChannels(),
                            PluginParameters::STORED_BITRATE, {}, 0) };
                        formatWriter->writeFromAudioSampleBuffer(p.getSampleBuffer(), 0, p.getSampleBuffer().getNumSamples());
                        stream.release();

                        const String& filename = file.getFullPathName();
                        p.spv(PluginParameters::FILE_PATH) = filename;
                        storeSampleToggle.setToggleState(false, dontSendNotification);
                        filenameComponent.setCurrentFile(file, true, dontSendNotification);
                        p.spv(PluginParameters::USING_FILE_REFERENCE) = !storeSampleToggle.getToggleState();
                    }
                }, true);
            storeSampleToggle.setToggleState(true, dontSendNotification);
        }
        else
        {
            p.spv(PluginParameters::USING_FILE_REFERENCE) = !storeSampleToggle.getToggleState();
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
        if (!p.getRecorder().isRecordingDevice())
        {
            if (p.getDeviceManager().getCurrentAudioDevice() && p.getDeviceManager().getCurrentAudioDevice()->getActiveInputChannels().countNumberOfSetBits())
            {
                recordingPrompt = false;
                p.getRecorder().startRecording();
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
    playbackOptions.setSelectedItemIndex(p.p(PluginParameters::PLAYBACK_MODE));
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
        p.haltVoices();
        };
    sampleRequiredControls.add(&haltButton);
    addAndMakeVisible(haltButton);

    addAndMakeVisible(fxChain);

    sampleEditor.addBoundsSelectListener(this);
    addAndMakeVisible(sampleEditor);
    addAndMakeVisible(sampleNavigator);

    promptBackground.setAlpha(0.35f);
    promptBackground.setInterceptsMouseClicks(true, false);
    addAndMakeVisible(promptBackground);
    
    addChildComponent(audioDeviceSettings);

    tooltipWindow.setAlwaysOnTop(true);
    addAndMakeVisible(tooltipWindow);

    setSampleControlsEnabled(false);
    loadSample(true);
    setWantsKeyboardFocus(true);
    addMouseListener(this, true);
    startTimerHz(PluginParameters::FRAME_RATE);
}

JustaSampleAudioProcessorEditor::~JustaSampleAudioProcessorEditor()
{
    sampleEditor.removeBoundsSelectListener(this);
    p.APVTS().state.removeListener(this);
}

void JustaSampleAudioProcessorEditor::valueTreePropertyChanged(ValueTree&, const Identifier& property)
{
    if (property.toString() == PluginParameters::FILE_PATH)
    {
        filenameChanged = true;
    }
}

//==============================================================================
void JustaSampleAudioProcessorEditor::timerCallback()
{
    // Resize if called for
    if (layoutDirty)
    {
        layoutDirty = false;
        resized();
    }

    // Update the sample if necessary
    if (p.sp(PluginParameters::SAMPLE_HASH) != expectedHash)
    {
        loadSample();
    }

    if (filenameChanged)
    {
        filenameChanged = false;
        filenameComponent.setCurrentFile(File(p.sp(PluginParameters::FILE_PATH)), false, dontSendNotification);
    }

    // If playback state has changed, update the sampleEditor and sampleNavigator
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

    // Handle recording changes
    if (p.getRecorder().isRecordingDevice())
    {
        auto& recordingBuffer = p.getRecorder().getRecordingBufferQueue();
        RecordingBufferChange* bufferChange = recordingBuffer.peek();

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
            recordingBuffer.pop();
            bufferChange = recordingBuffer.peek();
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

void JustaSampleAudioProcessorEditor::paint(Graphics& g)
{
    g.fillAll(lnf.BACKGROUND_COLOR);
}

void JustaSampleAudioProcessorEditor::resized()
{
    p.spv(PluginParameters::WIDTH) = getWidth();
    p.spv(PluginParameters::HEIGHT) = getHeight();

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

void JustaSampleAudioProcessorEditor::loadSample(bool initialLoad)
{
    setSampleControlsEnabled(p.getSampleBuffer().getNumSamples());
    if (p.getSampleBuffer().getNumSamples())
    {
        expectedHash = p.sp(PluginParameters::SAMPLE_HASH);
        setSampleControlsEnabled(true);
        storeSampleToggle.setEnabled(!p.sampleBufferNeedsReference());
        storeSampleToggle.setToggleState(!p.sp(PluginParameters::USING_FILE_REFERENCE), dontSendNotification);
        sampleEditor.setSample(p.getSampleBuffer(), initialLoad);
        sampleNavigator.setSample(p.getSampleBuffer(), initialLoad);
    }
}

void JustaSampleAudioProcessorEditor::setSampleControlsEnabled(bool enablement)
{
    for (Component* comp : sampleRequiredControls)
    {
        if (comp)
        {
            comp->setEnabled(enablement);
        }
    }
}

//==============================================================================
bool JustaSampleAudioProcessorEditor::isInterestedInFileDrag(const String& file)
{
    return p.canLoadFileExtension(file) && !promptBackgroundVisible;
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
            p.loadSampleFromPath(file);
            break;
        }
    }
}

void JustaSampleAudioProcessorEditor::boundsSelected(int startSample, int endSample)
{
    hidePromptBackground();
    p.startPitchDetectionRoutine(startSample, endSample);
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
    layoutDirty = true;
}

void JustaSampleAudioProcessorEditor::hidePromptBackground()
{
    promptBackgroundVisible = false;
    promptBackground.setVisible(false);
    onPromptExit();
    layoutDirty = true;
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
    else if (p.getRecorder().isRecordingDevice())
    {
        p.getRecorder().stopRecording();
        sampleEditor.setRecordingMode(false);
        sampleNavigator.setRecordingMode(false);
        recordButton.setToggleState(false, dontSendNotification);
    }
}

void JustaSampleAudioProcessorEditor::filenameComponentChanged(FilenameComponent* fileComponentThatHasChanged)
{
    File file = fileComponentThatHasChanged->getCurrentFile();
    bool fileLoaded = p.loadSampleFromPath(file.getFullPathName());
    if (fileLoaded)
    {
        p.spv(PluginParameters::RECENT_FILES) = fileComponentThatHasChanged->getRecentlyUsedFilenames();
    }
    else
    {
        fileComponentThatHasChanged->setCurrentFile(File(p.sp(PluginParameters::FILE_PATH)), false, dontSendNotification);
        auto recentFiles = fileComponentThatHasChanged->getRecentlyUsedFilenames();
        recentFiles.removeString(file.getFullPathName());
        fileComponentThatHasChanged->setRecentlyUsedFilenames(recentFiles);
    }
}

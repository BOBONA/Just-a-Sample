/*
  ==============================================================================

    PluginProcessor.cpp
    Created: 5 Sep 2023
    Author:  binya

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "Components/Paths.h"

JustaSampleAudioProcessorEditor::JustaSampleAudioProcessorEditor(JustaSampleAudioProcessor& processor)
    : AudioProcessorEditor(&processor), p(processor), pluginState(p.getPluginState()), synthVoices(p.getSamplerVoices()), lnf(dynamic_cast<CustomLookAndFeel&>(getLookAndFeel())),
    audioDeviceSettings(p.getDeviceManager(), 0, 2, 0, 0, false, false, true, false),

    // Toolbar
    filenameComponent("File_Chooser", {}, true, false, false, p.getWildcardFilter(), "", "Select a file to load..."),
    storeSampleToggle("Store_File", juce::Colours::white, juce::Colours::lightgrey, juce::Colours::darkgrey),
    magicPitchButton("Detect_Pitch", juce::Colours::white, juce::Colours::lightgrey, juce::Colours::darkgrey),
    recordButton("Record_Sound", juce::Colours::white, juce::Colours::lightgrey, juce::Colours::darkgrey),
    deviceSettingsButton("Device_Settings", juce::Colours::white, juce::Colours::lightgrey, juce::Colours::darkgrey),
    haltButton("Halt_Sound", juce::Colours::white, juce::Colours::lightgrey, juce::Colours::darkgrey),

    // Main controls
    sampleNavigator(p.APVTS(), p.getPluginState(), synthVoices),
    sampleEditor(p.APVTS(), p.getPluginState(), synthVoices, 
        [this](const juce::MouseWheelDetails& details, int center) -> void { sampleNavigator.scrollView(details, center, true); }),
    fxChain(p),

    // Attachments
    semitoneSliderAttachment(p.APVTS(), PluginParameters::SEMITONE_TUNING, semitoneSlider),
    centSliderAttachment(p.APVTS(), PluginParameters::CENT_TUNING, centSlider),
    playbackOptionsAttachment(p.APVTS(), PluginParameters::PLAYBACK_MODE, playbackOptions),
    loopToggleButtonAttachment(p.APVTS(), PluginParameters::IS_LOOPING, isLoopingButton),
    masterGainSliderAttachment(p.APVTS(), PluginParameters::MASTER_GAIN, masterGainSlider)
{
    // This is to fix a rendering issue found with Reaper
#if !JUCE_DEBUG
    if (hostType.isReaper())
        openGLContext.attachTo(*getTopLevelComponent());
#endif

    // Set the plugin sizing
    int width = pluginState.width;
    int height = pluginState.height;
    if (250 > width || width > 1000 || 200 > height || height > 800)
        setSize(500, 400);
    else
        setSize(width, height);
    setResizable(true, false);
    setResizeLimits(250, 200, 1000, 800);
    
    // Recent files
    juce::StringArray recentFiles{ pluginState.recentFiles };
    for (int i = recentFiles.size() - 1; i >= 0; i--)
        filenameComponent.addRecentlyUsedFile(juce::File(recentFiles[i]));
    filenameComponent.addListener(this);
    addAndMakeVisible(filenameComponent);

    juce::Path storeIcon;
    storeIcon.loadPathFromData(PathData::saveIcon, sizeof(PathData::saveIcon));
    storeIcon.scaleToFit(0, 0, 13, 13, true);
    storeSampleToggle.setShape(storeIcon, true, true, false);
    storeSampleToggle.setClickingTogglesState(true);
    storeSampleToggle.shouldUseOnColours(true);
    storeSampleToggle.setOnColours(juce::Colours::darkgrey, juce::Colours::lightgrey, juce::Colours::white);
    storeSampleToggle.onClick = [this] { toggleStoreSample(); };
    sampleRequiredControls.add(&storeSampleToggle);
    addAndMakeVisible(storeSampleToggle);

    // Tuning
    tuningLabel.setText("Tuning: ", juce::dontSendNotification);
    addAndMakeVisible(tuningLabel);

    semitoneSlider.setSliderStyle(juce::Slider::LinearBar);
    sampleRequiredControls.add(&semitoneSlider);
    addAndMakeVisible(semitoneSlider);
    
    centSlider.setSliderStyle(juce::Slider::LinearBar);
    sampleRequiredControls.add(&centSlider);
    addAndMakeVisible(centSlider);

    magicPitchButtonShape.addStar(juce::Point(15.f, 15.f), 5, 8.f, 3.f, 0.45f);
    magicPitchButton.setShape(magicPitchButtonShape, true, true, false);
    magicPitchButton.onClick = [this] { promptPitchDetection(); };
    sampleRequiredControls.add(&magicPitchButton);
    addAndMakeVisible(magicPitchButton);

    // Recording
    juce::Path recordIcon;
    recordIcon.loadPathFromData(PathData::recordIcon, sizeof(PathData::recordIcon));
    recordIcon.scaleToFit(0, 0, 13, 13, true);
    recordButton.setShape(recordIcon, true, true, false);
    recordButton.setClickingTogglesState(true);
    recordButton.shouldUseOnColours(true);
    recordButton.setOnColours(juce::Colours::darkgrey, juce::Colours::lightgrey, juce::Colours::white);
    recordButton.onClick = [this] { startRecording(true); };
    addAndMakeVisible(recordButton);

    juce::Path deviceSettingsIcon;
    deviceSettingsIcon.loadPathFromData(PathData::settingsIcon, sizeof(PathData::settingsIcon));
    deviceSettingsIcon.scaleToFit(0, 0, 6, 6, true);
    deviceSettingsButton.setShape(deviceSettingsIcon, true, true, false);
    deviceSettingsButton.onClick = [this] { prompt.openPrompt({ &audioDeviceSettings }); };
    addAndMakeVisible(deviceSettingsButton);
    addChildComponent(audioDeviceSettings);
    audioDeviceSettings.setAlwaysOnTop(true);

    // Playback options
    playbackOptions.addItemList(PluginParameters::PLAYBACK_MODE_LABELS, 1);
    playbackOptions.setSelectedItemIndex(p.p(PluginParameters::PLAYBACK_MODE));
    sampleRequiredControls.add(&playbackOptions);
    addAndMakeVisible(playbackOptions);

    isLoopingLabel.setText("Loop:", juce::dontSendNotification);
    addAndMakeVisible(isLoopingLabel);

    sampleRequiredControls.add(&isLoopingButton);
    addAndMakeVisible(isLoopingButton);

    masterGainSlider.setSliderStyle(juce::Slider::LinearBar);
    masterGainSlider.setTextValueSuffix(" db");
    addAndMakeVisible(masterGainSlider);

    // Halt voices
    juce::Path stopPath;
    stopPath.loadPathFromData(PathData::stopIcon, sizeof(PathData::stopIcon));
    stopPath.scaleToFit(0, 0, 13, 13, true);
    haltButton.setShape(stopPath, true, true, false);
    haltButton.onClick = [this] { p.haltVoices(); };
    sampleRequiredControls.add(&haltButton);
    addAndMakeVisible(haltButton);

    // Main controls
    addAndMakeVisible(sampleEditor);
    addAndMakeVisible(sampleLoader);
    addAndMakeVisible(sampleNavigator);
    addAndMakeVisible(fxChain);

    addAndMakeVisible(prompt);

    tooltipWindow.setAlwaysOnTop(true);
    addAndMakeVisible(tooltipWindow);

    setSampleControlsEnabled(false);
    setWantsKeyboardFocus(true);
    addMouseListener(this, true);
    startTimerHz(PluginParameters::FRAME_RATE);
}

JustaSampleAudioProcessorEditor::~JustaSampleAudioProcessorEditor() = default;

//==============================================================================
void JustaSampleAudioProcessorEditor::timerCallback()
{
    // Update the sample if necessary
    if (pluginState.sampleHash != expectedHash)
    {
        loadSample();

        // The file path should be updated by this point, in which case the filenameComponent should be updated
        if (filenameComponent.getCurrentFileText() != pluginState.filePath)
        {
            filenameComponent.setCurrentFile(juce::File(pluginState.filePath), true, juce::dontSendNotification);
            pluginState.recentFiles = filenameComponent.getRecentlyUsedFilenames();
        }
    }

    // Handle loading state
    sampleLoader.setLoading(p.getSampleLoader().isLoading());
    recordButton.setEnabled(!sampleLoader.isLoading());
    magicPitchButton.setEnabled(!sampleLoader.isLoading());
    sampleLoader.setVisible(!bool(p.getSampleBuffer().getNumSamples()) || p.getSampleLoader().isLoading());

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
        sampleEditor.repaint();
        sampleNavigator.repaint();
    }

    // Little animation if detecting pitch
    if (sampleEditor.isInBoundsSelection())
    {
        magicPitchButtonShape.applyTransform(juce::AffineTransform::rotation(juce::MathConstants<float>::pi / 180));
        magicPitchButton.setShape(magicPitchButtonShape, false, true, false);
    }

    // Handle recording changes
    if (p.getRecorder().isRecordingDevice())
    {
        if (!sampleEditor.isRecordingMode())
        {
            sampleLoader.setVisible(false);
            sampleEditor.setRecordingMode(true);
            sampleNavigator.setRecordingMode(true);
            prompt.openPrompt({}, [this] {
                p.getRecorder().stopRecording();
                sampleLoader.setVisible(!p.getSampleBuffer().getNumSamples());
                sampleEditor.setRecordingMode(false);
                sampleNavigator.setRecordingMode(false);
                recordButton.setToggleState(false, juce::dontSendNotification);
                }, { &sampleEditor, &sampleNavigator, &sampleLoader });
        }
        handleActiveRecording();
    }
    else if (sampleEditor.isRecordingMode())
    {
        prompt.closePrompt();
    }
}

void JustaSampleAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(lnf.BACKGROUND_COLOR);
}

void JustaSampleAudioProcessorEditor::resized()
{
    pluginState.width = getWidth();
    pluginState.height = getHeight();

    auto bounds = getLocalBounds();

    prompt.setBounds(bounds);

    audioDeviceSettings.setBounds(bounds.reduced(juce::jmin<int>(bounds.getWidth(), bounds.getHeight()) / 4));

    juce::FlexBox topControls{
        juce::FlexBox::Direction::row, juce::FlexBox::Wrap::wrap, juce::FlexBox::AlignContent::stretch,
        juce::FlexBox::AlignItems::stretch, juce::FlexBox::JustifyContent::flexEnd };
    topControls.items.add(juce::FlexItem(filenameComponent).withFlex(1).withMinWidth(float(getWidth() - 15.f)));
    topControls.items.add(juce::FlexItem(storeSampleToggle).withMinWidth(15));
    topControls.items.add(juce::FlexItem(tuningLabel).withMinWidth(50));
    topControls.items.add(juce::FlexItem(semitoneSlider).withMinWidth(30));
    topControls.items.add(juce::FlexItem(centSlider).withMinWidth(40));
    topControls.items.add(juce::FlexItem(magicPitchButton).withMinWidth(15));
    topControls.items.add(juce::FlexItem(recordButton).withMinWidth(15));
    topControls.items.add(juce::FlexItem(deviceSettingsButton).withMinWidth(6));
    topControls.items.add(juce::FlexItem(playbackOptions).withMinWidth(150));
    topControls.items.add(juce::FlexItem(isLoopingLabel).withMinWidth(35));
    topControls.items.add(juce::FlexItem(isLoopingButton).withMinWidth(20));
    topControls.items.add(juce::FlexItem(masterGainSlider).withMinWidth(60));
    topControls.items.add(juce::FlexItem(haltButton).withMinWidth(15));
    float totalWidth = 0;
    for (const juce::FlexItem& item : topControls.items)
    {
        totalWidth += item.minWidth;
    }
    auto top = bounds.removeFromTop(15 * int(ceilf(totalWidth / getWidth())));
    topControls.performLayout(top);

    auto editor = bounds.removeFromTop(int(bounds.getHeight() * 0.66f));
    auto navigator = bounds.removeFromTop(int(bounds.getHeight() * 0.2f));

    sampleLoader.setBounds(editor);
    sampleEditor.setBounds(editor);
    sampleNavigator.setBounds(navigator);
    fxChain.setBounds(bounds);
}

//==============================================================================
void JustaSampleAudioProcessorEditor::loadSample()
{
    bool sampleLoaded = p.getSampleBuffer().getNumSamples();
    setSampleControlsEnabled(sampleLoaded);
    sampleLoader.setVisible(!sampleLoaded);
    if (sampleLoaded)
    {
        expectedHash = pluginState.sampleHash;
        setSampleControlsEnabled(true);
        storeSampleToggle.setEnabled(!p.sampleBufferNeedsReference());
        storeSampleToggle.setToggleState(!pluginState.usingFileReference, juce::dontSendNotification);
        sampleEditor.setSample(p.getSampleBuffer(), p.isInitialSampleLoad());
        sampleNavigator.setSample(p.getSampleBuffer(), p.isInitialSampleLoad());
    }
}

void JustaSampleAudioProcessorEditor::handleActiveRecording()
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
                pendingRecordingBuffer.getNumSamples() < recordingBufferSize + bufferChange->addedBuffer.getNumSamples() ? juce::jmax<int>(recordingBufferSize + bufferChange->addedBuffer.getNumSamples(), 2 * pendingRecordingBuffer.getNumSamples()) :
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
    if (pendingRecordingBuffer.getNumSamples() != previousBufferSize)  // Update when the buffer size changes
    {
        sampleEditor.setSample(pendingRecordingBuffer, false);
        sampleNavigator.setSample(pendingRecordingBuffer, false);
    } 
    else if (previousSampleSize != recordingBufferSize)  // Update the rendered paths when more data is added to the buffer (not changing the size)
    {
        sampleEditor.sampleUpdated(previousSampleSize, recordingBufferSize);
        sampleNavigator.sampleUpdated(previousSampleSize, recordingBufferSize);
    }
}

void JustaSampleAudioProcessorEditor::toggleStoreSample()
{
    // If the file path is empty or the file doesn't exist, prompt the user to save the sample, otherwise toggle the parameter
    if (!pluginState.usingFileReference && (pluginState.filePath.load().isEmpty() || !juce::File(pluginState.filePath).existsAsFile()))
    {
        p.openFileChooser("Save the sample to a file",
                          juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles, [this](const juce::FileChooser& chooser) -> void {
                              juce::File file = chooser.getResult();
                auto stream = std::make_unique<juce::FileOutputStream>(file);
                if (file.hasWriteAccess() && stream->openedOk())
                {
                    // Write the sample to file
                    stream->setPosition(0);
                    stream->truncate();

                    juce::WavAudioFormat wavFormat;
                    std::unique_ptr<juce::AudioFormatWriter> formatWriter{ wavFormat.createWriterFor(
                        &*stream, p.getBufferSampleRate(), p.getSampleBuffer().getNumChannels(),
                        PluginParameters::STORED_BITRATE, {}, 0) };
                    formatWriter->writeFromAudioSampleBuffer(p.getSampleBuffer(), 0, p.getSampleBuffer().getNumSamples());
                    stream.release();

                    const juce::String& filename = file.getFullPathName();
                    pluginState.filePath = filename;
                    storeSampleToggle.setToggleState(false, juce::dontSendNotification);
                    filenameComponent.setCurrentFile(file, true, juce::dontSendNotification);
                    pluginState.usingFileReference = !storeSampleToggle.getToggleState();
                }
            }, true);
        storeSampleToggle.setToggleState(true, juce::dontSendNotification);
    }
    else
    {
        pluginState.usingFileReference = !storeSampleToggle.getToggleState();
    }
}

void JustaSampleAudioProcessorEditor::startRecording(bool promptSettings)
{
    if (p.getDeviceManager().getCurrentAudioDevice() && p.getDeviceManager().getCurrentAudioDevice()->getActiveInputChannels().countNumberOfSetBits())
        p.getRecorder().startRecording();
    else if (promptSettings)
        prompt.openPrompt({ &audioDeviceSettings }, [this] { startRecording(false); });
    else
        recordButton.setToggleState(false, juce::dontSendNotification);
}

void JustaSampleAudioProcessorEditor::promptPitchDetection()
{
    if (!sampleEditor.isInBoundsSelection() && playbackOptions.isEnabled())
    {
        // This changes the state until a portion of the sample is selected, afterward the pitch detection will happen
        magicPitchButton.setEnabled(false);
        magicPitchButton.setColours(juce::Colours::white, juce::Colours::white, juce::Colours::white);
        prompt.openPrompt({}, [this] {
            sampleEditor.cancelBoundsSelection();
            magicPitchButton.setEnabled(true);
            magicPitchButton.setColours(juce::Colours::white, juce::Colours::lightgrey, juce::Colours::darkgrey);
        }, { &sampleEditor, &sampleNavigator, &sampleLoader });
        sampleEditor.promptBoundsSelection("Drag to select a portion of the sample to analyze.",  [this](int startPos, int endPos) -> void {
            p.startPitchDetectionRoutine(startPos, endPos);
            prompt.closePrompt();
        });
    }
}

void JustaSampleAudioProcessorEditor::setSampleControlsEnabled(bool enablement)
{
    for (Component* comp : sampleRequiredControls)
        if (comp)
            comp->setEnabled(enablement);
}

//==============================================================================
bool JustaSampleAudioProcessorEditor::isInterestedInFileDrag(const juce::String& file)
{
    return p.canLoadFileExtension(file) && !prompt.isPromptVisible();
}

bool JustaSampleAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const juce::String& file : files)
        if (isInterestedInFileDrag(file))
            return true;
    return false;
}

void JustaSampleAudioProcessorEditor::filesDropped(const juce::StringArray& files, int, int)
{
    for (const juce::String& file : files)
    {
        if (isInterestedInFileDrag(file))
        {
            p.loadSampleFromPath(file);
            break;
        }
    }
}

void JustaSampleAudioProcessorEditor::filenameComponentChanged(juce::FilenameComponent* fileComponentThatHasChanged)
{
    juce::File file = fileComponentThatHasChanged->getCurrentFile();
    p.loadSampleFromPath(file.getFullPathName(), true, "", false, [this, fileComponentThatHasChanged, file](bool fileLoaded) -> void
    {
        if (!fileLoaded)
        {
            fileComponentThatHasChanged->setCurrentFile(juce::File(pluginState.filePath), false, juce::dontSendNotification);
            auto recentFiles = fileComponentThatHasChanged->getRecentlyUsedFilenames();
            recentFiles.removeString(file.getFullPathName());
            fileComponentThatHasChanged->setRecentlyUsedFilenames(recentFiles);
        }
    });
}
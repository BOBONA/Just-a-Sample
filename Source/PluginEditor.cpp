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
    : AudioProcessorEditor(&processor), p(processor), pluginState(p.getPluginState()), synthVoices(p.getSamplerVoices()),
    // Modules
    tuningLabel("", "Tuning"),
    attackLabel("", "Attack"),
    releaseLabel("", "Release"),
    playbackLabel("", "Playback"),
    loopingLabel("", "Loop"),
    masterLabel("", "Master"),

    // Tuning module
    semitoneSliderAttachment(p.APVTS(), PluginParameters::SEMITONE_TUNING, semitoneRotary),
    centSliderAttachment(p.APVTS(), PluginParameters::CENT_TUNING, centRotary),

    tuningDetectLabel("", "Detect"),
    tuningDetectButton("", Colors::DARK, Colors::DARK, Colors::DARK),

    // Attack and release modules
    attackTimeAttachment(p.APVTS(), PluginParameters::ATTACK, attackTimeRotary),
    attackCurveAttachment(p.APVTS(), PluginParameters::ATTACK_SHAPE, attackCurve),
    releaseTimeAttachment(p.APVTS(), PluginParameters::RELEASE, releaseTimeRotary),
    releaseCurveAttachment(p.APVTS(), PluginParameters::RELEASE_SHAPE, releaseCurve),

    // Playback module
    lofiModeButton(Colors::DARKER_SLATE, Colors::WHITE),
    lofiModeAttachment(p.APVTS(), PluginParameters::SKIP_ANTIALIASING, lofiModeButton),
    playbackModeButton(p.APVTS(), PluginParameters::PLAYBACK_MODE, Colors::SLATE, Colors::WHITE, "Basic", "Bungee"),
    playbackSpeedAttachment(p.APVTS(), PluginParameters::SPEED_FACTOR, playbackSpeedRotary),

    // Loop module
    loopButton(Colors::DARKER_SLATE, Colors::LOOP, true),
    loopStartButton(Colors::DARKER_SLATE, Colors::LOOP, true),
    loopEndButton(Colors::DARKER_SLATE, Colors::LOOP, true),
    loopAttachment(p.APVTS(), PluginParameters::IS_LOOPING, loopButton),
    loopStartAttachment(p.APVTS(), PluginParameters::LOOPING_HAS_START, loopStartButton),
    loopEndAttachment(p.APVTS(), PluginParameters::LOOPING_HAS_END, loopEndButton),

    // Master module
    monoOutputButton(Colors::DARKER_SLATE, Colors::WHITE),
    monoOutputAttachment(p.APVTS(), PluginParameters::MONO_OUTPUT, monoOutputButton),
    gainSliderAttachment(p.APVTS(), PluginParameters::MASTER_GAIN, gainSlider),

    // Toolbar
    filenameComponent("File_Chooser", {}, true, false, false, p.getWildcardFilter(), "", "Select a file to load..."),
    storeSampleToggle("Store_File", juce::Colours::white, juce::Colours::lightgrey, juce::Colours::darkgrey),
    recordButton("Record_Sound", juce::Colours::white, juce::Colours::lightgrey, juce::Colours::darkgrey),
    deviceSettingsButton("Device_Settings", juce::Colours::white, juce::Colours::lightgrey, juce::Colours::darkgrey),
    audioDeviceSettings(p.getDeviceManager(), 0, 2, 0, 0, false, false, true, false),
    haltButton("Halt_Sound", juce::Colours::white, juce::Colours::lightgrey, juce::Colours::darkgrey),

    // Main controls
    sampleNavigator(p.APVTS(), p.getPluginState(), synthVoices),
    sampleEditor(p.APVTS(), p.getPluginState(), synthVoices, 
        [this](const juce::MouseWheelDetails& details, int center) -> void { sampleNavigator.scrollView(details, center, true); }),
    fxChain(p),

    // Attachments
    playbackOptionsAttachment(p.APVTS(), PluginParameters::PLAYBACK_MODE, (playbackOptions.addItemList(PluginParameters::PLAYBACK_MODE_LABELS, 1), playbackOptions)),

    lnf(dynamic_cast<CustomLookAndFeel&>(getLookAndFeel()))
{
    // Set the plugin sizing
    int width = pluginState.width;
    int height = pluginState.height;
    if (250 > width || width > 1000 || 200 > height || height > 800)
        setSize(500, 400);
    else
        setSize(width, height);
    setResizable(true, false);
    setResizeLimits(250, 200, 1000, 800);

    // Controls toolbar
    juce::Array moduleLabels{ &tuningLabel, &tuningDetectLabel, &attackLabel, &releaseLabel, &playbackLabel, &loopingLabel, &masterLabel };
    for (juce::Label* label : moduleLabels)
    {
        label->setJustificationType(juce::Justification::centred);
        label->setColour(juce::Label::textColourId, Colors::DARK);
        addAndMakeVisible(label);
    }

    std::function convertWithSecondaryUnit = [](double value) {
        if (value >= 1000)
            return juce::String(value / 1000.f, 1);
        return juce::String(juce::roundToInt(value));
    };

    semitoneRotary.getProperties().set(ComponentProps::LABEL_UNIT, "sm");
    centRotary.getProperties().set(ComponentProps::LABEL_UNIT, "%");

    tuningDetectIcon = getOutlineFromSVG(BinaryData::IconDetect_svg);
    tuningDetectButton.setShape(tuningDetectIcon, false, true, false);
    tuningDetectButton.onClick = [this] { promptPitchDetection(); };
    tuningDetectButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible(&tuningDetectButton);

    attackTimeRotary.getProperties().set(ComponentProps::LABEL_UNIT, "ms");
    attackTimeRotary.getProperties().set(ComponentProps::GREATER_UNIT, "sec");
    attackTimeRotary.textFromValueFunction = convertWithSecondaryUnit;
    attackTimeRotary.updateText();

    attackCurve.setLookAndFeel(&attackCurveLNF);
    attackCurve.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    attackCurve.setMouseDragSensitivity(100);
    attackCurve.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    attackCurve.setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    addAndMakeVisible(&attackCurve);

    releaseTimeRotary.getProperties().set(ComponentProps::LABEL_UNIT, "ms");
    releaseTimeRotary.getProperties().set(ComponentProps::GREATER_UNIT, "sec");
    releaseTimeRotary.textFromValueFunction = convertWithSecondaryUnit;
    releaseTimeRotary.updateText();

    releaseCurve.setLookAndFeel(&releaseCurveLNF);
    releaseCurve.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    releaseCurve.setMouseDragSensitivity(100);
    releaseCurve.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    releaseCurve.setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    addAndMakeVisible(&releaseCurve);

    lofiModeButton.useShape(getOutlineFromSVG(BinaryData::IconLofi_svg));
    addAndMakeVisible(&lofiModeButton);

    addAndMakeVisible(&playbackModeButton);

    playbackSpeedRotary.getProperties().set(ComponentProps::LABEL_ICON, new ReferenceCountedPath(getOutlineFromSVG(BinaryData::IconSpeed_svg)));

    loopButton.useShape(getOutlineFromSVG(BinaryData::IconLoop_svg));
    addAndMakeVisible(&loopButton);

    loopStartButton.useShape(getOutlineFromSVG(BinaryData::IconLoopStart_svg));
    loopStartButton.setOwnerButton(&loopButton);
    addAndMakeVisible(&loopStartButton);

    juce::Path loopEndIcon{ getOutlineFromSVG(BinaryData::IconLoopStart_svg) };
    loopEndIcon.applyTransform(juce::AffineTransform::scale(-1, 1));
    loopEndButton.useShape(loopEndIcon);
    loopEndButton.setOwnerButton(&loopButton);
    addAndMakeVisible(&loopEndButton);

    monoOutputButton.useShape(getOutlineFromSVG(BinaryData::IconMono_svg));
    addAndMakeVisible(&monoOutputButton);

    gainSlider.setLookAndFeel(&gainSliderLNF);
    gainSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 0, 0);
    gainSlider.setTextValueSuffix(" db");
    addAndMakeVisible(&gainSlider);

    rotaries = { &semitoneRotary, &centRotary, &attackTimeRotary, &releaseTimeRotary, &playbackSpeedRotary };
    for (auto rotary : rotaries)
    {
        rotary->setSliderStyle(juce::Slider::RotaryVerticalDrag);
        rotary->setMouseDragSensitivity(150);
        rotary->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 0, 0);
        rotary->addMouseListener(this, true);  // We need this to pass drag events through the labels
        addAndMakeVisible(rotary);
    }
    
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
    sampleRequiredControls.add(&playbackOptions);
    addAndMakeVisible(playbackOptions);

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

JustaSampleAudioProcessorEditor::~JustaSampleAudioProcessorEditor()
{
    attackCurve.setLookAndFeel(nullptr);
    releaseCurve.setLookAndFeel(nullptr);
    gainSlider.setLookAndFeel(nullptr);
}

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
    tuningDetectButton.setEnabled(!sampleLoader.isLoading());
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

    // This is to fix a lag issue found with Reaper and the regular software renderer 
    if (hostType.isReaper() && getPeer() && getPeer()->getCurrentRenderingEngine() == 0 && !openGLContext.isAttached())
        openGLContext.attachTo(*getTopLevelComponent());
}

void JustaSampleAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(Colors::BACKGROUND);

    auto bounds = getLocalBounds();

    auto controls = bounds.toFloat().removeFromTop(scale(Layout::controlsHeight));

    juce::Path toolbarBackground;
    toolbarBackground.addRectangle(controls);

    g.setColour(Colors::FOREGROUND);
    g.fillPath(toolbarBackground);

    juce::Array widths = { Layout::tuningWidth, Layout::attackWidth, Layout::releaseWidth, Layout::playbackWidth, Layout::loopWidth };
    controls.removeFromLeft(scale(Layout::controlsPaddingX));
    for (int width : widths)
    {
        controls.removeFromLeft(scale(width + Layout::moduleGap / 2.f));
        g.setColour(Colors::SLATE.withAlpha(0.2f));
        g.drawVerticalLine(controls.getX(), controls.getY() + controls.getHeight() * 0.125f, controls.getY() + controls.getHeight() * 0.875f);
        controls.removeFromLeft(scale(Layout::moduleGap / 2.f));
    }
}

void JustaSampleAudioProcessorEditor::paintOverChildren(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    auto controls = bounds.toFloat().removeFromTop(scale(Layout::controlsHeight));
}

void JustaSampleAudioProcessorEditor::resized()
{
    pluginState.width = getWidth();
    pluginState.height = getHeight();

    auto bounds = getLocalBounds().toFloat();

    prompt.setBounds(bounds.toNearestInt());

    audioDeviceSettings.setBounds(bounds.reduced(juce::jmin<int>(bounds.getWidth(), bounds.getHeight()) / 4.f).toNearestInt());

    // Layout the controls toolbar
    auto controls = bounds.removeFromTop(scale(Layout::controlsHeight));
    controls.removeFromLeft(scale(Layout::controlsPaddingX));
    controls.removeFromTop(scale(8));
    controls.removeFromBottom(scale(10));

    juce::Array labels = { &tuningLabel, &attackLabel, &releaseLabel, &playbackLabel, &loopingLabel, &masterLabel };
    for (juce::Label* label : labels)
        label->setFont(getInriaSans().withHeight(scale(34.3f)));

    // Tuning module
    auto tuningModule = controls.removeFromLeft(scale(Layout::tuningWidth));

    auto tuningLabelBounds = tuningModule.removeFromTop(scale(Layout::moduleLabelHeight));
    tuningLabelBounds.reduce(0, scale(Layout::moduleLabelPadding));
    tuningLabel.setBounds(tuningLabelBounds.toNearestInt());
    tuningModule.removeFromTop(scale(Layout::moduleLabelGap));

    // Rotaries take up a bit of extra space for the thumb, so we need to compensate by subtracting from other values
    float rotarySize = scale(Layout::standardRotarySize);
    float rotaryPadding = rotarySize * Layout::rotaryPadding;

    tuningModule.reduce(scale(16.f) - rotaryPadding, scale(6.f) - rotaryPadding);

    auto semitoneBounds = tuningModule.removeFromLeft(rotarySize + 2 * rotaryPadding);
    semitoneRotary.setBounds(semitoneBounds.toNearestInt());
    semitoneRotary.sendLookAndFeelChange();
    tuningModule.removeFromLeft(scale(Layout::moduleControlsGap) - 2 * rotaryPadding);

    auto centBounds = tuningModule.removeFromLeft(rotarySize + 2 * rotaryPadding);
    centRotary.setBounds(centBounds.toNearestInt());
    centRotary.sendLookAndFeelChange();
    tuningModule.removeFromLeft(scale(Layout::moduleControlsGap) - rotaryPadding);

    auto detectTuningBounds = tuningModule.removeFromLeft(scale(71.f)).reduced(0.f, rotaryPadding);
    tuningDetectLabel.setBounds(detectTuningBounds.removeFromTop(scale(29.f)).expanded(scale(0.01), 0.f).toNearestInt());
    tuningDetectLabel.setFont(getInriaSansBold().withHeight(scale(27.5f)));
    detectTuningBounds.removeFromTop(scale(5.f));
    tuningDetectButton.setBounds(detectTuningBounds.removeFromTop(scale(46.f)).toNearestInt());

    controls.removeFromLeft(scale(Layout::moduleGap));

    // Attack module
    auto attackModule = controls.removeFromLeft(scale(Layout::attackWidth));

    auto attackLabelBounds = attackModule.removeFromTop(scale(Layout::moduleLabelHeight));
    attackLabelBounds.reduce(0, scale(Layout::moduleLabelPadding));
    attackLabel.setBounds(attackLabelBounds.toNearestInt());

    attackModule.removeFromTop(scale(Layout::moduleLabelGap));
    attackModule.reduce(scale(16.f) - rotaryPadding, scale(6.f) - rotaryPadding);

    auto attackTimeBounds = attackModule.removeFromLeft(rotarySize + 2 * rotaryPadding);
    attackTimeRotary.setBounds(attackTimeBounds.toNearestInt());
    attackTimeRotary.sendLookAndFeelChange();
    attackModule.removeFromLeft(scale(Layout::moduleControlsGap) - 2 * rotaryPadding);

    auto attackCurveBounds = attackModule.removeFromLeft(scale(58.f) + 2 * rotaryPadding).reduced(0.f, scale(16.5f));
    attackCurve.setBounds(attackCurveBounds.toNearestInt());

    controls.removeFromLeft(scale(Layout::moduleGap));

    // Release module
    auto releaseModule = controls.removeFromLeft(scale(Layout::releaseWidth));

    auto releaseLabelBounds = releaseModule.removeFromTop(scale(Layout::moduleLabelHeight));
    releaseLabelBounds.reduce(0, scale(Layout::moduleLabelPadding));
    releaseLabel.setBounds(releaseLabelBounds.toNearestInt());

    releaseModule.removeFromTop(scale(Layout::moduleLabelGap));
    releaseModule.reduce(scale(16.f) - rotaryPadding, scale(6.f) - rotaryPadding);

    auto releaseTimeBounds = releaseModule.removeFromLeft(rotarySize + 2 * rotaryPadding);
    releaseTimeRotary.setBounds(releaseTimeBounds.toNearestInt());
    releaseTimeRotary.sendLookAndFeelChange();
    releaseModule.removeFromLeft(scale(Layout::moduleControlsGap) - 2 * rotaryPadding);

    auto releaseCurveBounds = releaseModule.removeFromLeft(scale(58.f) + 2 * rotaryPadding).reduced(0.f, scale(16.5f));
    releaseCurve.setBounds(releaseCurveBounds.toNearestInt());

    controls.removeFromLeft(scale(Layout::moduleGap));

    // Playback module
    auto playbackModule = controls.removeFromLeft(scale(Layout::playbackWidth));

    auto playbackLabelBounds = playbackModule.removeFromTop(scale(Layout::moduleLabelHeight));
    playbackLabelBounds.reduce(0, scale(Layout::moduleLabelPadding));
    playbackLabel.setBounds(playbackLabelBounds.toNearestInt());

    playbackModule.removeFromTop(scale(Layout::moduleLabelGap));
    playbackModule.reduce(scale(16.f) - rotaryPadding, scale(6.f) - rotaryPadding);
    playbackModule.removeFromLeft(rotaryPadding);

    auto lofiButtonBounds = playbackModule.removeFromLeft(scale(71.f)).reduced(0.f, rotaryPadding + scale(7.5f));
    lofiModeButton.setBounds(lofiButtonBounds.toNearestInt());
    lofiModeButton.setBorder(scale(2.5f), scale(6.f));
    lofiModeButton.setPadding(scale(12.f));

    playbackModule.removeFromLeft(scale(Layout::moduleControlsGap));
    playbackModeButton.setBounds(playbackModule.removeFromLeft(scale(300.f)).reduced(0.f, rotaryPadding).toNearestInt());
    playbackModeButton.setBorder(scale(2.5f), scale(6.f));

    playbackModule.removeFromLeft(scale(Layout::moduleControlsGap) - rotaryPadding);
    playbackSpeedRotary.setBounds(playbackModule.removeFromLeft(rotarySize + 2 * rotaryPadding).toNearestInt());
    playbackSpeedRotary.sendLookAndFeelChange();
    
    controls.removeFromLeft(scale(Layout::moduleGap));

    // Loop module
    auto loopModule = controls.removeFromLeft(scale(Layout::loopWidth));

    auto loopingLabelBounds = loopModule.removeFromTop(scale(Layout::moduleLabelHeight));
    loopingLabelBounds.reduce(0, scale(Layout::moduleLabelPadding));
    loopingLabel.setBounds(loopingLabelBounds.toNearestInt());

    loopModule.removeFromTop(scale(Layout::moduleLabelGap));
    loopModule.reduce(scale(16.f), scale(6.f));

    loopStartButton.setBounds(loopModule.removeFromLeft(scale(34.f)).toNearestInt());
    loopStartButton.setBorder(scale(2.5f), scale(6.f), true, false, true, false);
    loopStartButton.setPadding(scale(8.f), scale(4.f), scale(7.5f), scale(7.5f));

    loopModule.removeFromLeft(scale(6.f));
    loopButton.setBounds(loopModule.removeFromLeft(scale(105.f)).toNearestInt());
    loopButton.setBorder(scale(2.5f));
    loopButton.setPadding(scale(13.f));

    loopModule.removeFromLeft(scale(6.f));
    loopEndButton.setBounds(loopModule.removeFromLeft(scale(34.f)).toNearestInt());
    loopEndButton.setBorder(scale(2.5f), scale(6.f), false, true, false, true);
    loopEndButton.setPadding(scale(4.f), scale(8.f), scale(7.5f), scale(7.5f));

    controls.removeFromLeft(scale(Layout::moduleGap));

    // Master module
    auto masterModule = controls.removeFromLeft(scale(Layout::masterWidth));

    auto masterLabelBounds = masterModule.removeFromTop(scale(Layout::moduleLabelHeight));
    masterLabelBounds.reduce(0, scale(Layout::moduleLabelPadding));
    masterLabel.setBounds(masterLabelBounds.toNearestInt());

    masterModule.removeFromTop(scale(Layout::moduleLabelGap));
    masterModule.reduce(scale(16.f), 0.f);

    auto monoButtonBounds = masterModule.removeFromLeft(scale(92.f)).reduced(0.f, scale(27.5f));
    monoOutputButton.setBounds(monoButtonBounds.toNearestInt());
    monoOutputButton.setBorder(scale(2.5f), scale(6.f));
    monoOutputButton.setPadding(scale(12.f));

    masterModule.removeFromLeft(scale(Layout::moduleControlsGap));
    gainSlider.setBounds(masterModule.removeFromLeft(scale(153.f)).toNearestInt());
    gainSlider.sendLookAndFeelChange();

    // Layout the editor
    auto editor = bounds.removeFromTop(bounds.getHeight() * 0.66f);
    auto navigator = bounds.removeFromTop(bounds.getHeight() * 0.2f);

    sampleLoader.setBounds(editor.toNearestInt());
    sampleEditor.setBounds(editor.toNearestInt());
    sampleNavigator.setBounds(navigator.toNearestInt());
    fxChain.setBounds(bounds.toNearestInt());
}

void JustaSampleAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    auto parent = event.originalComponent->getParentComponent();
    for (auto rotary : rotaries)
        if (parent == rotary)
            rotary->mouseDown(event.getEventRelativeTo(rotary));
}

void JustaSampleAudioProcessorEditor::mouseUp(const juce::MouseEvent& event)
{
    auto parent = event.originalComponent->getParentComponent();
    for (auto rotary : rotaries)
        if (parent == rotary)
            rotary->mouseUp(event.getEventRelativeTo(rotary));
}

void JustaSampleAudioProcessorEditor::mouseDrag(const juce::MouseEvent& event)
{
    auto parent = event.originalComponent->getParentComponent();
    for (auto rotary : rotaries)
        if (parent == rotary)
            rotary->mouseDrag(event.getEventRelativeTo(rotary));
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
        tuningDetectButton.setEnabled(false);
        prompt.openPrompt({}, [this] {
            sampleEditor.cancelBoundsSelection();
            tuningDetectButton.setEnabled(true);
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
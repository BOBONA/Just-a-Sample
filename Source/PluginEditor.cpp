/*
  ==============================================================================

    PluginProcessor.cpp
    Created: 5 Sep 2023
    Author:  binya

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

JustaSampleAudioProcessorEditor::JustaSampleAudioProcessorEditor(JustaSampleAudioProcessor& audioProcessor)
    : AudioProcessorEditor(&audioProcessor), p(audioProcessor), pluginState(p.getPluginState()), synthVoices(p.getSamplerVoices()),
    // Modules
    tuningLabel("", "Tuning"),
    attackLabel("", "Attack"),
    releaseLabel("", "Release"),
    playbackLabel("", "Playback"),
    loopingLabel("", "Loop"),
    masterLabel("", "Master"),

    // Tuning module
    semitoneRotaryAttachment(p.APVTS(), PluginParameters::SEMITONE_TUNING, semitoneRotary),
    centRotaryAttachment(p.APVTS(), PluginParameters::CENT_TUNING, centRotary),
    waveformSemitoneRotaryAttachment(p.APVTS(), PluginParameters::WAVEFORM_SEMITONE_TUNING, waveformSemitoneRotary),
    waveformCentRotaryAttachment(p.APVTS(), PluginParameters::WAVEFORM_CENT_TUNING, waveformCentRotary),

    tuningDetectLabel("", "Detect"),
    tuningDetectButton(Colors::DARK, getOutlineFromSVG(BinaryData::IconDetect_svg)),

    // Attack and release modules
    attackTimeAttachment(p.APVTS(), PluginParameters::ATTACK, attackTimeRotary),
    attackCurveAttachment(p.APVTS(), PluginParameters::ATTACK_SHAPE, attackCurve),
    releaseTimeAttachment(p.APVTS(), PluginParameters::RELEASE, releaseTimeRotary),
    releaseCurveAttachment(p.APVTS(), PluginParameters::RELEASE_SHAPE, releaseCurve),

    // Playback module
    lofiModeButton(Colors::DARKER_SLATE, Colors::WHITE),
    lofiModeAttachment(p.APVTS(), PluginParameters::SKIP_ANTIALIASING, lofiModeButton),
    playbackModeButton(p.APVTS(), PluginParameters::PLAYBACK_MODE, Colors::SLATE, Colors::WHITE, PluginParameters::PLAYBACK_MODE_LABELS[0], PluginParameters::PLAYBACK_MODE_LABELS[1], this),
    playbackModeAttachment(*p.APVTS().getParameter(PluginParameters::PLAYBACK_MODE), [this](float) { enablementChanged(); }, &p.getUndoManager()),
    playbackSpeedAttachment(p.APVTS(), PluginParameters::SPEED_FACTOR, playbackSpeedRotary),

    // Loop module
    loopButton(Colors::DARKER_SLATE, Colors::LOOP, true),
    loopStartButton(Colors::DARKER_SLATE, Colors::LOOP, true),
    loopEndButton(Colors::DARKER_SLATE, Colors::LOOP, true),
    loopAttachment(p.APVTS(), PluginParameters::IS_LOOPING, loopButton),
    loopStartAttachment(p.APVTS(), PluginParameters::LOOPING_HAS_START, loopStartButton),
    loopEndAttachment(p.APVTS(), PluginParameters::LOOPING_HAS_END, loopEndButton),

    // master module
    monoOutputButton(Colors::DARKER_SLATE, Colors::WHITE),
    monoOutputAttachment(p.APVTS(), PluginParameters::MONO_OUTPUT, monoOutputButton),
    gainSliderAttachment(p.APVTS(), PluginParameters::SAMPLE_GAIN, gainSlider),
    
    // Sample toolbar
    filenameComponent("", {}, true, false, false, p.getWildcardFilter(), "", "Select a file to load..."),
    linkSampleToggle(Colors::DARK, Colors::SLATE.withAlpha(0.5f), true, true),
    waveformModeLabel("", "Waveform Mode"),

    playStopButton(Colors::DARK, {}, this),
    recordButton(Colors::HIGHLIGHT),
    deviceSettingsButton(Colors::DARK, getOutlineFromSVG(BinaryData::IconRecordSettings_svg)),
    audioDeviceSettings(p.getDeviceManager(), 0, 2, true),

    fitButton(Colors::DARK, getOutlineFromSVG(BinaryData::IconFit_svg)),
    pinButton(Colors::DARK, Colors::SLATE.withAlpha(0.5f), true, true),
    pinButtonAttachment(pinButton, pluginState.pinView),

    // Main controls
    sampleEditor(p.APVTS(), p.getPluginState(), synthVoices, 
        [this](const juce::MouseWheelDetails& details, int center) -> void { sampleNavigator.scrollView(details, center, true); }),
    sampleNavigator(p.APVTS(), p.getPluginState(), synthVoices),

    fxChain(p),

    // Footer
    logo(Colors::DARK, getOutlineFromSVG(BinaryData::Logo_svg)),
    helpText("", defaultMessage),
    preFXButton(Colors::DARKER_SLATE, Colors::WHITE),
    preFXAttachment(p.APVTS(), PluginParameters::PRE_FX, preFXButton),
    showFXButton(Colors::DARK, Colors::SLATE.withAlpha(0.f), true, true),
    showFXAttachment(showFXButton, pluginState.showFX),
    eqEnablementAttachment(*p.APVTS().getParameter(PluginParameters::EQ_ENABLED), [this](float newValue) { eqEnabled = bool(newValue); fxEnablementChanged(); }, & p.getUndoManager()),
    reverbEnablementAttachment(*p.APVTS().getParameter(PluginParameters::REVERB_ENABLED), [this](float newValue) { reverbEnabled = bool(newValue); fxEnablementChanged(); }, & p.getUndoManager()),
    distortionEnablementAttachment(*p.APVTS().getParameter(PluginParameters::DISTORTION_ENABLED), [this](float newValue) { distortionEnabled = bool(newValue); fxEnablementChanged(); }, &p.getUndoManager()),
    chorusEnablementAttachment(*p.APVTS().getParameter(PluginParameters::CHORUS_ENABLED), [this](float newValue) { chorusEnabled = bool(newValue); fxEnablementChanged(); }, &p.getUndoManager()),

    lnf(dynamic_cast<CustomLookAndFeel&>(getLookAndFeel()))
{
    // Set the plugin sizing
    int width = pluginState.width;
    int height = pluginState.height;

    auto* desktopSize = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
    int maxWidth = desktopSize ? desktopSize->totalArea.getWidth() : 2000;
    int maxHeight = desktopSize ? desktopSize->totalArea.getHeight() : 1600;

    int minWidth = 250;
    int minHeight = 200;

    if (minWidth > width || width > maxWidth || minHeight > height || height > maxHeight)
        setSize(2000, 1600);
    else
        setSize(width, height);
    setResizable(true, false);
    setResizeLimits(minWidth, minHeight, maxWidth, maxHeight);

    // Controls toolbar
    juce::Array moduleLabels{ &tuningLabel, &tuningDetectLabel, &attackLabel, &releaseLabel, &playbackLabel, &loopingLabel, &masterLabel };
    for (juce::Label* label : moduleLabels)
    {
        label->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
    }

    std::function convertWithSecondaryUnit = [](double value) {
        if (value >= 1000)
            return juce::String(value / 1000.f, 1);
        return juce::String(juce::roundToInt(value));
    };

    semitoneRotary.getProperties().set(ComponentProps::ROTARY_UNIT, "sm");
    waveformSemitoneRotary.getProperties().set(ComponentProps::ROTARY_UNIT, "sm");
    centRotary.getProperties().set(ComponentProps::ROTARY_UNIT, "%");
    waveformCentRotary.getProperties().set(ComponentProps::ROTARY_UNIT, "%");

    tuningDetectButton.onClick = [this] { promptPitchDetection(); };
    tuningDetectButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    tuningDetectButton.setHelpText("Auto-tune to 440hz (experimental)");
    addAndMakeVisible(&tuningDetectButton);

    attackTimeRotary.getProperties().set(ComponentProps::ROTARY_UNIT, "ms");
    attackTimeRotary.getProperties().set(ComponentProps::ROTARY_GREATER_UNIT, "sec");
    attackTimeRotary.textFromValueFunction = convertWithSecondaryUnit;
    attackTimeRotary.updateText();

    attackCurve.setLookAndFeel(&attackCurveLNF);
    attackCurve.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    attackCurve.setMouseDragSensitivity(100);
    attackCurve.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(&attackCurve);

    releaseTimeRotary.getProperties().set(ComponentProps::ROTARY_UNIT, "ms");
    releaseTimeRotary.getProperties().set(ComponentProps::ROTARY_GREATER_UNIT, "sec");
    releaseTimeRotary.textFromValueFunction = convertWithSecondaryUnit;
    releaseTimeRotary.updateText();

    releaseCurve.setLookAndFeel(&releaseCurveLNF);
    releaseCurve.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    releaseCurve.setMouseDragSensitivity(100);
    releaseCurve.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(&releaseCurve);

    lofiModeButton.useShape(getOutlineFromSVG(BinaryData::IconLofi_svg));
    lofiModeButton.setHelpText("\"Lo-fi\" resampling");
    addAndMakeVisible(&lofiModeButton);

    playbackModeButton.setHelpText("Clear sound, higher pitches play quicker", "High CPU, maintains speed");
    addAndMakeVisible(&playbackModeButton);

    playbackSpeedRotary.getProperties().set(ComponentProps::ROTARY_ICON, new ReferenceCountedPath(getOutlineFromSVG(BinaryData::IconSpeed_svg)));
    playbackSpeedRotary.getProperties().set(ComponentProps::ROTARY_UNIT, "x");

    loopButton.useShape(getOutlineFromSVG(BinaryData::IconLoop_svg));
    loopButton.setHelpText("Loop sample");
    addAndMakeVisible(&loopButton);

    loopStartButton.useShape(getOutlineFromSVG(BinaryData::IconLoopStart_svg));
    loopStartButton.setOwnerButton(&loopButton);
    loopStartButton.setHelpText("Loop with start portion");
    addAndMakeVisible(&loopStartButton);

    juce::Path loopEndIcon{ getOutlineFromSVG(BinaryData::IconLoopStart_svg) };
    loopEndIcon.applyTransform(juce::AffineTransform::scale(-1, 1));
    loopEndButton.useShape(loopEndIcon);
    loopEndButton.setOwnerButton(&loopButton);
    loopEndButton.setHelpText("Loop with release portion");
    addAndMakeVisible(&loopEndButton);

    monoOutputButton.useShape(getOutlineFromSVG(BinaryData::IconMono_svg));
    monoOutputButton.setHelpText("Mix output to mono");
    addAndMakeVisible(&monoOutputButton);

    gainSlider.setLookAndFeel(&gainSliderLNF);
    gainSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 0, 0);
    gainSlider.setTextValueSuffix(" db");
    gainSlider.setHelpText("Sample gain");
    addAndMakeVisible(&gainSlider);

    rotaries = { &semitoneRotary, &waveformSemitoneRotary, &centRotary, &waveformCentRotary, &attackTimeRotary, &releaseTimeRotary, &playbackSpeedRotary };
    for (auto rotary : rotaries)
    {
        rotary->setSliderStyle(juce::Slider::RotaryVerticalDrag);
        rotary->setMouseDragSensitivity(150);
        rotary->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 0, 0);
        rotary->addMouseListener(this, true);  // We need this to pass drag events through the labels
        addAndMakeVisible(rotary);
    }

    // Editor and navigator
    addAndMakeVisible(sampleEditor);

    sampleNavigator.setHelpText("Drag or scroll to navigate");
    addAndMakeVisible(sampleNavigator);

    statusLabel.setColour(juce::Label::textColourId, Colors::DARK);
    statusLabel.setColour(juce::Label::backgroundColourId, Colors::BACKGROUND.withAlpha(0.85f));
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(statusLabel);
    
    // Sample controls
    editorOverlay.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(editorOverlay);

    juce::StringArray recentFiles{ pluginState.recentFiles };
    for (int i = recentFiles.size() - 1; i >= 0; i--)
        filenameComponent.addRecentlyUsedFile(juce::File(recentFiles[i]));
    filenameComponent.addListener(this);
    filenameComponent.setHelpText("Choose a file");
    addAndMakeVisible(filenameComponent);

    waveformModeLabel.setColour(juce::Label::textColourId, Colors::FOREGROUND);
    waveformModeLabel.setJustificationType(juce::Justification::centred);
    waveformModeLabel.setHelpText("Uses sample bounds as a waveform");
    addChildComponent(waveformModeLabel);

    linkSampleToggle.useShape(getOutlineFromSVG(BinaryData::IconLinkEnabled_svg));
    linkSampleToggle.onClick = [this] { toggleLinkSample(); };
    linkSampleToggle.setHelpText("Link preset to file");
    addAndMakeVisible(linkSampleToggle);

    playPath = getOutlineFromSVG(BinaryData::IconPlay_svg);
    stopPath.addRoundedRectangle(juce::Rectangle(0, 0, 10, 10), 2.f);
    playStopButton.setShape(playPath);
    playStopButton.onClick = [this]
        {
            if (!currentlyPlaying)
                p.playVoice();
            else
                p.haltVoices();
        };
    playStopButton.setHelpText(playHelpText);
    addAndMakeVisible(playStopButton);

    juce::Path recordIcon;
    recordIcon.addEllipse(0, 0, 10, 10);
    recordButton.setShape(recordIcon);
    recordButton.onClick = [this] { startRecording(true); };
    recordButton.setHelpText("Record a sample");
    addAndMakeVisible(recordButton);

    deviceSettingsButton.onClick = [this] { promptDeviceSettings(); };
    deviceSettingsButton.setHelpText("Open input device settings");
    addAndMakeVisible(deviceSettingsButton);

    fitButton.onClick = [this] { sampleNavigator.fitView(); };
    fitButton.setHelpText("Fit viewport to bounds");
    addAndMakeVisible(fitButton);

    pinButton.useShape(getOutlineFromSVG(BinaryData::IconPin_svg));
    pinButton.setHelpText("Pin bounds to viewport");
    addAndMakeVisible(pinButton);

    // FX
    addChildComponent(fxChain);

    // Footer
    logo.onClick = [] { bool _ = juce::URL("https://github.com/BOBONA/Just-a-Sample").launchInDefaultBrowser(); };
    logo.setHelpText("Click for more details");
    addAndMakeVisible(logo);

    helpText.setJustificationType(juce::Justification::centred);
    helpText.setHelpText("This is the help text ;)");
    addAndMakeVisible(helpText);

    preFXButton.useShape(getOutlineFromSVG(BinaryData::IconPreFX_svg));
    preFXButton.setHelpText("Apply FX before Attack/Release");
    addAndMakeVisible(&preFXButton);

    showFXButton.onStateChange = [this, minHeight, maxHeight]
    {
        if (fxChain.isVisible() != pluginState.showFX)
            setSize(getWidth(), juce::jlimit(minHeight, maxHeight, getHeight() + (pluginState.showFX ? scale(Layout::fxChainHeight) : -scale(Layout::fxChainHeight))));

        showFXButton.setHelpText(pluginState.showFX ? "Hide the effects chain" : "Show the effects chain");
    };

    showFXButton.useShape(getOutlineFromSVG(BinaryData::IconHideFX_svg), getOutlineFromSVG(BinaryData::IconShowFX_svg), juce::Justification::centredRight);
    showFXButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible(showFXButton);

    eqEnablementAttachment.sendInitialUpdate();
    reverbEnablementAttachment.sendInitialUpdate();
    distortionEnablementAttachment.sendInitialUpdate();
    chorusEnablementAttachment.sendInitialUpdate();

    addChildComponent(audioDeviceSettings);

    prompt.setHelpText("Click to close");
    addAndMakeVisible(prompt);

    juce::Array<Component*> foregroundComponents = {
        &tuningLabel, &attackLabel, &releaseLabel, &playbackLabel, &loopingLabel, &masterLabel, &semitoneRotary, &waveformSemitoneRotary, &centRotary, &waveformCentRotary, &tuningDetectLabel, &tuningDetectButton,
        &attackTimeRotary, &attackCurve, &releaseTimeRotary, &releaseCurve, &lofiModeButton, &playbackModeButton, &playbackSpeedRotary, &loopStartButton, &loopButton, &loopEndButton,
        &monoOutputButton, &gainSlider, &filenameComponent, &linkSampleToggle, &playStopButton, &recordButton, &deviceSettingsButton, &fitButton, &pinButton, &sampleNavigator, &preFXButton, &showFXButton
    };

    for (Component* component : foregroundComponents)
        component->setColour(Colors::backgroundColorId, Colors::FOREGROUND);

    setWantsKeyboardFocus(true);
    addMouseListener(this, true);
    startTimerHz(PluginParameters::FRAME_RATE);

    enablementChanged();
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
    // If the processor has a new sample loaded, update the editor
    if (pluginState.sampleHash != expectedHash && p.getSampleBuffer().getNumSamples())
    {
        loadSample();

        // The file path should be updated by this point, in which case the filenameComponent should be updated
        if (filenameComponent.getCurrentFileText() != pluginState.filePath)
        {
            filenameComponent.setCurrentFile(juce::File(pluginState.filePath), true, juce::dontSendNotification);
            pluginState.recentFiles = filenameComponent.getRecentlyUsedFilenames();
        }

        resized();
    }

    // If playback state has changed, update the sampleEditor and sampleNavigator
    bool wasPlaying = currentlyPlaying;
    currentlyPlaying = std::ranges::any_of(synthVoices, [](auto* voice) -> bool{ return bool(voice->getCurrentlyPlayingSound()); });

    if ((wasPlaying && !currentlyPlaying) || currentlyPlaying)
    {
        sampleEditor.repaint();
        sampleNavigator.repaint();
    }

    if (wasPlaying != currentlyPlaying)
    {
        playStopButton.setShape(currentlyPlaying ? stopPath : playPath);
        playStopButton.setCustomHelpText(currentlyPlaying ? stopHelpText : playHelpText);
    }

    editorOverlay.setWaveformMode(CustomSamplerVoice::isWavetableMode(p.getBufferSampleRate(), pluginState.sampleStart, pluginState.sampleEnd) && !p.getRecorder().isRecordingDevice());

    // Handle recording changes (this must take place in a timer callback, since the recorder does not update synchronously)
    if (p.getRecorder().isRecordingDevice())
    {
        if (!sampleEditor.isRecordingMode())
        {
            sampleEditor.setRecordingMode(true);
            sampleNavigator.setRecordingMode(true);
            prompt.openPrompt({ &sampleEditor }, [this] {
                p.getRecorder().stopRecording();
                sampleEditor.setRecordingMode(false);
                sampleNavigator.setRecordingMode(false);
                enablementChanged();
            });
        }
        handleActiveRecording();
    }
    else if (sampleEditor.isRecordingMode())
    {
        prompt.closePrompt();
    }

    enablementChanged();

    // This is to fix a lag issue found with Reaper and the regular software renderer 
    if (hostType.isReaper() && getPeer() && getPeer()->getCurrentRenderingEngine() == 0 && !openGLContext.isAttached())
        openGLContext.attachTo(*getTopLevelComponent());
}

void JustaSampleAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(Colors::WHITE);

    auto bounds = getConstrainedBounds().toFloat();

    g.setColour(Colors::BACKGROUND);
    g.fillRect(bounds);

    auto controls = bounds.removeFromTop(scale(Layout::controlsHeight));

    juce::Path toolbarBackground;
    toolbarBackground.addRectangle(controls.toNearestInt());

    g.setColour(Colors::FOREGROUND);
    g.fillPath(toolbarBackground);

    juce::Array widths = { Layout::tuningWidth, Layout::attackWidth, Layout::releaseWidth, Layout::playbackWidth, Layout::loopWidth };
    controls.removeFromLeft(scale(Layout::controlsPaddingX));
    for (int width : widths)
    {
        controls.removeFromLeft(scale(width + Layout::moduleGap / 2.f));
        g.setColour(Colors::SLATE.withAlpha(0.2f));
        g.drawVerticalLine(int(controls.getX()), controls.getY() + controls.getHeight() * 0.125f, controls.getY() + controls.getHeight() * 0.875f);
        controls.removeFromLeft(scale(Layout::moduleGap / 2.f));
    }

    auto footerBounds = bounds.removeFromBottom(scale(Layout::footerHeight));
    g.setColour(Colors::FOREGROUND);
    g.fillRect(footerBounds.toNearestInt());

    g.setColour(Colors::SLATE.withAlpha(0.2f));
    g.drawVerticalLine(int(std::round(showFXButton.getX() - scale(1.f))), footerBounds.getY() + scale(13.f), footerBounds.getBottom() - scale(13.f));

    bool fxEnabled = eqEnabled || reverbEnabled || distortionEnabled || chorusEnabled;
    if (fxEnabled)
    {
        auto fxBounds = showFXButton.getBounds().toFloat();
        auto indictatorBounds = fxBounds.removeFromTop(scale(17.f)).removeFromRight(scale(17.f)).translated(scale(12.f), scale(6.f));
        g.setColour(Colors::HIGHLIGHT);
        g.fillEllipse(indictatorBounds);
    }

    auto sampleLoaded = bool(p.getSampleBuffer().getNumSamples());
    if (pluginState.showFX)
    {
        bounds.removeFromBottom(scale(Layout::fxChainHeight));
    }
    else if (sampleLoaded)
    {
        float border = scale(2.5f);

        g.setColour(Colors::SLATE.withAlpha(0.5f));
        g.fillRect(bounds.getX(), bounds.getBottom() - border / 2.f, bounds.getWidth(), border);
    }

    if (sampleLoaded || pluginState.showFX)
    {
        auto navigatorBounds = bounds.removeFromBottom(scale(Layout::sampleNavigatorHeight));
        g.setColour(Colors::FOREGROUND);
        g.fillRect(navigatorBounds.toNearestInt());
    }
}

void EditorOverlay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    bounds.removeFromTop(scale(Layout::sampleControlsMargin.getY()));

    auto sampleControls = bounds.removeFromTop(scale(Layout::sampleControlsHeight)).reduced(scale(Layout::sampleControlsMargin.getX()), 0.f);
    auto fileControls = sampleControls.removeFromLeft(scale(Layout::fileControlsWidth));
    auto playbackControls = sampleControls.removeFromRight(scale(Layout::playbackControlsWidth));

    juce::Path sampleControlRegionsPath;
    sampleControlRegionsPath.addRoundedRectangle(fileControls, scale(11.f));
    sampleControlRegionsPath.addRoundedRectangle(playbackControls, scale(11.f));

    sampleControlShadow.render(g, sampleControlRegionsPath);
    g.setColour(Colors::FOREGROUND);
    g.fillPath(sampleControlRegionsPath);

    if (waveformMode)
    {
        sampleControls.removeFromLeft(scale(Layout::sampleControlsMargin.x));
        auto waveformModeBounds = sampleControls.removeFromLeft(scale(Layout::waveformModeWidth));

        sampleControlRegionsPath.clear();
        sampleControlRegionsPath.addRoundedRectangle(waveformModeBounds, scale(11.f));

        sampleControlShadow.render(g, sampleControlRegionsPath);
        g.setColour(Colors::HIGHLIGHT);
        g.fillPath(sampleControlRegionsPath);
    }

    auto navControls = bounds.removeFromBottom(scale(Layout::navigatorControlsSize.y)).removeFromRight(scale(Layout::navigatorControlsSize.x)).getSmallestIntegerContainer();
    
    juce::Path navControlRegionsPath;
    navControlRegionsPath.addRoundedRectangle(navControls.getX(), navControls.getY(), 
        navControls.getWidth(), navControls.getHeight(), scale(20.f), scale(20.f), 
        true, false, false, false);

    navControlShadow.render(g, navControlRegionsPath);
    g.setColour(Colors::FOREGROUND);
    g.fillPath(navControlRegionsPath);
}

void EditorOverlay::resized()
{
    sampleControlShadow.setOffset({ int(scale(2.f)), int(scale(2.f)) });
    navControlShadow.setOffset({ int(-scale(2.f)), int(-scale(2.f)) });
}

void JustaSampleAudioProcessorEditor::resized()
{
    pluginState.width = getWidth();
    pluginState.height = getHeight();

    auto bounds = getConstrainedBounds().toFloat();

    prompt.setBounds(bounds.toNearestInt());

    audioDeviceSettings.setBounds(bounds.reduced(juce::jmin(bounds.getWidth(), bounds.getHeight()) / 4.f).toNearestInt());

    // Layout the controls toolbar
    auto controls = bounds.removeFromTop(scale(Layout::controlsHeight));
    controls.removeFromLeft(scale(Layout::controlsPaddingX));
    controls.removeFromTop(scale(8));
    controls.removeFromBottom(scale(10));
    
    juce::Array labels = { &tuningLabel, &attackLabel, &releaseLabel, &playbackLabel, &loopingLabel, &masterLabel };
    for (juce::Label* label : labels)
        label->setFont(getInriaSans().withHeight(scalef(34.3f)));

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
    waveformSemitoneRotary.setBounds(semitoneBounds.toNearestInt());
    waveformSemitoneRotary.sendLookAndFeelChange();
    tuningModule.removeFromLeft(scale(Layout::moduleControlsGap) - 2 * rotaryPadding);

    auto centBounds = tuningModule.removeFromLeft(rotarySize + 2 * rotaryPadding);
    centRotary.setBounds(centBounds.toNearestInt());
    centRotary.sendLookAndFeelChange();
    waveformCentRotary.setBounds(centBounds.toNearestInt());
    waveformCentRotary.sendLookAndFeelChange();
    tuningModule.removeFromLeft(scale(Layout::moduleControlsGap) - rotaryPadding);

    auto detectTuningBounds = tuningModule.removeFromLeft(scale(71.f)).reduced(0.f, rotaryPadding);
    tuningDetectLabel.setBounds(detectTuningBounds.removeFromTop(scale(29.f)).expanded(scale(0.01f), 0.f).toNearestInt());
    tuningDetectLabel.setFont(getInriaSansBold().withHeight(scalef(27.5f)));
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
    lofiModeButton.setBorder(scalef(2.5f), scalef(6.f));
    lofiModeButton.setPadding(scalef(12.f));

    playbackModule.removeFromLeft(scale(Layout::moduleControlsGap));
    playbackModeButton.setBounds(playbackModule.removeFromLeft(scalef(300.f)).reduced(0.f, rotaryPadding).toNearestInt());
    playbackModeButton.setBorder(scalef(2.5f), scalef(6.f));

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
    loopStartButton.setBorder(scalef(2.5f), scalef(6.f), true, false, true, false);
    loopStartButton.setPadding(scalef(8.f), scalef(4.f), scalef(7.5f), scalef(7.5f));

    loopModule.removeFromLeft(scale(6.f));
    loopButton.setBounds(loopModule.removeFromLeft(scale(105.f)).toNearestInt());
    loopButton.setBorder(scalef(2.5f));
    loopButton.setPadding(scalef(13.f));

    loopModule.removeFromLeft(scale(6.f));
    loopEndButton.setBounds(loopModule.removeFromLeft(scale(34.f)).toNearestInt());
    loopEndButton.setBorder(scalef(2.5f), scalef(6.f), false, true, false, true);
    loopEndButton.setPadding(scalef(4.f), scalef(8.f), scalef(7.5f), scalef(7.5f));

    controls.removeFromLeft(scale(Layout::moduleGap));

    // master module
    auto masterModule = controls.removeFromLeft(scale(Layout::masterWidth));

    auto masterLabelBounds = masterModule.removeFromTop(scale(Layout::moduleLabelHeight));
    masterLabelBounds.reduce(0, scale(Layout::moduleLabelPadding));
    masterLabel.setBounds(masterLabelBounds.toNearestInt());

    masterModule.removeFromTop(scale(Layout::moduleLabelGap));
    masterModule.reduce(scale(16.f), 0.f);

    auto monoButtonBounds = masterModule.removeFromLeft(scale(92.f)).reduced(0.f, scale(27.5f));
    monoOutputButton.setBounds(monoButtonBounds.toNearestInt());
    monoOutputButton.setBorder(scalef(2.5f), scalef(6.f));
    monoOutputButton.setPadding(scalef(12.f));

    masterModule.removeFromLeft(scale(Layout::moduleControlsGap));
    gainSlider.setBounds(masterModule.removeFromLeft(scale(153.f)).toNearestInt());
    gainSlider.sendLookAndFeelChange();

    // Footer
    auto footer = bounds.removeFromBottom(scale(Layout::footerHeight));
    footer.removeFromBottom(scale(2.f));
    helpText.setFont(getInter().withHeight(scalef(41.4f)));
    helpText.setBounds(footer.reduced(footer.getWidth() * 0.2f, 0.f).toNearestInt());

    footer.reduce(scale(25.f), 0.f);

    auto logoBounds = footer.removeFromLeft(scale(260.f));
    logo.setBounds(logoBounds.toNearestInt());

    auto showFXButtonBounds = footer.removeFromRight(scale(205.f));
    showFXButton.setPadding(0.f, 0.f, scale(19.f), scale(19.f));
    showFXButton.setBounds(showFXButtonBounds.toNearestInt());

    footer.removeFromRight(scale(30.f));

    auto preFXButtonBounds = footer.removeFromRight(scale(100.f)).reduced(0.f, scale(11.f));
    preFXButton.setBounds(preFXButtonBounds.toNearestInt());
    preFXButton.setBorder(scalef(2.5f), scalef(6.f));
    preFXButton.setPadding(scalef(11.f));

    // FX
    if (pluginState.showFX)
    {
        auto fxChainBounds = bounds.removeFromBottom(scale(Layout::fxChainHeight));
        fxChain.setBounds(fxChainBounds.toNearestInt());
    }
    fxChain.setVisible(pluginState.showFX);

    // Navigator
    auto sampleLoaded = bool(p.getSampleBuffer().getNumSamples());
    if (sampleLoaded || pluginState.showFX)
    {
        auto sampleNavigatorBounds = bounds.removeFromBottom(scale(Layout::sampleNavigatorHeight));
        sampleNavigator.setBounds(sampleNavigatorBounds.toNearestInt());
    }

    // Editor
    auto editorBounds = bounds;
    sampleEditor.setBounds(editorBounds.toNearestInt());
    editorOverlay.setBounds(editorBounds.toNearestInt());

    statusLabel.setFont(getInter().withHeight(scalef(50.f)));
    updateLabel();

    // Sample controls
    editorBounds.removeFromTop(scale(Layout::sampleControlsMargin.getY()));

    auto sampleControls = editorBounds.removeFromTop(scale(Layout::sampleControlsHeight)).reduced(scale(Layout::sampleControlsMargin.getX()), 0.f);
    auto fileControls = sampleControls.removeFromLeft(scale(Layout::fileControlsWidth));
    auto playbackControls = sampleControls.removeFromRight(scale(Layout::playbackControlsWidth));

    fileControls.reduce(scale(12.f), scale(2.1f));
    auto filenameComponentBounds = fileControls.removeFromLeft(scale(1095.f));
    filenameComponent.setBounds(filenameComponentBounds.toNearestInt());

    fileControls.removeFromLeft(scale(20.f));
    auto linkSampleToggleBounds = fileControls.removeFromLeft(scale(48.f)).reduced(0.f, scale(2.85f));
    linkSampleToggle.setBounds(linkSampleToggleBounds.toNearestInt());
    linkSampleToggle.setPadding(scalef(3.f));
    linkSampleToggle.setBorder(0.f, scalef(5.f));

    sampleControls.removeFromLeft(scale(Layout::sampleControlsMargin.x));
    auto waveformModeBounds = sampleControls.removeFromLeft(scale(Layout::waveformModeWidth)).reduced(scale(12.f), scale(2.13f));

    waveformModeLabel.setFont(getInter().withHeight(scale(31.8f)));
    waveformModeLabel.setBounds(waveformModeBounds.toNearestInt());

    playbackControls.reduce(scale(12.f), scale(2.1f));
    auto playStopButtonBounds = playbackControls.removeFromLeft(scale(40.32f)).reduced(scale(4.98f), scale(3.36f));
    playStopButton.setBounds(playStopButtonBounds.toNearestInt());

    playbackControls.removeFromLeft(scale(14.f));
    auto recordControlBounds = playbackControls.removeFromLeft(scale(58.6f)).reduced(scale(2.f), scale(3.36f));
    auto recordButtonBounds = recordControlBounds.removeFromLeft(scale(33.6f));
    recordButton.setBounds(recordButtonBounds.toNearestInt());

    recordControlBounds.removeFromLeft(scale(4.f));
    auto deviceSettingsButtonBounds = recordControlBounds.removeFromTop(scale(23.f));
    deviceSettingsButton.setBounds(deviceSettingsButtonBounds.toNearestInt());

    auto navControlBounds = bounds.removeFromBottom(scale(Layout::navigatorControlsSize.y)).removeFromRight(scale(Layout::navigatorControlsSize.x));
    navControlBounds.removeFromLeft(scale(20.f));

    auto fitButtonBounds = navControlBounds.removeFromLeft(scale(34.f));
    fitButtonBounds.removeFromTop(scale(10.f));
    fitButtonBounds.removeFromBottom(scale(5.f));
    fitButton.setBounds(fitButtonBounds.toNearestInt());

    navControlBounds.removeFromLeft(scale(12.f));
    auto pinButtonBounds = navControlBounds.removeFromLeft(scale(44.f));
    pinButtonBounds.removeFromTop(scale(7.5f));
    pinButtonBounds.removeFromBottom(scale(2.5f));
    pinButton.setBounds(pinButtonBounds.toNearestInt());
    pinButton.setBorder(0.f, scalef(5.f));
    pinButton.setPadding(scalef(5.f));
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

    // We also update the help text when dragging
    mouseMove(event);
}

void JustaSampleAudioProcessorEditor::mouseMove(const juce::MouseEvent& event)
{
    auto* component = event.eventComponent;
    juce::String text{ "" };
    do
    {
        auto* customProvider = dynamic_cast<CustomHelpTextProvider*>(component);
        text = customProvider ? customProvider->getCustomHelpText() : component->getHelpText();
        component = component->getParentComponent();
    } while (text.isEmpty() && component);

    helpText.setText(text, juce::dontSendNotification);
}

void JustaSampleAudioProcessorEditor::mouseExit(const juce::MouseEvent& event)
{
    if (!getLocalBounds().contains(getMouseXYRelative()))
        helpText.setText("", juce::dontSendNotification);
}

void JustaSampleAudioProcessorEditor::helpTextChanged(const juce::String& newText)
{
    if (!isVisible())
        return;

    bool isMessageThread = juce::MessageManager::getInstance()->isThisTheMessageThread();
    if (isMessageThread)
        helpText.setText(newText, juce::dontSendNotification);
    else
        juce::MessageManager::callAsync([this, newText] { helpText.setText(newText, juce::dontSendNotification); });
}

bool JustaSampleAudioProcessorEditor::keyPressed(const juce::KeyPress& key)
{
    auto sampleLoaded = bool(p.getSampleBuffer().getNumSamples());

    if (sampleLoaded && key.getTextCharacter() == 'f')
    {
        sampleNavigator.fitView();
        return true;
    }

    if (sampleLoaded && key.getTextCharacter() == 'p')
    {
        pluginState.pinView = !pluginState.pinView;
        return true;
    }

    return false;
}

juce::Rectangle<int> JustaSampleAudioProcessorEditor::getConstrainedBounds() const
{
    auto fxSpace = !pluginState.showFX ? scalef(Layout::fxChainHeight) : 0;

    auto bounds = getLocalBounds();
    auto width = getWidth();
    auto height = getHeight();
    auto constrainedWidth = int(juce::jlimit<float>((height + fxSpace) * 0.8f, (height + fxSpace) * 2.f, width));

    if (constrainedWidth <= width)
        bounds.reduce((width - constrainedWidth) / 2, 0);
    else
        bounds.reduce(0, int((height - width / 0.8f) / 2.f));

    return bounds;
}

//==============================================================================
void JustaSampleAudioProcessorEditor::loadSample()
{
    bool sampleLoaded = p.getSampleBuffer().getNumSamples();
    if (sampleLoaded)
    {
        expectedHash = pluginState.sampleHash;
        linkSampleToggle.setToggleState(pluginState.usingFileReference, juce::dontSendNotification);
        sampleEditor.setSample(p.getSampleBuffer(), p.getBufferSampleRate(), p.isInitialSampleLoad());
        sampleNavigator.setSample(p.getSampleBuffer(), p.getBufferSampleRate(), p.isInitialSampleLoad());
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
    while (bufferChange && iterations < 5)  // 5 is a safety measure
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
        sampleEditor.setSample(pendingRecordingBuffer, 0.f, false);
        sampleNavigator.setSample(pendingRecordingBuffer, 0.f, false);
    } 
    else if (previousSampleSize != recordingBufferSize)  // Update the rendered paths when more data is added to the buffer (not changing the size)
    {
        sampleEditor.sampleUpdated(previousSampleSize, recordingBufferSize);
        sampleNavigator.sampleUpdated(previousSampleSize, recordingBufferSize);
    }
}

void JustaSampleAudioProcessorEditor::toggleLinkSample()
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
                    linkSampleToggle.setToggleState(true, juce::dontSendNotification);
                    filenameComponent.setCurrentFile(file, true, juce::dontSendNotification);
                    pluginState.usingFileReference = linkSampleToggle.getToggleState();
                }
            }, true);
        linkSampleToggle.setToggleState(false, juce::dontSendNotification);
    }
    else
    {
        pluginState.usingFileReference = linkSampleToggle.getToggleState();
    }
}

void JustaSampleAudioProcessorEditor::startRecording(bool promptSettings)
{
    if (p.getDeviceManager().getCurrentAudioDevice() && p.getDeviceManager().getCurrentAudioDevice()->getActiveInputChannels().countNumberOfSetBits())
        p.getRecorder().startRecording();
    else if (promptSettings)
        promptDeviceSettings(true);
}

void JustaSampleAudioProcessorEditor::promptPitchDetection()
{
    prompt.openPrompt({ &sampleEditor }, [this] {
        sampleEditor.cancelBoundsSelection();
        enablementChanged();
    });
    sampleEditor.promptBoundsSelection([this](int startPos, int endPos) -> void {
        p.startPitchDetectionRoutine(startPos, endPos);
        enablementChanged();
        prompt.closePrompt();
    });
}

void JustaSampleAudioProcessorEditor::promptDeviceSettings(bool recordOnClose)
{
    audioDeviceSettings.setVisible(true);
    prompt.openPrompt({ &audioDeviceSettings }, [this, recordOnClose]() -> void
    {
        audioDeviceSettings.setVisible(false);
        if (recordOnClose)
            startRecording(false);
    });
}

void JustaSampleAudioProcessorEditor::enablementChanged()
{
    auto isSampleLoaded = bool(p.getSampleBuffer().getNumSamples());
    auto isRecording = p.getRecorder().isRecordingDevice() || sampleEditor.isInBoundsSelection();  // For UI purposes, treat bounds selection like recording
    auto isLoading = p.getSampleLoader().isLoading();
    auto isWaveformMode = CustomSamplerVoice::isWavetableMode(p.getBufferSampleRate(), pluginState.sampleStart, pluginState.sampleEnd) && !isRecording;

    // Note, it is very important that a sample's enablement is set to its correct value in one step, since internally JUCE repaints when the enablement changes
    juce::Array<Component*> sampleDependentComponents = {
        &semitoneRotary, &centRotary, &waveformSemitoneRotary, &waveformCentRotary, &tuningDetectLabel,
        &attackTimeRotary, &attackCurve, &releaseTimeRotary, &releaseCurve, &gainSlider,
        &playStopButton, &fitButton, &pinButton, &sampleEditor, &sampleNavigator, &fxChain, &showFXButton
    };

    juce::Array<Component*> notWaveformModeComponents = { &playbackModeButton, &loopStartButton, &loopButton, &loopEndButton, &tuningDetectLabel, &tuningDetectButton };

    juce::Array<Component*> notLoadingDependentComponents = { &tuningDetectButton, &tuningDetectLabel, &recordButton, &deviceSettingsButton };

    juce::Array<Component*> notRecordingDependentComponents = { &filenameComponent, &playStopButton, &recordButton, &deviceSettingsButton, &fitButton, &pinButton, &sampleNavigator };

    // Set enablement on both
    juce::SortedSet<Component*> all;
    all.addArray(sampleDependentComponents.getRawDataPointer(), sampleDependentComponents.size());
    all.addArray(notWaveformModeComponents.getRawDataPointer(), notWaveformModeComponents.size());
    all.addArray(notLoadingDependentComponents.getRawDataPointer(), notLoadingDependentComponents.size());
    all.addArray(notRecordingDependentComponents.getRawDataPointer(), notRecordingDependentComponents.size());

    for (auto component : all)
    {
        auto sampleDependent = sampleDependentComponents.contains(component);
        auto notRecordingDependent = notRecordingDependentComponents.contains(component);
        auto notLoadingDependent = notLoadingDependentComponents.contains(component);
        auto notWaveformModeDependent = notWaveformModeComponents.contains(component);

        component->setEnabled((isSampleLoaded || !sampleDependent) && (!isRecording || !notRecordingDependent) && 
            ((isSampleLoaded && !isWaveformMode) || !notWaveformModeDependent) && (!isLoading || !notLoadingDependent));
    }

    // Some controls need more specialized rules...
    semitoneRotary.setVisible(!isWaveformMode);
    centRotary.setVisible(!isWaveformMode);
    waveformSemitoneRotary.setVisible(isWaveformMode);
    waveformCentRotary.setVisible(isWaveformMode);

    lofiModeButton.setEnabled(isSampleLoaded && (!playbackModeButton.getChoice() || isWaveformMode));

    playbackSpeedRotary.setEnabled(isSampleLoaded && playbackModeButton.getChoice() && !isWaveformMode);
    monoOutputButton.setEnabled(isSampleLoaded && p.getSampleBuffer().getNumChannels() > 1);

    linkSampleToggle.setEnabled(isSampleLoaded && !isRecording && !p.sampleBufferNeedsReference());

    waveformModeLabel.setVisible(isSampleLoaded && isWaveformMode);

    sampleNavigator.setVisible(isSampleLoaded || pluginState.showFX);

    preFXButton.setEnabled(isSampleLoaded && (reverbEnabled || distortionEnabled || chorusEnabled || eqEnabled)),

    // Handle status label
    statusLabel.setVisible(!p.getSampleBuffer().getNumSamples() || p.getSampleLoader().isLoading() || sampleEditor.isInBoundsSelection() || p.getRecorder().isRecordingDevice() || fileDragging);
    statusLabel.setColour(juce::Label::outlineColourId, Colors::DARK.withAlpha(float(fileDragging)));
    if (p.getSampleLoader().isLoading())
        updateLabel("Loading...");
    else if (sampleEditor.isInBoundsSelection())
        updateLabel("Select a region to analyze");
    else if (p.getRecorder().isRecordingDevice())
        updateLabel("Recording...");
    else if (!p.getSampleBuffer().getNumSamples())
        updateLabel("Drop a sample!");
    else if (fileDragging)
        updateLabel("Replace sample");
}

void JustaSampleAudioProcessorEditor::fxEnablementChanged()
{
    enablementChanged();
    repaint(showFXButton.getBounds().expanded(scale(15.f), 0.f));
}

//==============================================================================
bool JustaSampleAudioProcessorEditor::isInterestedInFile(const juce::String& file) const
{
    return p.canLoadFileExtension(file) && !prompt.isPromptVisible();
}

bool JustaSampleAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    return std::ranges::any_of(files, [this](const auto& f) { return isInterestedInFile(f); }) && !p.getRecorder().isRecordingDevice();
}

void JustaSampleAudioProcessorEditor::filesDropped(const juce::StringArray& files, int, int)
{
    for (const juce::String& file : files)
    {
        if (isInterestedInFileDrag(file))
        {
            fileDragging = false;
            enablementChanged();
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

void JustaSampleAudioProcessorEditor::fileDragEnter(const juce::StringArray& files, int x, int y)
{
    if (isInterestedInFileDrag(files))
    {
        fileDragging = true;
        enablementChanged();
    }
}

void JustaSampleAudioProcessorEditor::fileDragExit(const juce::StringArray& files)
{
    fileDragging = false;
    enablementChanged();
}

void JustaSampleAudioProcessorEditor::updateLabel(const juce::String& text)
{
    if (text.isNotEmpty())
        statusLabel.setText(text, juce::dontSendNotification);

    statusLabel.setBounds(juce::Rectangle(0, 0, int(statusLabel.getFont().getStringWidthFloat(statusLabel.getText())), int(statusLabel.getFont().getHeight()))
        .withCentre(sampleEditor.getBounds().getCentre()).expanded(scale(40.f)).toNearestInt());
}

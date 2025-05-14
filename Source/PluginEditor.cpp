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
    : AudioProcessorEditor(&audioProcessor), p(audioProcessor), pluginState(p.getPluginState()), dummyParam(p.APVTS(), PluginParameters::State::UI_DUMMY_PARAM),
    synthVoices(p.getSamplerVoices()),
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
    tuningDetectButton(getOutlineFromSVG(BinaryData::IconDetect_svg)),

    // Attack and release modules
    attackTimeAttachment(p.APVTS(), PluginParameters::ATTACK, attackTimeRotary),
    attackCurveAttachment(p.APVTS(), PluginParameters::ATTACK_SHAPE, attackCurve),
    releaseTimeAttachment(p.APVTS(), PluginParameters::RELEASE, releaseTimeRotary),
    releaseCurveAttachment(p.APVTS(), PluginParameters::RELEASE_SHAPE, releaseCurve),

    // Playback module
    lofiModeAttachment(p.APVTS(), PluginParameters::SKIP_ANTIALIASING, lofiModeButton),
    playbackModeButton(p.APVTS(), PluginParameters::PLAYBACK_MODE, PluginParameters::PLAYBACK_MODE_LABELS[0], PluginParameters::PLAYBACK_MODE_LABELS[1], this),
    playbackModeAttachment(*p.APVTS().getParameter(PluginParameters::PLAYBACK_MODE), [this](float) { enablementChanged(); }, &p.getUndoManager()),
    playbackSpeedAttachment(p.APVTS(), PluginParameters::SPEED_FACTOR, playbackSpeedRotary),

    // Loop module
    loopButton(true),
    loopStartButton(true),
    loopEndButton(true),
    loopAttachment(p.APVTS(), PluginParameters::IS_LOOPING, loopButton),
    loopStartAttachment(p.APVTS(), PluginParameters::LOOPING_HAS_START, loopStartButton),
    loopEndAttachment(p.APVTS(), PluginParameters::LOOPING_HAS_END, loopEndButton),

    // Master module
    monoOutputAttachment(p.APVTS(), PluginParameters::MONO_OUTPUT, monoOutputButton),
    gainSliderAttachment(p.APVTS(), PluginParameters::SAMPLE_GAIN, gainSlider),
    
    // Sample toolbar
    filenameComponent("", {}, true, false, false, p.getWildcardFilter(), "", "Select a file to load..."),
    linkSampleToggle(true, true),
    waveformModeLabel("", "Waveform Mode"),

    playStopButton({}, this),
    deviceSettingsButton(getOutlineFromSVG(BinaryData::IconRecordSettings_svg)),
    audioDeviceSettings(p.getDeviceManager(), 0, 2, true),

    fitButton(getOutlineFromSVG(BinaryData::IconFit_svg)),
    pinButton(true, true),
    pinButtonAttachment(pinButton, pluginState.pinView, &dummyParam),

    // Main controls
    sampleEditor(p.APVTS(), p.getPluginState(), synthVoices, 
        [this](const juce::MouseWheelDetails& details, int center) -> void { sampleNavigator.scrollView(details, center, true); }),
    sampleNavigator(p.APVTS(), p.getPluginState(), synthVoices),

    fxChain(p),

    // Footer
    logo(getOutlineFromSVG(BinaryData::Logo_svg)),
    helpButton(getOutlineFromSVG(BinaryData::IconHelp_svg)),
    darkModeButton(true, true, this),
    helpText("", defaultMessage),
    preFXAttachment(p.APVTS(), PluginParameters::PRE_FX, preFXButton),
    showFXButton(true, true, this),
    showFXAttachment(showFXButton, pluginState.showFX, &dummyParam),
    darkModeAttachment(darkModeButton, pluginState.darkMode, &dummyParam),
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
        setSize(800, 400);
    else
        setSize(width, height);
    setResizable(true, true);
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

    semitoneRotary.getProperties().set(ComponentProps::ROTARY_UNIT, PluginParameters::SEMITONE_UNIT);
    waveformSemitoneRotary.getProperties().set(ComponentProps::ROTARY_UNIT, PluginParameters::SEMITONE_UNIT);
    centRotary.getProperties().set(ComponentProps::ROTARY_UNIT, PluginParameters::CENT_UNIT);
    waveformCentRotary.getProperties().set(ComponentProps::ROTARY_UNIT, PluginParameters::CENT_UNIT);

    tuningDetectButton.onClick = [this] { promptPitchDetection(); };
    tuningDetectButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    tuningDetectButton.setHelpText("Auto-tune to 440hz (experimental)");
    addAndMakeVisible(&tuningDetectButton);

    attackTimeRotary.getProperties().set(ComponentProps::ROTARY_UNIT, PluginParameters::TIME_UNIT);
    attackTimeRotary.getProperties().set(ComponentProps::ROTARY_GREATER_UNIT, PluginParameters::TIME_UNIT_LONG);
    attackTimeRotary.textFromValueFunction = convertWithSecondaryUnit;
    attackTimeRotary.updateText();

    attackCurve.setLookAndFeel(&attackCurveLNF);
    attackCurve.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    attackCurve.setMouseDragSensitivity(100);
    attackCurve.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(&attackCurve);

    releaseTimeRotary.getProperties().set(ComponentProps::ROTARY_UNIT, PluginParameters::TIME_UNIT);
    releaseTimeRotary.getProperties().set(ComponentProps::ROTARY_GREATER_UNIT, PluginParameters::TIME_UNIT_LONG);
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
    playbackSpeedRotary.getProperties().set(ComponentProps::ROTARY_UNIT, PluginParameters::SPEED_UNIT);

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
    gainSlider.setTextValueSuffix(" " + PluginParameters::VOLUME_UNIT);
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

    sampleNavigator.setHelpText("Drag bounds to navigate (shift for constant speed)");
    addAndMakeVisible(sampleNavigator);

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

    fitButton.onClick = [this] { sampleNavigator.fitView(); dummyParam.sendUIUpdate(); };
    fitButton.setHelpText("Fit viewport to bounds");
    addAndMakeVisible(fitButton);

    pinButton.useShape(getOutlineFromSVG(BinaryData::IconPin_svg));
    pinButton.setHelpText("Pin bounds to viewport");
    addAndMakeVisible(pinButton);

    // FX
    addChildComponent(fxChain);

    // Footer
    logo.onClick = [] { juce::URL("https://github.com/BOBONA/Just-a-Sample").launchInDefaultBrowser(); };
    logo.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    logo.setHelpText("Just a Sample v" + juce::String(JUCE_APP_VERSION));
    addAndMakeVisible(logo);

    helpButton.onClick = [this] { juce::URL("https://github.com/BOBONA/Just-a-Sample/blob/master/FEATURES.md").launchInDefaultBrowser(); };
    helpButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    helpButton.setHelpText("More help");
    addAndMakeVisible(helpButton);

    darkModeButton.onStateChange = [this]
        {
            darkModeButton.setCustomHelpText(pluginState.darkMode ? "Light mode" : "Dark mode");
            setTheme(pluginState.darkMode ? darkTheme : defaultTheme);
        };
    darkModeButton.useShape(getOutlineFromSVG(BinaryData::IconLightMode_svg), getOutlineFromSVG(BinaryData::IconDarkMode_svg), juce::Justification::centred);
    darkModeButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible(darkModeButton);

    helpText.setJustificationType(juce::Justification::centred);
    helpText.setHelpText("This is the help text ;)");
    addAndMakeVisible(helpText);

    preFXButton.useShape(getOutlineFromSVG(BinaryData::IconPreFX_svg));
    preFXButton.setHelpText("Apply FX before Attack/Release");
    addAndMakeVisible(&preFXButton);

    showFXButton.onStateChange = [this, minHeight, maxHeight]
    {
        auto constrainedBounds = getConstrainedBounds();
        bool matchesBounds = constrainedBounds.getHeight() == getHeight();

        if ((fxChain.isVisible() != pluginState.showFX) && matchesBounds)
        {
            auto change = fxChain.isVisible() ? -scalei(Layout::fxChainHeight) : scalei(Layout::fxChainHeight);
            setSize(getWidth(), juce::jlimit(minHeight, maxHeight, getHeight() + int(change)));
        }
          
        showFXButton.setHelpText(pluginState.showFX ? "Hide the effects chain" : "Show the effects chain");
        resized();
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

    // Update theme
    darkModeButton.onStateChange();

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

    // If a new device manager state has been loaded from setStateInformation, this will trigger a refresh
    if (audioDeviceSettings.isVisible())
        p.initializeDeviceManager();

    // Handle recording changes (this must take place in a timer callback, since the recorder does not update synchronously)
    if (p.getRecorder().isRecordingDevice())
    {
        if (!sampleEditor.isRecordingMode())
        {
            sampleEditor.setRecordingMode(true);
            sampleNavigator.setRecordingMode(true);
            prompt.openPrompt({ &sampleEditor }, [this] {
                p.getRecorder().stopRecording();
                if (!pendingRecordingBuffer.getNumSamples())  // We'd rather wait until the sample is loaded to stop showing recording
                {
                    sampleEditor.setRecordingMode(false);
                    sampleNavigator.setRecordingMode(false);
                }
                enablementChanged();
            });
        }
        handleActiveRecording();
    }
    else if (sampleEditor.isRecordingMode())
    {
        prompt.closePrompt();
    }

    // We detect sample view scroll gestures as series of continuous scroll events while the view is changing and the mouse is up
    if (zoomGestureEndCallbacks >= 0)
        zoomGestureEndCallbacks--;
    if (mouseWheelDetected && pluginState.viewStart != lastViewStart && pluginState.viewEnd != lastViewEnd && !isMouseButtonDownAnywhere())
        zoomGestureEndCallbacks = maxCallbacksSinceInZoomGesture;

    if (zoomGestureEndCallbacks == 0)
        dummyParam.sendUIUpdate();

    mouseWheelDetected = false;
    lastViewStart = pluginState.viewStart;
    lastViewEnd = pluginState.viewEnd;

    enablementChanged();

    // This is to fix a lag issue found with Reaper and the regular software renderer 
    if (hostType.isReaper() && getPeer() && getPeer()->getCurrentRenderingEngine() == 0 && !openGLContext.isAttached())
        openGLContext.attachTo(*getTopLevelComponent());
}

void JustaSampleAudioProcessorEditor::paint(juce::Graphics& g)
{
    auto colors = getTheme();

    g.fillAll(theme.light);

    auto bounds = getConstrainedBounds().toFloat();

    g.setColour(theme.background);
    g.fillRect(bounds);

    auto controls = bounds.removeFromTop(scalei(Layout::controlsHeight));

    juce::Path toolbarBackground;
    toolbarBackground.addRectangle(controls.toNearestInt());

    g.setColour(theme.foreground);
    g.fillPath(toolbarBackground);

    float divTop = controls.getY() + controls.getHeight() * 0.125f;
    float divBot = controls.getY() + controls.getHeight() * 0.875f;

    g.setColour(theme.slate.withAlpha(0.2f));
    g.drawVerticalLine((tuningDetectButton.getRight() + attackTimeRotary.getX()) / 2, divTop, divBot);
    g.drawVerticalLine((attackCurve.getRight() + releaseTimeRotary.getX()) / 2, divTop, divBot);
    g.drawVerticalLine((releaseCurve.getRight() + lofiModeButton.getX()) / 2, divTop, divBot);
    g.drawVerticalLine((playbackSpeedRotary.getRight() + loopStartButton.getX()) / 2, divTop, divBot);
    g.drawVerticalLine((loopEndButton.getRight() + monoOutputButton.getX()) / 2, divTop, divBot);

    auto footerBounds = bounds.removeFromBottom(scalei(Layout::footerHeight));
    g.setColour(theme.foreground);
    g.fillRect(footerBounds.toNearestInt());

    g.setColour(theme.slate.withAlpha(0.2f));
    g.drawVerticalLine(int(std::round(showFXButton.getX() - scalei(1.f))), footerBounds.getY() + scalei(13.f), footerBounds.getBottom() - scalei(13.f));

    bool fxEnabled = eqEnabled || reverbEnabled || distortionEnabled || chorusEnabled;
    if (fxEnabled)
    {
        auto fxBounds = showFXButton.getBounds().toFloat();
        auto indictatorBounds = fxBounds.removeFromTop(scalei(17.f)).removeFromRight(scalei(17.f)).translated(scalei(12.f), scalei(6.f));
        g.setColour(theme.highlight);
        g.fillEllipse(indictatorBounds);
    }

    auto sampleLoaded = bool(p.getSampleBuffer().getNumSamples());
    if (pluginState.showFX)
    {
        bounds.removeFromBottom(scalei(Layout::fxChainHeight));
    }
    else if (sampleLoaded)
    {
        float border = scalei(2.5f);

        g.setColour(theme.slate.withAlpha(0.5f));
        g.fillRect(bounds.getX(), bounds.getBottom() - border / 2.f, bounds.getWidth(), border);
    }

    if (sampleLoaded || pluginState.showFX)
    {
        auto navigatorBounds = bounds.removeFromBottom(scalei(Layout::sampleNavigatorHeight));
        g.setColour(theme.foreground);
        g.fillRect(navigatorBounds.toNearestInt());
    }
}

void EditorOverlay::paint(juce::Graphics& g)
{
    auto theme = getTheme();

    auto bounds = getLocalBounds().toFloat();
    bounds.removeFromTop(scale(Layout::sampleControlsMargin.getY()));

    auto sampleControls = bounds.removeFromTop(scale(Layout::sampleControlsHeight)).reduced(scale(Layout::sampleControlsMargin.getX()), 0.f);
    auto fileControls = sampleControls.removeFromLeft(scale(Layout::fileControlsWidth));
    auto playbackControls = sampleControls.removeFromRight(scale(Layout::playbackControlsWidth));

    juce::Path sampleControlRegionsPath;
    sampleControlRegionsPath.addRoundedRectangle(fileControls, scale(11.f));
    sampleControlRegionsPath.addRoundedRectangle(playbackControls, scale(11.f));

    sampleControlShadow.render(g, sampleControlRegionsPath);
    g.setColour(theme.foreground);
    g.fillPath(sampleControlRegionsPath);

    if (waveformMode)
    {
        sampleControls.removeFromLeft(scale(Layout::sampleControlsMargin.x));
        auto waveformModeBounds = sampleControls.removeFromLeft(scale(Layout::waveformModeWidth));

        sampleControlRegionsPath.clear();
        sampleControlRegionsPath.addRoundedRectangle(waveformModeBounds, scale(11.f));

        sampleControlShadow.render(g, sampleControlRegionsPath);
        g.setColour(theme.highlight);
        g.fillPath(sampleControlRegionsPath);
    }

    auto navControls = bounds.removeFromBottom(scale(Layout::navigatorControlsSize.y)).removeFromRight(scale(Layout::navigatorControlsSize.x))
        .getSmallestIntegerContainer().toFloat();
    
    juce::Path navControlRegionsPath;
    navControlRegionsPath.addRoundedRectangle(navControls.getX(), navControls.getY(), 
        navControls.getWidth(), navControls.getHeight(), scale(20.f), scale(20.f), 
        true, false, false, false);

    navControlShadow.render(g, navControlRegionsPath);
    g.setColour(theme.foreground);
    g.fillPath(navControlRegionsPath);
}

void EditorOverlay::resized()
{
    sampleControlShadow.setOffset({ int(scale(2.f)), int(scale(2.f)) });
    navControlShadow.setOffset({ int(-scale(2.f)), int(-scale(2.f)) });
}

void EditorOverlay::lookAndFeelChanged()
{
    auto colors = getTheme();

    sampleControlShadow.setColor(colors.slate.withAlpha(0.125f));
    navControlShadow.setColor(colors.slate.withAlpha(0.125f));
}

void JustaSampleAudioProcessorEditor::resized()
{
    pluginState.width = getWidth();
    pluginState.height = getHeight();

    auto bounds = getConstrainedBounds().toFloat();

    prompt.setBounds(bounds.toNearestInt());

    audioDeviceSettings.setBounds(juce::Rectangle(scalef(1200.f), scalef(700.f)).withCentre(bounds.getCentre()).toNearestInt());

    // Layout the controls toolbar
    auto controls = bounds.removeFromTop(scalei(Layout::controlsHeight));
    controls.removeFromLeft(scalei(Layout::controlsPaddingX));
    controls.removeFromTop(scalei(8));
    controls.removeFromBottom(scalei(10));
    
    juce::Array labels = { &tuningLabel, &attackLabel, &releaseLabel, &playbackLabel, &loopingLabel, &masterLabel };
    for (juce::Label* label : labels)
        label->setFont(getInriaSans().withHeight(scalef(34.3f)));

    // Tuning module
    auto tuningModule = controls.removeFromLeft(scalei(Layout::tuningWidth));

    auto tuningLabelBounds = tuningModule.removeFromTop(scalei(Layout::moduleLabelHeight));
    tuningLabelBounds.reduce(0, scalei(Layout::moduleLabelPadding));
    tuningLabel.setBounds(tuningLabelBounds.toNearestInt());
    tuningModule.removeFromTop(scalei(Layout::moduleLabelGap));

    // Rotaries take up a bit of extra space for the thumb, so we need to compensate by subtracting from other values
    float rotarySize = scalei(Layout::standardRotarySize);
    float rotaryPadding = rotarySize * Layout::rotaryPadding;

    tuningModule.reduce(scalei(16.f) - rotaryPadding, scalei(6.f) - rotaryPadding);

    auto semitoneBounds = tuningModule.removeFromLeft(rotarySize + 2 * rotaryPadding);
    semitoneRotary.setBounds(semitoneBounds.toNearestInt());
    semitoneRotary.sendLookAndFeelChange();
    waveformSemitoneRotary.setBounds(semitoneBounds.toNearestInt());
    waveformSemitoneRotary.sendLookAndFeelChange();
    tuningModule.removeFromLeft(scalei(Layout::moduleControlsGap) - 2 * rotaryPadding);

    auto centBounds = tuningModule.removeFromLeft(rotarySize + 2 * rotaryPadding);
    centRotary.setBounds(centBounds.toNearestInt());
    centRotary.sendLookAndFeelChange();
    waveformCentRotary.setBounds(centBounds.toNearestInt());
    waveformCentRotary.sendLookAndFeelChange();
    tuningModule.removeFromLeft(scalei(Layout::moduleControlsGap) - rotaryPadding);

    auto detectTuningBounds = tuningModule.removeFromLeft(scalei(71.f)).reduced(0.f, rotaryPadding);
    tuningDetectLabel.setBounds(detectTuningBounds.removeFromTop(scalei(29.f)).expanded(scalei(0.01f), 0.f).toNearestInt());
    tuningDetectLabel.setFont(getInriaSansBold().withHeight(scalef(27.5f)));
    detectTuningBounds.removeFromTop(scalei(5.f));
    tuningDetectButton.setBounds(detectTuningBounds.removeFromTop(scalei(46.f)).toNearestInt());

    controls.removeFromLeft(scalei(Layout::moduleGap));

    // Attack module
    auto attackModule = controls.removeFromLeft(scalei(Layout::attackWidth));

    auto attackLabelBounds = attackModule.removeFromTop(scalei(Layout::moduleLabelHeight));
    attackLabelBounds.reduce(0, scalei(Layout::moduleLabelPadding));
    attackLabel.setBounds(attackLabelBounds.toNearestInt());

    attackModule.removeFromTop(scalei(Layout::moduleLabelGap));
    attackModule.reduce(scalei(16.f) - rotaryPadding, scalei(6.f) - rotaryPadding);

    auto attackTimeBounds = attackModule.removeFromLeft(rotarySize + 2 * rotaryPadding);
    attackTimeRotary.setBounds(attackTimeBounds.toNearestInt());
    attackTimeRotary.sendLookAndFeelChange();
    attackModule.removeFromLeft(scalei(Layout::moduleControlsGap) - 2 * rotaryPadding);

    auto attackCurveBounds = attackModule.removeFromLeft(scalei(58.f) + 2 * rotaryPadding).reduced(0.f, scalei(16.5f));
    attackCurve.setBounds(attackCurveBounds.toNearestInt());

    controls.removeFromLeft(scalei(Layout::moduleGap));

    // Release module
    auto releaseModule = controls.removeFromLeft(scalei(Layout::releaseWidth));

    auto releaseLabelBounds = releaseModule.removeFromTop(scalei(Layout::moduleLabelHeight));
    releaseLabelBounds.reduce(0, scalei(Layout::moduleLabelPadding));
    releaseLabel.setBounds(releaseLabelBounds.toNearestInt());

    releaseModule.removeFromTop(scalei(Layout::moduleLabelGap));
    releaseModule.reduce(scalei(16.f) - rotaryPadding, scalei(6.f) - rotaryPadding);

    auto releaseTimeBounds = releaseModule.removeFromLeft(rotarySize + 2 * rotaryPadding);
    releaseTimeRotary.setBounds(releaseTimeBounds.toNearestInt());
    releaseTimeRotary.sendLookAndFeelChange();
    releaseModule.removeFromLeft(scalei(Layout::moduleControlsGap) - 2 * rotaryPadding);

    auto releaseCurveBounds = releaseModule.removeFromLeft(scalei(58.f) + 2 * rotaryPadding).reduced(0.f, scalei(16.5f));
    releaseCurve.setBounds(releaseCurveBounds.toNearestInt());

    controls.removeFromLeft(scalei(Layout::moduleGap));

    // Playback module
    auto playbackModule = controls.removeFromLeft(scalei(Layout::playbackWidth));

    auto playbackLabelBounds = playbackModule.removeFromTop(scalei(Layout::moduleLabelHeight));
    playbackLabelBounds.reduce(0, scalei(Layout::moduleLabelPadding));
    playbackLabel.setBounds(playbackLabelBounds.toNearestInt());

    playbackModule.removeFromTop(scalei(Layout::moduleLabelGap));
    playbackModule.reduce(scalei(16.f) - rotaryPadding, scalei(6.f) - rotaryPadding);
    playbackModule.removeFromLeft(rotaryPadding);

    auto lofiButtonBounds = playbackModule.removeFromLeft(scalei(71.f)).reduced(0.f, rotaryPadding + scalei(7.5f));
    lofiModeButton.setBounds(lofiButtonBounds.toNearestInt());
    lofiModeButton.setBorder(scalef(2.5f), scalef(6.f));
    lofiModeButton.setPadding(scalef(12.f));

    playbackModule.removeFromLeft(scalei(Layout::moduleControlsGap));
    playbackModeButton.setBounds(playbackModule.removeFromLeft(scalef(300.f)).reduced(0.f, rotaryPadding).toNearestInt());
    playbackModeButton.setBorder(scalef(2.5f), scalef(6.f));

    playbackModule.removeFromLeft(scalei(Layout::moduleControlsGap) - rotaryPadding);
    playbackSpeedRotary.setBounds(playbackModule.removeFromLeft(rotarySize + 2 * rotaryPadding).toNearestInt());
    playbackSpeedRotary.sendLookAndFeelChange();
    
    controls.removeFromLeft(scalei(Layout::moduleGap));

    // Loop module
    auto loopModule = controls.removeFromLeft(scalei(Layout::loopWidth));

    auto loopingLabelBounds = loopModule.removeFromTop(scalei(Layout::moduleLabelHeight));
    loopingLabelBounds.reduce(0, scalei(Layout::moduleLabelPadding));
    loopingLabel.setBounds(loopingLabelBounds.toNearestInt());

    loopModule.removeFromTop(scalei(Layout::moduleLabelGap));
    loopModule.reduce(scalei(16.f), scalei(6.f));

    loopStartButton.setBounds(loopModule.removeFromLeft(scalei(34.f)).toNearestInt());
    loopStartButton.setBorder(scalef(2.5f), scalef(6.f), true, false, true, false);
    loopStartButton.setPadding(scalef(8.f), scalef(4.f), scalef(7.5f), scalef(7.5f));

    loopModule.removeFromLeft(scalei(6.f));
    loopButton.setBounds(loopModule.removeFromLeft(scalei(105.f)).toNearestInt());
    loopButton.setBorder(scalef(2.5f));
    loopButton.setPadding(scalef(13.f));

    loopModule.removeFromLeft(scalei(6.f));
    loopEndButton.setBounds(loopModule.removeFromLeft(scalei(34.f)).toNearestInt());
    loopEndButton.setBorder(scalef(2.5f), scalef(6.f), false, true, false, true);
    loopEndButton.setPadding(scalef(4.f), scalef(8.f), scalef(7.5f), scalef(7.5f));

    controls.removeFromLeft(scalei(Layout::moduleGap));

    // Master module
    auto masterModule = controls.removeFromLeft(scalei(Layout::masterWidth));

    auto masterLabelBounds = masterModule.removeFromTop(scalei(Layout::moduleLabelHeight));
    masterLabelBounds.reduce(0, scalei(Layout::moduleLabelPadding));
    masterLabel.setBounds(masterLabelBounds.toNearestInt());

    masterModule.removeFromTop(scalei(Layout::moduleLabelGap));
    masterModule.reduce(scalei(16.f), 0.f);

    auto monoButtonBounds = masterModule.removeFromLeft(scalei(92.f)).reduced(0.f, scalei(27.5f));
    monoOutputButton.setBounds(monoButtonBounds.toNearestInt());
    monoOutputButton.setBorder(scalef(2.5f), scalef(6.f));
    monoOutputButton.setPadding(scalef(12.f));

    masterModule.removeFromLeft(scalei(Layout::moduleControlsGap));
    gainSlider.setBounds(masterModule.removeFromLeft(scalei(153.f)).toNearestInt());
    gainSlider.sendLookAndFeelChange();

    // Footer
    auto footer = bounds.removeFromBottom(scalei(Layout::footerHeight));
    footer.removeFromBottom(scalei(2.f));
    helpText.setFont(getInter().withHeight(scalef(41.4f)));
    helpText.setBounds(footer.reduced(footer.getWidth() * 0.2f, 0.f).toNearestInt());

    footer.reduce(scalei(25.f), 0.f);
    
    auto logoBounds = footer.removeFromLeft(scalei(260.f));
    logo.setBounds(logoBounds.toNearestInt());

    footer.removeFromLeft(scalei(18.5f));
    auto helpButtonBounds = footer.removeFromLeft(scalei(43.f)).reduced(0.f, scalei(13.5f));
    helpButton.setBounds(helpButtonBounds.toNearestInt());

    footer.removeFromLeft(scalei(15.f));
    auto darkModeButtonBounds = footer.removeFromLeft(scalei(43.f)).reduced(0.f, scalei(13.5f));
    darkModeButton.setBounds(darkModeButtonBounds.toNearestInt());

    auto showFXButtonBounds = footer.removeFromRight(scalei(205.f));
    showFXButton.setPadding(0.f, 0.f, scalei(19.f), scalei(19.f));
    showFXButton.setBounds(showFXButtonBounds.toNearestInt());

    footer.removeFromRight(scalei(30.f));

    auto preFXButtonBounds = footer.removeFromRight(scalei(100.f)).reduced(0.f, scalei(11.f));
    preFXButton.setBounds(preFXButtonBounds.toNearestInt());
    preFXButton.setBorder(scalef(2.5f), scalef(6.f));
    preFXButton.setPadding(scalef(11.f));

    // FX
    if (pluginState.showFX)
    {
        auto fxChainBounds = bounds.removeFromBottom(scalei(Layout::fxChainHeight));
        fxChain.setBounds(fxChainBounds.toNearestInt());
    }
    fxChain.setVisible(pluginState.showFX);

    // Navigator
    auto sampleLoaded = bool(p.getSampleBuffer().getNumSamples());
    if (sampleLoaded || pluginState.showFX)
    {
        auto sampleNavigatorBounds = bounds.removeFromBottom(scalei(Layout::sampleNavigatorHeight));
        sampleNavigator.setBounds(sampleNavigatorBounds.toNearestInt());
    }

    // Editor
    auto editorBounds = bounds;
    sampleEditor.setBounds(editorBounds.toNearestInt());
    editorOverlay.setBounds(editorBounds.toNearestInt());

    statusLabel.setFont(getInter().withHeight(scalef(50.f)));
    updateLabel();

    // Sample controls
    editorBounds.removeFromTop(scalei(Layout::sampleControlsMargin.getY()));

    auto sampleControls = editorBounds.removeFromTop(scalei(Layout::sampleControlsHeight)).reduced(scalei(Layout::sampleControlsMargin.getX()), 0.f);
    auto fileControls = sampleControls.removeFromLeft(scalei(Layout::fileControlsWidth));
    auto playbackControls = sampleControls.removeFromRight(scalei(Layout::playbackControlsWidth));

    fileControls.reduce(scalei(12.f), scalei(2.1f));
    auto filenameComponentBounds = fileControls.removeFromLeft(scalei(1095.f));
    filenameComponent.setBounds(filenameComponentBounds.toNearestInt());

    fileControls.removeFromLeft(scalei(20.f));
    auto linkSampleToggleBounds = fileControls.removeFromLeft(scalei(48.f)).reduced(0.f, scalei(2.85f));
    linkSampleToggle.setBounds(linkSampleToggleBounds.toNearestInt());
    linkSampleToggle.setPadding(scalef(3.f));
    linkSampleToggle.setBorder(0.f, scalef(5.f));

    sampleControls.removeFromLeft(scalei(Layout::sampleControlsMargin.x));
    auto waveformModeBounds = sampleControls.removeFromLeft(scalei(Layout::waveformModeWidth)).reduced(scalei(12.f), scalei(2.13f));

    waveformModeLabel.setFont(getInter().withHeight(scalei(31.8f)));
    waveformModeLabel.setBounds(waveformModeBounds.toNearestInt());

    playbackControls.reduce(scalei(12.f), scalei(2.1f));
    auto playStopButtonBounds = playbackControls.removeFromLeft(scalei(40.32f)).reduced(scalei(4.98f), scalei(3.36f));
    playStopButton.setBounds(playStopButtonBounds.toNearestInt());

    playbackControls.removeFromLeft(scalei(14.f));
    auto recordControlBounds = playbackControls.removeFromLeft(scalei(58.6f)).reduced(scalei(2.f), scalei(3.36f));
    auto recordButtonBounds = recordControlBounds.removeFromLeft(scalei(33.6f));
    recordButton.setBounds(recordButtonBounds.toNearestInt());

    recordControlBounds.removeFromLeft(scalei(4.f));
    auto deviceSettingsButtonBounds = recordControlBounds.removeFromTop(scalei(23.f));
    deviceSettingsButton.setBounds(deviceSettingsButtonBounds.toNearestInt());

    auto navControlBounds = bounds.removeFromBottom(scalei(Layout::navigatorControlsSize.y)).removeFromRight(scalei(Layout::navigatorControlsSize.x));
    navControlBounds.removeFromLeft(scalei(20.f));

    auto fitButtonBounds = navControlBounds.removeFromLeft(scalei(34.f));
    fitButtonBounds.removeFromTop(scalei(10.f));
    fitButtonBounds.removeFromBottom(scalei(5.f));
    fitButton.setBounds(fitButtonBounds.toNearestInt());

    navControlBounds.removeFromLeft(scalei(12.f));
    auto pinButtonBounds = navControlBounds.removeFromLeft(scalei(44.f));
    pinButtonBounds.removeFromTop(scalei(7.5f));
    pinButtonBounds.removeFromBottom(scalei(2.5f));
    pinButton.setBounds(pinButtonBounds.toNearestInt());
    pinButton.setBorder(0.f, scalef(5.f));
    pinButton.setPadding(scalef(5.f));

    repaint();
}

void JustaSampleAudioProcessorEditor::lookAndFeelChanged()
{
    const auto shadow = theme.dark.withAlpha(0.25f);

    lofiModeButton.setColors(theme.darkerSlate, theme.light, shadow);
    loopButton.setColors(theme.darkerSlate, theme.loop, shadow);
    loopStartButton.setColors(theme.darkerSlate, theme.loop, shadow);
    loopEndButton.setColors(theme.darkerSlate, theme.loop, shadow);
    monoOutputButton.setColors(theme.darkerSlate, theme.light, shadow);
    linkSampleToggle.setColors(theme.dark, theme.slate.withAlpha(0.5f), shadow);
    pinButton.setColors(theme.dark, theme.slate.withAlpha(0.5f), shadow);
    darkModeButton.setColors(theme.dark, theme.slate.withAlpha(0.f), shadow);
    preFXButton.setColors(theme.darkerSlate, theme.light, shadow);
    showFXButton.setColors(theme.dark, theme.slate.withAlpha(0.f), shadow);

    tuningDetectButton.setColor(theme.dark);
    playStopButton.setColor(theme.dark);
    recordButton.setColor(theme.highlight);
    deviceSettingsButton.setColor(theme.dark);
    fitButton.setColor(theme.dark);
    logo.setColor(theme.dark);
    helpButton.setColor(theme.dark);

    playbackModeButton.setColors(theme.slate, theme.light, shadow);

    statusLabel.setColour(juce::Label::textColourId, theme.dark);
    statusLabel.setColour(juce::Label::backgroundColourId, theme.background.withAlpha(0.85f));
    waveformModeLabel.setColour(juce::Label::textColourId, theme.foreground);
    sampleNavigator.setColour(Colors::painterColorId, theme.darkerSlate);

    juce::Array<Component*> foregroundComponents = {
        &tuningLabel, &attackLabel, &releaseLabel, &playbackLabel, &loopingLabel, &masterLabel, &semitoneRotary, &waveformSemitoneRotary, &centRotary, &waveformCentRotary, &tuningDetectLabel, &tuningDetectButton,
        &attackTimeRotary, &attackCurve, &releaseTimeRotary, &releaseCurve, &lofiModeButton, &playbackModeButton, &playbackSpeedRotary, &loopStartButton, &loopButton, &loopEndButton,
        &monoOutputButton, &gainSlider, &filenameComponent, &linkSampleToggle, &playStopButton, &recordButton, &deviceSettingsButton, &fitButton, &pinButton, &sampleNavigator, &preFXButton, &showFXButton
    };

    for (Component* component : foregroundComponents)
        component->setColour(Colors::backgroundColorId, theme.foreground);
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

void JustaSampleAudioProcessorEditor::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&)
{
    mouseWheelDetected = true;
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

void JustaSampleAudioProcessorEditor::mouseExit(const juce::MouseEvent&)
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

void JustaSampleAudioProcessorEditor::setTheme(const Colors newTheme)
{
    theme = newTheme;
    lnf.setTheme(theme);
    attackCurveLNF.setTheme(theme);
    releaseCurveLNF.setTheme(theme);
    gainSliderLNF.setTheme(theme);

    enablementChanged();
    sendLookAndFeelChange();
}

Colors JustaSampleAudioProcessorEditor::getTheme() const
{
    return theme;
}

juce::Rectangle<int> JustaSampleAudioProcessorEditor::getConstrainedBounds() const
{
    auto fxSpace = !pluginState.showFX ? scalef(Layout::fxChainHeight) : 0;

    auto bounds = getLocalBounds();
    auto width = getWidth();
    auto height = getHeight();
    auto constrainedWidth = int(juce::jlimit<float>((height + fxSpace) * 0.8f, (height + fxSpace) * 2.f, float(width)));
    auto constrainedHeight = int(width / 0.8f - fxSpace);
    
    if (constrainedWidth < width)
        bounds.reduce((width - constrainedWidth) / 2, 0);
    else if (constrainedHeight < height)
        bounds.reduce(0, (height - constrainedHeight) / 2);

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
        sampleEditor.setSample(p.getSampleBuffer(), p.getBufferSampleRate(), userDraggedSample);
        sampleNavigator.setSample(p.getSampleBuffer(), p.getBufferSampleRate(), userDraggedSample);
        dummyParam.sendUIUpdate();
    }
    sampleEditor.setRecordingMode(false);
    sampleNavigator.setRecordingMode(false);
    userDraggedSample = false;
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
    p.initializeDeviceManager();

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
        dummyParam.sendUIUpdate();
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
    editorOverlay.setWaveformMode(isSampleLoaded && isWaveformMode);

    const auto navigatorWasVisible = sampleNavigator.isVisible();
    sampleNavigator.setVisible(isSampleLoaded || pluginState.showFX);
    if (navigatorWasVisible != sampleNavigator.isVisible())
        resized();

    preFXButton.setEnabled(isSampleLoaded && (reverbEnabled || distortionEnabled || chorusEnabled || eqEnabled)),

    // Handle status label
    statusLabel.setVisible(!p.getSampleBuffer().getNumSamples() || p.getSampleLoader().isLoading() || sampleEditor.isInBoundsSelection() || sampleEditor.isRecordingMode() || fileDragging);
    statusLabel.setColour(juce::Label::outlineColourId, theme.dark.withAlpha(float(fileDragging)));
    if (p.getSampleLoader().isLoading())
        updateLabel("Loading...");
    else if (sampleEditor.isInBoundsSelection())
        updateLabel("Select a region to analyze");
    else if (sampleEditor.isRecordingMode())
        updateLabel("Recording...");
    else if (!p.getSampleBuffer().getNumSamples())
        updateLabel("Drop a sample!");
    else if (fileDragging)
        updateLabel("Replace sample");
}

void JustaSampleAudioProcessorEditor::fxEnablementChanged()
{
    enablementChanged();
    repaint(showFXButton.getBounds().expanded(int(scalei(15.f)), 0));
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
            userDraggedSample = true;
            p.loadSampleFromPath(file, true, "", false, 
                [this](bool loaded) { if (!loaded) userDraggedSample = false; });
            break;
        }
    }
}

void JustaSampleAudioProcessorEditor::filenameComponentChanged(juce::FilenameComponent* fileComponentThatHasChanged)
{
    juce::File file = fileComponentThatHasChanged->getCurrentFile();
    userDraggedSample = true;
    p.loadSampleFromPath(file.getFullPathName(), true, "", false, [this, fileComponentThatHasChanged, file](bool fileLoaded) -> void
    {
        if (!fileLoaded)
        {
            fileComponentThatHasChanged->setCurrentFile(juce::File(pluginState.filePath), false, juce::dontSendNotification);
            auto recentFiles = fileComponentThatHasChanged->getRecentlyUsedFilenames();
            recentFiles.removeString(file.getFullPathName());
            fileComponentThatHasChanged->setRecentlyUsedFilenames(recentFiles);
            userDraggedSample = false;
        }
    });
}

void JustaSampleAudioProcessorEditor::fileDragEnter(const juce::StringArray& files, int /*x*/, int /*y*/)
{
    if (isInterestedInFileDrag(files))
    {
        fileDragging = true;
        enablementChanged();
    }
}

void JustaSampleAudioProcessorEditor::fileDragExit(const juce::StringArray&)
{
    fileDragging = false;
    enablementChanged();
}

void JustaSampleAudioProcessorEditor::updateLabel(const juce::String& text)
{
    if (text.isNotEmpty())
        statusLabel.setText(text, juce::dontSendNotification);

    auto labelWidth = juce::TextLayout::getStringWidth(statusLabel.getFont(), statusLabel.getText());
    statusLabel.setBounds(juce::Rectangle(0, 0, int(labelWidth), int(statusLabel.getFont().getHeight()))
        .withCentre(sampleEditor.getBounds().getCentre()).expanded(int(scalei(40.f))).toNearestInt());
}

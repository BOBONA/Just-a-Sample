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
    magicPitchButton("Detect_Pitch", Colours::white, Colours::lightgrey, Colours::darkgrey),
    reverbModule(processor.apvts, "Reverb")
{
    if (hostType.isReaper())
    {
        openGLContext.attachTo(*getTopLevelComponent());
    }

    p.apvts.state.addListener(this);

    setResizeLimits(250, 200, 1000, 800);
    setResizable(false, true);
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
    
    fileLabel.setText("File not selected", dontSendNotification);
    fileLabel.setFont(15);
    addAndMakeVisible(fileLabel);

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
        if (!processor.isPitchDetecting && playbackOptions.isEnabled() && processor.pitchDetectionRoutine())
        {
            sampleEditor.setEnabled(false);
            sampleNavigator.setEnabled(false);
            magicPitchButton.setEnabled(false);
            magicPitchButton.setColours(Colours::white, Colours::white, Colours::white);
            samplePortionEnabled = false;
            processor.isPitchDetecting = true;
        }
        };
    sampleRequiredControls.add(&magicPitchButton);
    addAndMakeVisible(magicPitchButton);

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

    reverbModule.addRow({
        ModuleControl{"Size", PluginParameters::REVERB_SIZE, ModuleControl::ROTARY}, 
        {"Damping", PluginParameters::REVERB_DAMPING, ModuleControl::ROTARY} 
    });
    reverbModule.addRow({
        ModuleControl{"Width/Lowpass", PluginParameters::REVERB_PARAM1, ModuleControl::ROTARY}, 
        {"Freeze Mode/Highpass/Decay", PluginParameters::REVERB_PARAM2, ModuleControl::ROTARY}
    });
    reverbModule.addRow({
        ModuleControl{"None/Predelay", PluginParameters::REVERB_PARAM3, ModuleControl::ROTARY}, 
        {"Wet/Dry", PluginParameters::REVERB_WET_MIX, ModuleControl::ROTARY}
    });
    addAndMakeVisible(reverbModule);

    for (Component* comp : sampleRequiredControls)
    {
        if (comp)
        {
            comp->setEnabled(false);
        }
    }

    addAndMakeVisible(sampleEditor);
    addAndMakeVisible(sampleNavigator);
    updateWorkingSample();

    startTimerHz(60);
}

JustaSampleAudioProcessorEditor::~JustaSampleAudioProcessorEditor()
{
    processor.apvts.state.removeListener(this);
}

void JustaSampleAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll(lnf.BACKGROUND_COLOR);
}

void JustaSampleAudioProcessorEditor::resized()
{
    processor.editorWidth = getWidth();
    processor.editorHeight = getHeight();
    auto bounds = getLocalBounds();

    FlexBox topControls{ FlexBox::Direction::row, FlexBox::Wrap::wrap, FlexBox::AlignContent::stretch, 
        FlexBox::AlignItems::stretch, FlexBox::JustifyContent::flexEnd };
    topControls.items.add(FlexItem(fileLabel).withFlex(1).withMinWidth(getWidth()));
    topControls.items.add(FlexItem(tuningLabel).withMinWidth(50));
    topControls.items.add(FlexItem(semitoneSlider).withMinWidth(30));
    topControls.items.add(FlexItem(centSlider).withMinWidth(40));
    topControls.items.add(FlexItem(magicPitchButton).withMinWidth(15));
    topControls.items.add(FlexItem(playbackOptions).withMinWidth(150));
    topControls.items.add(FlexItem(isLoopingLabel).withMinWidth(35));
    topControls.items.add(FlexItem(isLoopingButton).withMinWidth(20));
    topControls.items.add(FlexItem(masterGainSlider).withMinWidth(60));
    float totalWidth = 0;
    for (const FlexItem& item : topControls.items)
    {
        totalWidth += item.minWidth;
    }
    auto top = bounds.removeFromTop(15 * ceil(totalWidth / getWidth()));
    topControls.performLayout(top);

    auto editor = bounds.removeFromTop(bounds.getHeight() * 0.66f);
    auto navigator = bounds.removeFromTop(bounds.getHeight() * 0.2f);

    sampleEditor.setBounds(editor);
    sampleNavigator.setBounds(navigator);

    auto moduleWidth = bounds.getWidth() / 4;
    reverbModule.setBounds(bounds.removeFromLeft(moduleWidth));
}

void JustaSampleAudioProcessorEditor::timerCallback()
{
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
    if (processor.isPitchDetecting)
    {
        magicPitchButtonShape.applyTransform(AffineTransform::rotation(MathConstants<float>::pi / 180));
        magicPitchButton.setShape(magicPitchButtonShape, false, true, false);
    }
    if (!processor.isPitchDetecting && !samplePortionEnabled)
    {
        samplePortionEnabled = true;
        sampleEditor.setEnabled(true);
        sampleNavigator.setEnabled(true);
        magicPitchButton.setEnabled(true);
        magicPitchButton.setColours(Colours::white, Colours::lightgrey, Colours::darkgrey);
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

void JustaSampleAudioProcessorEditor::filesDropped(const StringArray& files, int x, int y)
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

void JustaSampleAudioProcessorEditor::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property)
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
        fileLabel.setText(processor.apvts.state.getProperty(PluginParameters::FILE_PATH), dontSendNotification);
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

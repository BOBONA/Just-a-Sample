#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginParameters.h"

JustaSampleAudioProcessor::JustaSampleAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), 
    apvts(*this, &undoManager, "Parameters", createParameterLayout()),
    fileFilter("", {}, {})
#endif
{
    apvts.addParameterListener(PluginParameters::PLAYBACK_MODE, this);
    apvts.addParameterListener(PluginParameters::IS_LOOPING, this);
    apvts.addParameterListener(PluginParameters::LOOPING_HAS_START, this);
    apvts.addParameterListener(PluginParameters::LOOPING_HAS_END, this);
    apvts.state.addListener(this);
    formatManager.registerBasicFormats();
    fileFilter = WildcardFileFilter(formatManager.getWildcardForAllFormats(), {}, {});
    setProperLatency();
    pitchDetector.addListener(this);
}

JustaSampleAudioProcessor::~JustaSampleAudioProcessor()
{
    pitchDetector.removeListener(this);
}

const juce::String JustaSampleAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool JustaSampleAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool JustaSampleAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool JustaSampleAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double JustaSampleAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int JustaSampleAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int JustaSampleAudioProcessor::getCurrentProgram()
{
    return 0;
}

void JustaSampleAudioProcessor::setCurrentProgram (int)
{
}

const juce::String JustaSampleAudioProcessor::getProgramName (int)
{
    return {};
}

void JustaSampleAudioProcessor::changeProgramName(int, const juce::String&)
{
}

void JustaSampleAudioProcessor::prepareToPlay(double sampleRate, int)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
    setProperLatency();
    resetSamplerVoices();
}

void JustaSampleAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool JustaSampleAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void JustaSampleAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
}

bool JustaSampleAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* JustaSampleAudioProcessor::createEditor()
{
    LookAndFeel::setDefaultLookAndFeel(&lookAndFeel);
    return new JustaSampleAudioProcessorEditor(*this);
}

void JustaSampleAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    apvts.state.setProperty(PluginParameters::WIDTH, editorWidth, apvts.undoManager);
    apvts.state.setProperty(PluginParameters::HEIGHT, editorHeight, apvts.undoManager);
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void JustaSampleAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid())
    {
        apvts.replaceState(tree);
        loadFile(apvts.state.getProperty(PluginParameters::FILE_PATH));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout JustaSampleAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<AudioParameterChoice>(
        PluginParameters::PLAYBACK_MODE, PluginParameters::PLAYBACK_MODE, PluginParameters::PLAYBACK_MODE_LABELS, 1));
    layout.add(std::make_unique<AudioParameterBool>(
        PluginParameters::SKIP_ANTIALIASING, PluginParameters::SKIP_ANTIALIASING, true));
    layout.add(std::make_unique<AudioParameterBool>(
        PluginParameters::IS_LOOPING, PluginParameters::IS_LOOPING, false));
    layout.add(std::make_unique<AudioParameterFloat>(
        PluginParameters::MASTER_GAIN, PluginParameters::MASTER_GAIN, PluginParameters::MASTER_GAIN_RANGE_DB, 0.f));
    layout.add(std::make_unique<AudioParameterInt>(
        PluginParameters::SEMITONE_TUNING, PluginParameters::SEMITONE_TUNING, PluginParameters::SEMITONE_TUNING_RANGE.getStart(), PluginParameters::SEMITONE_TUNING_RANGE.getEnd(), 0));
    layout.add(std::make_unique<AudioParameterInt>(
        PluginParameters::CENT_TUNING, PluginParameters::CENT_TUNING, PluginParameters::CENT_TUNING_RANGE.getStart(), PluginParameters::CENT_TUNING_RANGE.getEnd(), 0));
    layout.add(std::make_unique<AudioParameterInt>(
        PluginParameters::FX_PERM, PluginParameters::FX_PERM, 0, 23, PluginParameters::permToParam({ PluginParameters::DISTORTION, PluginParameters::CHORUS, PluginParameters::REVERB, PluginParameters::EQ })));
    layout.add(std::make_unique<AudioParameterBool>(
        PluginParameters::MONO_OUTPUT, PluginParameters::MONO_OUTPUT, false));
    layout.add(std::make_unique<AudioParameterFloat>(
        PluginParameters::SPEED_FACTOR, PluginParameters::SPEED_FACTOR, PluginParameters::SPEED_FACTOR_RANGE, 1.f));
    layout.add(std::make_unique<AudioParameterBool>(
        PluginParameters::FORMANT_PRESERVED, PluginParameters::FORMANT_PRESERVED, false));

    layout.add(std::make_unique<AudioParameterBool>(
        PluginParameters::REVERB_ENABLED, PluginParameters::REVERB_ENABLED, false));
    layout.add(std::make_unique<AudioParameterFloat>(
        PluginParameters::REVERB_MIX, PluginParameters::REVERB_MIX, PluginParameters::REVERB_MIX_RANGE.getStart(), PluginParameters::REVERB_MIX_RANGE.getEnd(), 0.5f));
    layout.add(std::make_unique<AudioParameterFloat>(
        PluginParameters::REVERB_SIZE, PluginParameters::REVERB_SIZE, PluginParameters::REVERB_SIZE_RANGE.getStart(), PluginParameters::REVERB_SIZE_RANGE.getEnd(), 0.5f));
    layout.add(std::make_unique<AudioParameterFloat>(
        PluginParameters::REVERB_DAMPING, PluginParameters::REVERB_DAMPING, PluginParameters::REVERB_DAMPING_RANGE.getStart(), PluginParameters::REVERB_DAMPING_RANGE.getEnd(), 0.5f));
    layout.add(std::make_unique<AudioParameterFloat>(
        PluginParameters::REVERB_LOWS, PluginParameters::REVERB_LOWS, PluginParameters::REVERB_LOWS_RANGE.getStart(), PluginParameters::REVERB_LOWS_RANGE.getEnd(), 0.5f));
    layout.add(std::make_unique<AudioParameterFloat>(
        PluginParameters::REVERB_HIGHS, PluginParameters::REVERB_HIGHS, PluginParameters::REVERB_HIGHS_RANGE.getStart(), PluginParameters::REVERB_LOWS_RANGE.getEnd(), 0.5f));
    layout.add(std::make_unique<AudioParameterFloat>(
        PluginParameters::REVERB_PREDELAY, PluginParameters::REVERB_PREDELAY, PluginParameters::REVERB_PREDELAY_RANGE, 0.5f));
    
    layout.add(std::make_unique<AudioParameterBool>(
        PluginParameters::DISTORTION_ENABLED, PluginParameters::DISTORTION_ENABLED, false));
    layout.add(std::make_unique<AudioParameterFloat>(
        PluginParameters::DISTORTION_MIX, PluginParameters::DISTORTION_MIX, PluginParameters::DISTORTION_MIX_RANGE.getStart(), PluginParameters::DISTORTION_MIX_RANGE.getEnd(), 1.f));
    layout.add(std::make_unique<AudioParameterFloat>(
        PluginParameters::DISTORTION_HIGHPASS, PluginParameters::DISTORTION_HIGHPASS, PluginParameters::DISTORTION_HIGHPASS_RANGE.getStart(), PluginParameters::DISTORTION_HIGHPASS_RANGE.getEnd(), 0.f));
    layout.add(std::make_unique<AudioParameterFloat>(
        PluginParameters::DISTORTION_DENSITY, PluginParameters::DISTORTION_DENSITY, PluginParameters::DISTORTION_DENSITY_RANGE.getStart(), PluginParameters::DISTORTION_DENSITY_RANGE.getEnd(), 0.f));
   
    layout.add(std::make_unique<AudioParameterBool>(
        PluginParameters::EQ_ENABLED, PluginParameters::EQ_ENABLED, false));
    layout.add(std::make_unique<AudioParameterFloat>(
        PluginParameters::EQ_LOW_GAIN, PluginParameters::EQ_LOW_GAIN, PluginParameters::EQ_LOW_GAIN_RANGE.getStart(), PluginParameters::EQ_LOW_GAIN_RANGE.getEnd(), 0.f));
    layout.add(std::make_unique<AudioParameterFloat>(
        PluginParameters::EQ_MID_GAIN, PluginParameters::EQ_MID_GAIN, PluginParameters::EQ_MID_GAIN_RANGE.getStart(), PluginParameters::EQ_MID_GAIN_RANGE.getEnd(), 0.f));
    layout.add(std::make_unique<AudioParameterFloat>(
        PluginParameters::EQ_HIGH_GAIN, PluginParameters::EQ_HIGH_GAIN, PluginParameters::EQ_HIGH_GAIN_RANGE.getStart(), PluginParameters::EQ_HIGH_GAIN_RANGE.getEnd(), 0.f));
    layout.add(std::make_unique<AudioParameterFloat>(
        PluginParameters::EQ_LOW_FREQ, PluginParameters::EQ_LOW_FREQ, PluginParameters::EQ_LOW_FREQ_RANGE.getStart(), PluginParameters::EQ_LOW_FREQ_RANGE.getEnd(), 200.f));
    layout.add(std::make_unique<AudioParameterFloat>(
        PluginParameters::EQ_HIGH_FREQ, PluginParameters::EQ_HIGH_FREQ, PluginParameters::EQ_HIGH_FREQ_RANGE.getStart(), PluginParameters::EQ_HIGH_FREQ_RANGE.getEnd(), 2000.f));
    
    layout.add(std::make_unique<AudioParameterBool>(
        PluginParameters::CHORUS_ENABLED, PluginParameters::CHORUS_ENABLED, false));
    layout.add(std::make_unique<AudioParameterFloat>(
        PluginParameters::CHORUS_RATE, PluginParameters::CHORUS_RATE, PluginParameters::CHORUS_RATE_RANGE.getStart(), PluginParameters::CHORUS_RATE_RANGE.getEnd(), 1.f));
    layout.add(std::make_unique<AudioParameterFloat>(
        PluginParameters::CHORUS_DEPTH, PluginParameters::CHORUS_DEPTH, PluginParameters::CHORUS_DEPTH_RANGE.getStart(), PluginParameters::CHORUS_DEPTH_RANGE.getEnd(), 0.25f));
    layout.add(std::make_unique<AudioParameterFloat>(
        PluginParameters::CHORUS_FEEDBACK, PluginParameters::CHORUS_FEEDBACK, PluginParameters::CHORUS_FEEDBACK_RANGE.getStart(), PluginParameters::CHORUS_FEEDBACK_RANGE.getEnd(), 0.f));
    layout.add(std::make_unique<AudioParameterFloat>(
        PluginParameters::CHORUS_CENTER_DELAY, PluginParameters::CHORUS_CENTER_DELAY, PluginParameters::CHORUS_CENTER_DELAY_RANGE.getStart(), PluginParameters::CHORUS_CENTER_DELAY_RANGE.getEnd(), 7.f));
    layout.add(std::make_unique<AudioParameterFloat>(
        PluginParameters::CHORUS_MIX, PluginParameters::CHORUS_MIX, PluginParameters::CHORUS_MIX_RANGE.getStart(), PluginParameters::CHORUS_MIX_RANGE.getEnd(), 0.5f));
    return layout;
}

bool JustaSampleAudioProcessor::canLoadFileExtension(const String& filePath)
{
    return fileFilter.isFileSuitable(filePath);
}

void JustaSampleAudioProcessor::loadFileAndReset(const String& path)
{
    resetParameters = true;
    if (loadFile(path))
    {
        apvts.state.setProperty(PluginParameters::FILE_PATH, path, &undoManager);
        apvts.state.setProperty(PluginParameters::SAMPLE_START, 0, &undoManager);
        apvts.state.setProperty(PluginParameters::SAMPLE_END, sampleBuffer.getNumSamples() - 1, &undoManager);

        apvts.getParameterAsValue(PluginParameters::IS_LOOPING) = false;
        apvts.state.setProperty(PluginParameters::LOOPING_HAS_START, false, &undoManager);
        apvts.state.setProperty(PluginParameters::LOOPING_HAS_END, false, &undoManager);
        apvts.state.setProperty(PluginParameters::LOOP_START, 0, &undoManager);
        apvts.state.setProperty(PluginParameters::LOOP_END, sampleBuffer.getNumSamples() - 1, &undoManager);
    }
    resetParameters = false;
}

bool JustaSampleAudioProcessor::loadFile(const String& path)
{
    if (isPitchDetecting)
        return false;
    const auto file = File(path);
    AudioFormatReader* reader = formatManager.createReaderFor(file);
    if (reader != nullptr && path != samplePath)
    {
        formatReader = std::unique_ptr<AudioFormatReader>(reader);
        sampleBuffer.setSize(formatReader->numChannels, int(formatReader->lengthInSamples));
        formatReader->read(&sampleBuffer, 0, int(formatReader->lengthInSamples), 0, true, true);
        samplePath = path;
        updateSamplerSound(sampleBuffer);
        return true;
    }
    return false;
}

void JustaSampleAudioProcessor::resetSamplerVoices()
{
    samplerVoices.clear();
    synth.clearVoices();
    for (int i = 0; i < PluginParameters::NUM_VOICES; i++)
    {
        CustomSamplerVoice* samplerVoice = new CustomSamplerVoice(getTotalNumOutputChannels(), getBlockSize());
        synth.addVoice(samplerVoice);
        samplerVoices.add(samplerVoice);
    }
}

void JustaSampleAudioProcessor::haltVoices()
{
    for (int i = 0; i < synth.getNumVoices(); i++)
    {
        SynthesiserVoice* voice = synth.getVoice(i);
        voice->stopNote(1, false);
    }
}

void JustaSampleAudioProcessor::updateSamplerSound(AudioBuffer<float>& sample)
{
    resetSamplerVoices();
    synth.clearSounds();
    synth.addSound(new CustomSamplerSound(apvts, sample, int(formatReader->sampleRate)));
}

void JustaSampleAudioProcessor::valueTreePropertyChanged(ValueTree&, const Identifier& property)
{
    if (property.toString() == PluginParameters::FILE_PATH)
    {
        auto& path = apvts.state.getProperty(PluginParameters::FILE_PATH);
        if (samplePath != path.toString())
        {
            loadFile(path);
        }
    } 
    else if (property.toString() == PluginParameters::LOOPING_HAS_START)
    {
        if (apvts.state.getProperty(PluginParameters::LOOPING_HAS_START))
        {
            updateLoopStartPortionBounds();
        }
    }
    else if (property.toString() == PluginParameters::LOOPING_HAS_END)
    {
        if (apvts.state.getProperty(PluginParameters::LOOPING_HAS_END))
        {
            updateLoopEndPortionBounds();
        }
    }
}

void JustaSampleAudioProcessor::parameterChanged(const String& parameterID, float newValue)
{
    if (parameterID == PluginParameters::IS_LOOPING)
    {
        bool isLooping = newValue;
        if (isLooping)
        {
            // check bounds for the looping start and end sections
            if (apvts.state.getProperty(PluginParameters::LOOPING_HAS_START))
            {
                updateLoopStartPortionBounds();
            }
            if (apvts.state.getProperty(PluginParameters::LOOPING_HAS_END))
            {
                updateLoopEndPortionBounds();
            }
        }
    }
    else if (parameterID == PluginParameters::PLAYBACK_MODE)
    {
        auto mode = PluginParameters::getPlaybackMode(newValue);
        setProperLatency(mode);
    }
}

void JustaSampleAudioProcessor::updateLoopStartPortionBounds()
{
    Value viewStart = apvts.state.getPropertyAsValue(PluginParameters::UI_VIEW_START, apvts.undoManager);
    Value loopStart = apvts.state.getPropertyAsValue(PluginParameters::LOOP_START, apvts.undoManager);
    Value loopEnd = apvts.state.getPropertyAsValue(PluginParameters::LOOP_END, apvts.undoManager);
    Value sampleStart = apvts.state.getPropertyAsValue(PluginParameters::SAMPLE_START, apvts.undoManager);
    Value sampleEnd = apvts.state.getPropertyAsValue(PluginParameters::SAMPLE_END, apvts.undoManager);
    int newLoc = loopStart.getValue();
    if (newLoc >= int(sampleStart.getValue()))
    {
        newLoc = int(sampleStart.getValue()) - int(visibleSamples() * lookAndFeel.DEFAULT_LOOP_START_END_PORTION);
    }
    loopStart.setValue(jmax<int>(viewStart.getValue(), newLoc));
    // if the sample start was at the beginning of the view
    if (viewStart.getValue().equals(sampleStart.getValue()))
    {
        newLoc = int(sampleStart.getValue()) + int(visibleSamples() * lookAndFeel.DEFAULT_LOOP_START_END_PORTION);
        sampleStart = jmin<int>(newLoc, sampleEnd.getValue());
        if (viewStart.getValue().equals(sampleStart.getValue()))
        {
            sampleStart = int(sampleStart.getValue()) + 1;
            sampleEnd = int(sampleEnd.getValue()) + 1;
        }
        if (loopEnd.getValue() <= sampleEnd.getValue())
        {
            loopEnd = int(sampleEnd.getValue()) + 1;
        }
    }
}

void JustaSampleAudioProcessor::updateLoopEndPortionBounds()
{
    Value viewEnd = apvts.state.getPropertyAsValue(PluginParameters::UI_VIEW_END, apvts.undoManager);
    Value loopStart = apvts.state.getPropertyAsValue(PluginParameters::LOOP_START, apvts.undoManager);
    Value loopEnd = apvts.state.getPropertyAsValue(PluginParameters::LOOP_END, apvts.undoManager);
    Value sampleStart = apvts.state.getPropertyAsValue(PluginParameters::SAMPLE_START, apvts.undoManager);
    Value sampleEnd = apvts.state.getPropertyAsValue(PluginParameters::SAMPLE_END, apvts.undoManager);
    int newLoc = loopEnd.getValue();
    if (newLoc <= int(sampleEnd.getValue()))
    {
        newLoc = int(sampleEnd.getValue()) + int(visibleSamples() * lookAndFeel.DEFAULT_LOOP_START_END_PORTION);
    }
    loopEnd.setValue(jmin<int>(viewEnd.getValue(), newLoc));
    // if the sample end was at the end of the view
    if (viewEnd.getValue().equals(sampleEnd.getValue()))
    {
        newLoc = int(sampleEnd.getValue()) - int(visibleSamples() * lookAndFeel.DEFAULT_LOOP_START_END_PORTION);
        sampleEnd = jmax<int>(newLoc, sampleStart.getValue());
        if (viewEnd.getValue().equals(sampleEnd.getValue()))
        {
            sampleEnd = int(sampleEnd.getValue()) - 1;
            sampleStart = int(sampleStart.getValue()) - 1;
        }
        if (loopStart.getValue() >= sampleStart.getValue())
        {
            loopStart = int(loopStart.getValue()) - 1;
        }
    }
}

int JustaSampleAudioProcessor::visibleSamples() const
{
    return int(p(PluginParameters::UI_VIEW_END)) - int(p(PluginParameters::UI_VIEW_START));
}

void JustaSampleAudioProcessor::setProperLatency()
{
    setProperLatency(PluginParameters::getPlaybackMode(apvts.getParameterAsValue(PluginParameters::PLAYBACK_MODE).getValue()));
}

void JustaSampleAudioProcessor::setProperLatency(PluginParameters::PLAYBACK_MODES mode)
{
    if (mode == PluginParameters::BASIC)
    {
        setLatencySamples(0);
    }
    else if (apvts.getParameterAsValue(PluginParameters::IS_LOOPING).getValue() && p(PluginParameters::LOOPING_HAS_END))
    {
        setLatencySamples(2 * BufferPitcher::EXPECTED_PADDING);
    }
    else
    {
        setLatencySamples(BufferPitcher::EXPECTED_PADDING);
    }
}

bool JustaSampleAudioProcessor::pitchDetectionRoutine()
{
    if (!sampleBuffer.getNumSamples())
        return false;
    int effectiveStart = (apvts.getParameterAsValue(PluginParameters::IS_LOOPING).getValue() && p(PluginParameters::LOOPING_HAS_START)) ? p(PluginParameters::LOOP_START) : p(PluginParameters::SAMPLE_START);
    int effectiveEnd = (apvts.getParameterAsValue(PluginParameters::IS_LOOPING).getValue() && p(PluginParameters::LOOPING_HAS_END)) ? p(PluginParameters::LOOP_END) : p(PluginParameters::SAMPLE_END);
    if (effectiveEnd - effectiveStart + 1 > 8)
    {
        isPitchDetecting = true;
        pitchDetector.setData(sampleBuffer, effectiveStart, effectiveEnd, int(formatReader->sampleRate));
        pitchDetector.startThread();
        return true;
    }
    else
    {
        return false;
    }
}

void JustaSampleAudioProcessor::exitSignalSent()
{
    float pitch = pitchDetector.getPitch();
    if (pitch != -1)
    {
        float tuningAmount = 12 * log2f(440 / pitch);
        if (tuningAmount < -12 || tuningAmount > 12)
        {
            tuningAmount = fmodf(tuningAmount, 12);
        }
        int semitones = int(tuningAmount);
        int cents = int(100 * (tuningAmount - semitones));
        apvts.getParameterAsValue(PluginParameters::SEMITONE_TUNING) = semitones;
        apvts.getParameterAsValue(PluginParameters::CENT_TUNING) = cents;
    }
    isPitchDetecting = false;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JustaSampleAudioProcessor();
}

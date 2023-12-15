/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginParameters.h"

//==============================================================================
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
    apvts.addParameterListener(PluginParameters::IS_LOOPING, this);
    apvts.addParameterListener(PluginParameters::LOOPING_HAS_START, this);
    apvts.addParameterListener(PluginParameters::LOOPING_HAS_END, this);
    apvts.state.addListener(this);
    formatManager.registerBasicFormats();
    fileFilter = WildcardFileFilter(formatManager.getWildcardForAllFormats(), {}, {});
}

JustaSampleAudioProcessor::~JustaSampleAudioProcessor()
{
    delete formatReader;
}

//==============================================================================
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

void JustaSampleAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String JustaSampleAudioProcessor::getProgramName (int index)
{
    return {};
}

void JustaSampleAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void JustaSampleAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
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

//==============================================================================
bool JustaSampleAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* JustaSampleAudioProcessor::createEditor()
{
    LookAndFeel::setDefaultLookAndFeel(&lookAndFeel);
    return new JustaSampleAudioProcessorEditor(*this);
}

//==============================================================================
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
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        PluginParameters::PLAYBACK_MODE, PluginParameters::PLAYBACK_MODE, PluginParameters::PLAYBACK_MODE_LABELS, 1));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        PluginParameters::IS_LOOPING, PluginParameters::IS_LOOPING, false));
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
        apvts.getParameterAsValue(PluginParameters::PLAYBACK_MODE) = PluginParameters::PLAYBACK_MODES::ADVANCED;
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
    const auto file = File(path);
    auto reader = formatManager.createReaderFor(file);
    if (reader != nullptr && path != samplePath)
    {
        delete formatReader;
        formatReader = reader;
        sampleBuffer.setSize(formatReader->numChannels, formatReader->lengthInSamples);
        formatReader->read(&sampleBuffer, 0, formatReader->lengthInSamples, 0, true, true);
        auto small = 0.000000001;
        for (auto ch = 0; ch < sampleBuffer.getNumChannels(); ch++)
        {
            for (auto i = 0; i < sampleBuffer.getNumSamples(); i++)
            {
                if (sampleBuffer.getSample(ch, i) == 0)
                {
                    sampleBuffer.setSample(ch, i, small);
                }
            }
        }
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
    for (int i = 0; i < NUM_VOICES; i++)
    {
        CustomSamplerVoice* samplerVoice = new CustomSamplerVoice(getSampleRate(), getTotalNumOutputChannels());
        synth.addVoice(samplerVoice);
        samplerVoices.add(samplerVoice);
    }
}

void JustaSampleAudioProcessor::updateSamplerSound(AudioBuffer<float>& sample)
{
    resetSamplerVoices();
    synth.clearSounds();
    synth.addSound(new CustomSamplerSound(apvts, sample, formatReader->sampleRate, BASE_FREQ));
}

void JustaSampleAudioProcessor::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property)
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
        newLoc = int(sampleStart.getValue()) - visibleSamples() * lookAndFeel.DEFAULT_LOOP_START_END_PORTION;
    }
    loopStart.setValue(jmax<int>(viewStart.getValue(), newLoc));
    // if the sample start was at the beginning of the view
    if (viewStart.getValue().equals(sampleStart.getValue()))
    {
        int newLoc = int(sampleStart.getValue()) + visibleSamples() * lookAndFeel.DEFAULT_LOOP_START_END_PORTION;
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
        newLoc = int(sampleEnd.getValue()) + visibleSamples() * lookAndFeel.DEFAULT_LOOP_START_END_PORTION;
    }
    loopEnd.setValue(jmin<int>(viewEnd.getValue(), newLoc));
    // if the sample end was at the end of the view
    if (viewEnd.getValue().equals(sampleEnd.getValue()))
    {
        int newLoc = int(sampleEnd.getValue()) - visibleSamples() * lookAndFeel.DEFAULT_LOOP_START_END_PORTION;
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

int JustaSampleAudioProcessor::visibleSamples()
{
    return int(p(PluginParameters::UI_VIEW_END)) - int(p(PluginParameters::UI_VIEW_START));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JustaSampleAudioProcessor();
}

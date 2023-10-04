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
    synth.clearVoices();
    for (int i = 0; i < NUM_VOICES; i++)
    {
        synth.addVoice(new CustomSamplerVoice(getSampleRate(), getTotalNumOutputChannels()));
    }
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
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void JustaSampleAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid())
    {
        apvts.replaceState(tree);
        updateProcessor();
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout JustaSampleAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    return layout;
}

bool JustaSampleAudioProcessor::canLoadFileExtension(const String& filePath)
{
    return fileFilter.isFileSuitable(filePath);
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
        samplePath = path;
        updateSynthSample(sampleBuffer);
        return true;
    }
    return false;
}

void JustaSampleAudioProcessor::updateSynthSample(AudioBuffer<float>& sample)
{
    synth.clearSounds();
    synth.addSound(new CustomSamplerSound(sample, formatReader->sampleRate, BASE_FREQ));
}

void JustaSampleAudioProcessor::updateProcessor()
{
    loadFile(apvts.state.getProperty(PluginParameters::FILE_PATH));
}

void JustaSampleAudioProcessor::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property)
{
    if (property.toString() == PluginParameters::FILE_PATH)
    {
        auto path = apvts.state.getProperty(PluginParameters::FILE_PATH);
        if (samplePath != path.toString())
        {
            loadFile(path);
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JustaSampleAudioProcessor();
}

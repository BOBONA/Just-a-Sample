/*
  ==============================================================================

    PluginProcessor.cpp
    Created: 5 Sep 2023
    Author:  binya

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginParameters.h"

#if JUCE_DEBUG
#include "Utilities/BufferUtils.h"
#endif

JustaSampleAudioProcessor::JustaSampleAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
    apvts(*this, &undoManager, "Parameters", PluginParameters::createParameterLayout()),
    fileFilter("", {}, {}),
    deviceRecorder(deviceManager)
#endif
{
    apvts.addParameterListener(PluginParameters::PLAYBACK_MODE, this);
    apvts.addParameterListener(PluginParameters::IS_LOOPING, this);
    apvts.addParameterListener(PluginParameters::LOOPING_HAS_START, this);
    apvts.addParameterListener(PluginParameters::LOOPING_HAS_END, this);

    deviceRecorder.addListener(this);
    pitchDetector.addListener(this);

    formatManager.registerBasicFormats();
    fileFilter = juce::WildcardFileFilter(formatManager.getWildcardForAllFormats(), {}, {});
}

JustaSampleAudioProcessor::~JustaSampleAudioProcessor()
{
    // Probably don't need to remove listeners, since the processor outlives its owned objects
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool JustaSampleAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
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

juce::AudioProcessorEditor* JustaSampleAudioProcessor::createEditor()
{
    juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel);
    return new JustaSampleAudioProcessorEditor(*this);
}

void JustaSampleAudioProcessor::prepareToPlay(double sampleRate, int)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
}

void JustaSampleAudioProcessor::releaseResources()
{
    // When playback stops, this is a place to clean up resources
}

void JustaSampleAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

#if JUCE_DEBUG
    for (int ch = 0; ch < buffer.getNumChannels(); ch++)
    {
        protectYourEars(buffer.getWritePointer(ch), buffer.getNumSamples());
    }
#endif
}

//==============================================================================
void JustaSampleAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // All state properties should be saved at this point
    spv(PluginParameters::State::WIDTH) = pluginState.width.load();
    spv(PluginParameters::State::HEIGHT) = pluginState.height.load();
    spv(PluginParameters::State::FILE_PATH) = pluginState.filePath.load();
    spv(PluginParameters::State::SAMPLE_HASH) = pluginState.sampleHash.load();
    spv(PluginParameters::State::USING_FILE_REFERENCE) = pluginState.usingFileReference.load();
    spv(PluginParameters::State::RECENT_FILES) = pluginState.recentFiles.load();
    if (const auto stateXml = deviceManager.createStateXml())
        spv(PluginParameters::State::SAVED_DEVICE_SETTINGS) = stateXml->toString();
    spv(PluginParameters::State::UI_VIEW_START) = pluginState.viewStart.load();
    spv(PluginParameters::State::UI_VIEW_END) = pluginState.viewEnd.load();
    spv(PluginParameters::State::SAMPLE_START) = pluginState.sampleStart.load();
    spv(PluginParameters::State::SAMPLE_END) = pluginState.sampleEnd.load();
    spv(PluginParameters::State::LOOP_START) = pluginState.loopStart.load();
    spv(PluginParameters::State::LOOP_END) = pluginState.loopEnd.load();

    // Then, write empty "header" information to the stream
    auto mos = std::make_unique<juce::MemoryOutputStream>(destData, true);
    mos->writeInt(0);  // apvts size
    mos->writeInt(0);  // sample size
    size_t initialSize = mos->getDataSize();

    // Write the APVTS
    apvts.state.writeToStream(*mos);
    size_t apvtsSize = mos->getDataSize() - initialSize;

    // If we're not using a file reference, write the sample buffer to the stream
    std::unique_ptr<juce::AudioFormatWriter> formatWriter{ nullptr };
    if (!pluginState.usingFileReference && sampleBuffer.getNumSamples())
    {
        juce::WavAudioFormat wavFormat;
        formatWriter = std::unique_ptr<juce::AudioFormatWriter>(wavFormat.createWriterFor(&*mos, bufferSampleRate, sampleBuffer.getNumChannels(), PluginParameters::STORED_BITRATE, {}, 0));
        formatWriter->writeFromAudioSampleBuffer(sampleBuffer, 0, sampleBuffer.getNumSamples());
    }
    size_t sampleSize = mos->getDataSize() - apvtsSize - initialSize;

    // Return to the beginning of the stream and write in the actual sizes for the header
    bool positionMoved = mos->setPosition(0);
    assert(positionMoved);  // this should never fail
    mos->writeInt(int(apvtsSize));
    mos->writeInt(int(sampleSize));

    // Format writer deletes the underlying stream, so we need to release the safe pointer
    if (formatWriter)
        mos.release();
}

void JustaSampleAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // First, read the header information which includes the sizes of the APVTS and sample buffer
    juce::MemoryInputStream mis(data, sizeInBytes, false);
    int apvtsSize = mis.readInt();
    int sampleSize = mis.readInt();

    if (sizeInBytes != apvtsSize + sampleSize + 8)
        return; // format issue

    // Read the APVTS
    juce::SubregionStream apvtsStream{ &mis, mis.getPosition(), apvtsSize, false };
    auto tree = juce::ValueTree::readFromStream(apvtsStream);
    if (tree.isValid())
    {
        apvts.replaceState(tree);

        // Load the plugin state struct
        pluginState.width = sp(PluginParameters::State::WIDTH);
        pluginState.height = sp(PluginParameters::State::HEIGHT);
        pluginState.filePath = sp(PluginParameters::State::FILE_PATH);
        pluginState.sampleHash = sp(PluginParameters::State::SAMPLE_HASH);
        pluginState.usingFileReference = sp(PluginParameters::State::USING_FILE_REFERENCE);
        pluginState.viewStart = sp(PluginParameters::State::UI_VIEW_START);
        pluginState.viewEnd = sp(PluginParameters::State::UI_VIEW_END);
        pluginState.sampleStart = sp(PluginParameters::State::SAMPLE_START);
        pluginState.sampleEnd = sp(PluginParameters::State::SAMPLE_END);
        pluginState.loopStart = sp(PluginParameters::State::LOOP_START);
        pluginState.loopEnd = sp(PluginParameters::State::LOOP_END);

        auto recentFiles{ sp(PluginParameters::State::RECENT_FILES) };
        juce::StringArray fileArray{};
        for (int i = recentFiles.size() - 1; i >= 0; i--)
            fileArray.add(recentFiles[i].toString());
        pluginState.recentFiles = fileArray;

        juce::XmlDocument deviceSettingsDocument{ sp(PluginParameters::State::SAVED_DEVICE_SETTINGS) };
        deviceManager.initialise(2, 0, &*deviceSettingsDocument.getDocumentElement(), true);

        // Either load the sample from the file reference or from the stream directly
        juce::String filePath = pluginState.filePath;
        if (pluginState.usingFileReference && filePath.isNotEmpty())
        {
            bool fileLoaded = loadSampleFromPath(filePath, false, pluginState.sampleHash);
            if (!fileLoaded)  // Either the file was not found, loaded incorrectly, or the hash was incorrect
            {
                openFileChooser("File was not found. Please locate " + juce::File(filePath).getFileName(),
                                juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this](const juce::FileChooser& chooser)
                    {
                        auto file = chooser.getResult();
                        loadSampleFromPath(file.getFullPathName(), false, pluginState.sampleHash, true);
                    });
            }
        }
        else
        {
            juce::WavAudioFormat wavFormat;
            auto sampleStream = std::make_unique<juce::SubregionStream>(&mis, mis.getPosition(), sampleSize, false);
            std::unique_ptr<juce::AudioFormatReader> wavFormatReader;
            wavFormatReader = std::unique_ptr<juce::AudioFormatReader>(
                wavFormat.createReaderFor(&*sampleStream, false));
            if (wavFormatReader)
            {
                juce::AudioBuffer<float> loadedSample{ int(wavFormatReader->numChannels), int(wavFormatReader->lengthInSamples) };
                wavFormatReader->read(&loadedSample, 0, int(wavFormatReader->lengthInSamples), 0, true, true);
                loadSample(loadedSample, wavFormatReader->sampleRate, false);

                sampleStream.release();  // This is necessary since the format reader will destroy the object automatically
            }
        }
    }
}

//==============================================================================
void JustaSampleAudioProcessor::loadSample(juce::AudioBuffer<float>& sample, int sampleRate, bool resetParameters)
{
    sampleBuffer = std::move(sample);
    bufferSampleRate = sampleRate;
    pluginState.sampleHash = getSampleHash(sampleBuffer);

    if (resetParameters)
    {
        pluginState.usingFileReference = sampleBufferNeedsReference();

        pluginState.sampleStart = 0;
        pluginState.sampleEnd = sampleBuffer.getNumSamples() - 1;

        pv(PluginParameters::IS_LOOPING) = false;
        pv(PluginParameters::LOOPING_HAS_START) = false;
        pv(PluginParameters::LOOPING_HAS_END) = false;
        pluginState.loopStart = 0;
        pluginState.loopEnd = sampleBuffer.getNumSamples() - 1;
    }

    synth.clearVoices();
    samplerVoices.clear();
    for (int i = 0; i < PluginParameters::NUM_VOICES; i++)
    {
        CustomSamplerVoice* samplerVoice = new CustomSamplerVoice(getBlockSize());
        synth.addVoice(samplerVoice);
        samplerVoices.add(samplerVoice);
    }

    synth.clearSounds();
    synth.addSound(new CustomSamplerSound(apvts, pluginState, sampleBuffer, int(bufferSampleRate)));
}

bool JustaSampleAudioProcessor::loadSampleFromPath(const juce::String& path, bool resetParameters, const juce::String& expectedHash, bool continueWithWrongHash)
{
    const juce::File file{ path };
    const std::unique_ptr<juce::AudioFormatReader> formatReader{ formatManager.createReaderFor(file) };

    if (!formatReader || !formatReader->lengthInSamples)
        return false;

    // Load the file and check the hash
    juce::AudioBuffer<float> newSample{ int(formatReader->numChannels), int(formatReader->lengthInSamples) };
    formatReader->read(&newSample, 0, int(formatReader->lengthInSamples), 0, true, true);
    juce::String sampleHash = getSampleHash(newSample);
    if (expectedHash.isNotEmpty() && sampleHash != expectedHash && !continueWithWrongHash)
        return false;

    loadSample(newSample, formatReader->sampleRate, resetParameters || (sampleHash != expectedHash && continueWithWrongHash));
    pluginState.filePath = path;
    return true;
}

//==============================================================================
bool JustaSampleAudioProcessor::canLoadFileExtension(const juce::String& filePath) const
{
    return fileFilter.isFileSuitable(filePath);
}

juce::String JustaSampleAudioProcessor::getSampleHash(const juce::AudioBuffer<float>& buffer) const
{
    juce::MemoryBlock memoryBlock;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        memoryBlock.append(buffer.getReadPointer(channel), buffer.getNumSamples() * sizeof(float));
    }

    juce::MD5 md5{ memoryBlock };
    return md5.toHexString();
}


bool JustaSampleAudioProcessor::sampleBufferNeedsReference(const juce::AudioBuffer<float>& buffer) const
{
    double totalFileBits = double(buffer.getNumSamples()) * buffer.getNumChannels() * PluginParameters::STORED_BITRATE;
    return totalFileBits > PluginParameters::MAX_FILE_SIZE;
}

//==============================================================================
void JustaSampleAudioProcessor::openFileChooser(const juce::String& message, int flags, const std::function<void(const juce::FileChooser&)>& callback, bool wavOnly)
{
    fileChooser = std::make_unique<juce::FileChooser>(message, juce::File::getSpecialLocation(juce::File::userDesktopDirectory), wavOnly ? "*.wav" : formatManager.getWildcardForAllFormats());
    juce::MessageManager::callAsync([this, flags, callback] { fileChooser->launchAsync(flags, callback); });
}

void JustaSampleAudioProcessor::haltVoices() const
{
    for (int i = 0; i < synth.getNumVoices(); i++)
    {
        juce::SynthesiserVoice* voice = synth.getVoice(i);
        voice->stopNote(1, false);
    }
}

void JustaSampleAudioProcessor::recordingFinished(juce::AudioBuffer<float> recordingBuffer, int recordingSampleRate)
{
    // If the recording is too large, we prompt to save it to a file, otherwise load it into the plugin simply
    if (sampleBufferNeedsReference(recordingBuffer))
    {
        openFileChooser("Recording too large for plugin state, save to a file",
                        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles, 
            [this, recordingBuffer, recordingSampleRate](const juce::FileChooser& chooser) -> void {
                juce::File file = chooser.getResult();
                std::unique_ptr<juce::FileOutputStream> stream = std::make_unique<juce::FileOutputStream>(file);
                if (file.hasWriteAccess() && stream->openedOk())
                {
                    stream->setPosition(0);
                    stream->truncate();

                    juce::WavAudioFormat wavFormat;
                    std::unique_ptr<juce::AudioFormatWriter> formatWriter{ wavFormat.createWriterFor(
                        &*stream, recordingSampleRate, recordingBuffer.getNumChannels(),
                        PluginParameters::STORED_BITRATE, {}, 0) };
                    formatWriter->writeFromAudioSampleBuffer(recordingBuffer, 0, recordingBuffer.getNumSamples());
                    stream.release();

                    loadSampleFromPath(file.getFullPathName(), true);
                }
            }, true);
    }
    else
    {
        loadSample(recordingBuffer, recordingSampleRate, true);
        pluginState.filePath = "";
    }
}

bool JustaSampleAudioProcessor::startPitchDetectionRoutine(int startSample, int endSample)
{
    if (!sampleBuffer.getNumSamples())
        return false;

    // Truth be told, this does not need to be in a separate thread, but the algorithm I was using before was much slower
    pitchDetector.setData(sampleBuffer, startSample, endSample, bufferSampleRate);
    pitchDetector.startThread();
    return true;
}

void JustaSampleAudioProcessor::exitSignalSent()
{
    double pitch = pitchDetector.getPitch();
    if (pitch > 0)
    {
        double tuningAmount = 12 * log2(440 / pitch);
        if (tuningAmount < -12 || tuningAmount > 12)
        {
            tuningAmount = fmod(tuningAmount, 12);
        }
        int semitones = int(tuningAmount);
        int cents = int(100 * (tuningAmount - semitones));
        apvts.getParameterAsValue(PluginParameters::SEMITONE_TUNING) = semitones;
        apvts.getParameterAsValue(PluginParameters::CENT_TUNING) = cents;
    }
}

//==============================================================================
void JustaSampleAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    // Theoretically, the parameters are in an invalid state in the time between the parameters changing and this callback running
    if (parameterID == PluginParameters::IS_LOOPING)
    {
        bool isLooping = newValue;
        if (isLooping)
        {
            if (p(PluginParameters::LOOPING_HAS_START))
                updateLoopStartBounds();
            if (p(PluginParameters::LOOPING_HAS_END))
                updateLoopEndBounds();
        }
    }
    else if (parameterID == PluginParameters::LOOPING_HAS_START)
    {
        if (newValue)
            updateLoopStartBounds();
    }
    else if (parameterID == PluginParameters::LOOPING_HAS_END)
    {
        if (newValue)
            updateLoopEndBounds();
    }
    else if (parameterID == PluginParameters::PLAYBACK_MODE)
    {
        auto mode = PluginParameters::getPlaybackMode(newValue);
    }
}

void JustaSampleAudioProcessor::updateLoopStartBounds()
{
    if (pluginState.loopStart < pluginState.viewStart || pluginState.loopStart >= pluginState.sampleStart)
        pluginState.loopStart = juce::jmax<int>(pluginState.viewStart, pluginState.sampleStart - visibleSamples() * lookAndFeel.DEFAULT_LOOP_START_END_PORTION);
}

void JustaSampleAudioProcessor::updateLoopEndBounds()
{
    if (pluginState.loopEnd > pluginState.viewEnd || pluginState.loopEnd <= pluginState.sampleEnd)
        pluginState.loopEnd = juce::jmin<int>(pluginState.viewEnd, pluginState.sampleEnd + visibleSamples() * lookAndFeel.DEFAULT_LOOP_START_END_PORTION);
}

int JustaSampleAudioProcessor::visibleSamples() const
{
    return pluginState.viewEnd.load() - pluginState.viewStart.load() + 1;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JustaSampleAudioProcessor();
}

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
#include "Sampler/CustomSynthesizer.h"

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
    samplerSound(apvts, pluginState, sampleBuffer, int(bufferSampleRate)),
    fileFilter("", {}, {}),
    deviceRecorder(deviceManager)
#endif
{
    deviceRecorder.addListener(this);
    pitchDetector.addListener(this);

    formatManager.registerBasicFormats();
    fileFilter = juce::WildcardFileFilter(formatManager.getWildcardForAllFormats(), {}, {});

    mtsClient = MTS_RegisterClient();
}

JustaSampleAudioProcessor::~JustaSampleAudioProcessor()
{
    for (int i = synth.getNumVoices() - 1; i >= 0; i--)
        synth.removeVoiceWithoutDeleting(i);

    MTS_DeregisterClient(mtsClient);
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

juce::VST3ClientExtensions* JustaSampleAudioProcessor::getVST3ClientExtensions()
{
    return &reaperExtensions;
}

void JustaSampleAudioProcessor::prepareToPlay(double sampleRate, int /*maximumExpectedSamplesPerBlock*/)
{
    juce::ScopedLock lock(voiceLock);

    synth.clearSounds();
    synth.addSound(new BlankSynthesizerSound());

    for (int i = synth.getNumVoices() - 1; i >= 0; i--)
        synth.removeVoiceWithoutDeleting(i);
    samplerVoices.clear();

    synth.setCurrentPlaybackSampleRate(sampleRate);

    for (int i = 0; i < PluginParameters::MAX_VOICES; i++)
    {
        const bool initializeSample = samplerSound.sampleRate > 0 && samplerSound.sample.getNumSamples() > 0;
        auto* voice = new CustomSamplerVoice(samplerSound, mtsClient, sampleRate, getBlockSize(), initializeSample);
        samplerVoices.add(voice);
    }
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

    // Handle VST3 Reaper extensions functionality
    if (wrapperType == wrapperType_VST3)
    {
        auto file = reaperExtensions.getNamedConfigParam(REAPER_FILE_PATH);
        juce::File filePath{ file };
        auto currentFile = lastLoadAttempt.isNotEmpty() ? lastLoadAttempt : juce::String(pluginState.filePath);
        juce::File currentFilePath{ currentFile };
        if (file.isNotEmpty() && filePath.getFileIdentifier() != currentFilePath.getFileIdentifier())
        {
            loadedFromReaper = true;
            loadSampleFromPath(file, true, "", false, [&](bool loaded) -> void
                {
                    if (!loaded)
                        loadedFromReaper = false;
                });
        }
    }

    if (sampleBuffer.getNumSamples() == 0 || sampleBuffer.getNumChannels() == 0)
        return;

    juce::ScopedTryLock lock(voiceLock);

    if (lock.isLocked())
    {
        adjustVoiceCount();

        synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

#if JUCE_DEBUG
        for (int ch = 0; ch < buffer.getNumChannels(); ch++)
        {
            protectYourEars(buffer.getWritePointer(ch), buffer.getNumSamples());
        }
#endif
    }
}

void JustaSampleAudioProcessor::adjustVoiceCount(int count)
{
    int numVoices = juce::jmax<int>(1, p(PluginParameters::NUM_VOICES));
    if (count >= 0)
        numVoices = count;
    int currentVoices = synth.getNumVoices();

    // We never need to instantiate new voices
    if (currentVoices > numVoices)
    {
        for (int i = currentVoices - 1; i >= numVoices; --i)
        {
            synth.removeVoiceWithoutDeleting(i);
            samplerVoices[i]->immediateHalt();
        }
    }
  
    if (currentVoices < numVoices)
    {
        for (int i = currentVoices; i < numVoices; ++i)
        {
            synth.addVoice(samplerVoices[i]);
        }
    }
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
    spv(PluginParameters::State::PIN_VIEW) = pluginState.pinView.load();
    spv(PluginParameters::State::PRIMARY_CHANNEL) = pluginState.primaryChannel.load();
    spv(PluginParameters::State::SAMPLE_START) = pluginState.sampleStart.load();
    spv(PluginParameters::State::SAMPLE_END) = pluginState.sampleEnd.load();
    spv(PluginParameters::State::LOOP_START) = pluginState.loopStart.load();
    spv(PluginParameters::State::LOOP_END) = pluginState.loopEnd.load();
    spv(PluginParameters::State::SHOW_FX) = pluginState.showFX.load();
    spv(PluginParameters::State::DARK_MODE) = pluginState.darkMode.load();

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
    mos->setPosition(0);
    assert(mos->getPosition() == 0);  // This should never fail
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
    size_t apvtsSize = mis.readInt();
    size_t sampleSize = mis.readInt();

    if (size_t(sizeInBytes) != apvtsSize + sampleSize + 8)
        return; // format issue

    // Read the APVTS
    juce::SubregionStream apvtsStream{ &mis, mis.getPosition(), juce::int64(apvtsSize), false };
    auto tree = juce::ValueTree::readFromStream(apvtsStream);
    if (tree.isValid())
    {
        apvts.replaceState(tree);

        // Load the plugin state struct
        pluginState.width = sp(PluginParameters::State::WIDTH);
        pluginState.height = sp(PluginParameters::State::HEIGHT);
        pluginState.filePath = sp(PluginParameters::State::FILE_PATH);
        pluginState.usingFileReference = sp(PluginParameters::State::USING_FILE_REFERENCE);
        pluginState.pinView = sp(PluginParameters::State::PIN_VIEW);
        pluginState.primaryChannel = sp(PluginParameters::State::PRIMARY_CHANNEL);
        pluginState.showFX = sp(PluginParameters::State::SHOW_FX);
        pluginState.darkMode = sp(PluginParameters::State::DARK_MODE);

        // We'd rather wait to update certain fields until the sample is actually loaded. This is usually irrelevant, but if the DAW
        // allows undo and redo then a previous sample will likely be loaded while the new one is loading, so the new info will not make sense.
        auto sampleHash = sp(PluginParameters::State::SAMPLE_HASH);
        auto updateFileInfo = [this, sampleHash,
            viewStart = sp(PluginParameters::State::UI_VIEW_START), viewEnd = sp(PluginParameters::State::UI_VIEW_END),
            sampleStart = sp(PluginParameters::State::SAMPLE_START), sampleEnd = sp(PluginParameters::State::SAMPLE_END),
            loopStart = sp(PluginParameters::State::LOOP_START), loopEnd = sp(PluginParameters::State::LOOP_END)]
            {
                pluginState.sampleHash = sampleHash;
                pluginState.viewStart = viewStart;
                pluginState.viewEnd = viewEnd;
                pluginState.sampleStart = sampleStart;
                pluginState.sampleEnd = sampleEnd;
                pluginState.loopStart = loopStart;
                pluginState.loopEnd = loopEnd;
            };

        bool newFile = pluginState.sampleHash != sp(PluginParameters::State::SAMPLE_HASH).toString();
        if (!newFile)
            updateFileInfo();

        auto recentFiles{ sp(PluginParameters::State::RECENT_FILES) };
        juce::StringArray fileArray{};
        for (int i = recentFiles.size() - 1; i >= 0; i--)
            fileArray.add(recentFiles[i].toString());
        pluginState.recentFiles = fileArray;

        juce::XmlDocument deviceSettingsDocument{ sp(PluginParameters::State::SAVED_DEVICE_SETTINGS) };
        deviceManagerLoadedState = deviceSettingsDocument.getDocumentElement();
        auto currentState = deviceManager.createStateXml();
        deviceManagerLoaded = currentState && currentState->isEquivalentTo(&*deviceManagerLoadedState, true);

        // Either load the sample from the file reference or from the stream directly
        juce::String filePath = pluginState.filePath;
        if (pluginState.usingFileReference && filePath.isNotEmpty() && newFile)
        {
            loadSampleFromPath(filePath, false, sampleHash, false, [this, filePath, updateFileInfo, sampleHash](bool fileLoaded) -> void
                {
                    if (!fileLoaded)  // Either the file was not found, loaded incorrectly, or the hash was incorrect
                    {
                        openFileChooser("File was not found. Please locate " + juce::File(filePath).getFileName(),
                            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this, updateFileInfo, sampleHash](const juce::FileChooser& chooser)
                            {
                                auto file = chooser.getResult();
                                loadSampleFromPath(file.getFullPathName(), false, sampleHash, true, 
                                    [updateFileInfo](bool loaded) { if (loaded) updateFileInfo(); });
                            });
                    }
                    else updateFileInfo();
                });
        }
        else if (sampleSize && newFile)
        {
            // A copy must be made to allow reading in another thread, outside the lifetime of this function
            auto sampleData = std::make_shared<juce::MemoryBlock>(sampleSize);
            mis.read(sampleData->getData(), int(sampleSize));
            auto wavStream = new juce::MemoryInputStream(*sampleData, false);

            juce::WavAudioFormat wavFormat;
            std::unique_ptr<juce::AudioFormatReader> wavFormatReader{ wavFormat.createReaderFor(wavStream, true) };
            if (wavFormatReader)
            {
                lastLoadAttempt = "";
                reaperExtensions.setNamedConfigParam(REAPER_FILE_PATH, "");
                sampleLoader.loadSample(std::move(wavFormatReader), [this, sampleData /* necessary capture */, updateFileInfo, sampleHash]
                (const std::unique_ptr<juce::AudioBuffer<float>>& loadedSample, const juce::String&, const std::unique_ptr<juce::AudioFormatReader>& reader) -> void
                    {
                        if (!reader)
                            return;

                        loadSample(*loadedSample, int(reader->sampleRate), false, sampleHash);

                        updateFileInfo();
                    });
            }
        }
    }
}

//==============================================================================
void JustaSampleAudioProcessor::loadSample(juce::AudioBuffer<float>& sample, int sampleRate, bool resetParameters, const juce::String& precomputedHash)
{
    juce::ScopedLock lock(voiceLock);

    haltVoices();

    sampleBuffer = std::move(sample);
    bufferSampleRate = float(sampleRate);

    if (precomputedHash.isNotEmpty())
        pluginState.sampleHash = precomputedHash;
    else
        pluginState.sampleHash = getSampleHash(sampleBuffer);

    if (resetParameters)
    {
        pluginState.usingFileReference = sampleBufferNeedsReference();

        pluginState.sampleStart = 0;
        pluginState.sampleEnd = sampleBuffer.getNumSamples() - 1;

        // The order here is actually important, because the start and end buttons can otherwise enable the main loop button
        pv(PluginParameters::LOOPING_HAS_START) = false;
        pv(PluginParameters::LOOPING_HAS_END) = false;
        pv(PluginParameters::IS_LOOPING) = false;

        pluginState.loopStart = 0;
        pluginState.loopEnd = sampleBuffer.getNumSamples() - 1;
    }

    samplerSound.sampleChanged(int(bufferSampleRate));
    for (const auto& voice : samplerVoices)
        voice->initializeSample();
}

void JustaSampleAudioProcessor::loadSampleFromPath(const juce::String& path, bool resetParameters, const juce::String& expectedHash, bool continueWithWrongHash, const std::function<void(bool)>& callback)
{
    const juce::File file{ path };
    std::unique_ptr<juce::AudioFormatReader> formatReader{ formatManager.createReaderFor(file) };

    if (!formatReader || !formatReader->lengthInSamples)
        return callback(false);

    lastLoadAttempt = path;
    reaperExtensions.setNamedConfigParam(REAPER_FILE_PATH, path);

    // Load the file and check the hash
    sampleLoader.loadSample(std::move(formatReader), [this, callback, path, expectedHash, continueWithWrongHash, resetParameters]
        (const std::unique_ptr<juce::AudioBuffer<float>>& loadedSample, const juce::String& sampleHash, const std::unique_ptr<juce::AudioFormatReader>& reader) -> void
        {
            if (expectedHash.isNotEmpty() && sampleHash != expectedHash && !continueWithWrongHash)
                return callback(false);

            pluginState.filePath = path;
            loadSample(*loadedSample, int(reader->sampleRate), resetParameters || (sampleHash != expectedHash && continueWithWrongHash), sampleHash);

            return callback(true);
        });
}

//==============================================================================
bool JustaSampleAudioProcessor::canLoadFileExtension(const juce::String& filePath) const
{
    return fileFilter.isFileSuitable(filePath);
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

void JustaSampleAudioProcessor::haltVoices()
{
    for (auto voice : samplerVoices)
    {
        voice->immediateHalt();
    }
}

void JustaSampleAudioProcessor::playVoice()
{
    synth.noteOn(0, 70, 1.0f);
}

void JustaSampleAudioProcessor::initializeDeviceManager()
{
    if (!deviceManagerLoaded)
    {
        deviceManager.initialise(2, 0, &*deviceManagerLoadedState, true);
        deviceManagerLoaded = true;
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
                auto stream = std::make_unique<juce::FileOutputStream>(file);
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
        pluginState.filePath = "";
        lastLoadAttempt = "";
        reaperExtensions.setNamedConfigParam(REAPER_FILE_PATH, "");
        loadSample(recordingBuffer, recordingSampleRate, true);
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
        float a4_hz = p(PluginParameters::A4_HZ);
        if (a4_hz <= 0)
            a4_hz = 440.0f;

        double tuningAmount = 12 * log2(a4_hz / pitch);
        if (tuningAmount < -12 || tuningAmount > 12)
            tuningAmount = fmod(tuningAmount, 12);

        int semitones = int(tuningAmount);
        int cents = int(100 * (tuningAmount - semitones));
        apvts.getParameterAsValue(PluginParameters::SEMITONE_TUNING) = semitones;
        apvts.getParameterAsValue(PluginParameters::CENT_TUNING) = cents;
    }
}

//==============================================================================
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

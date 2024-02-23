#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginParameters.h"

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
    apvts(*this, &undoManager, "Parameters", createParameterLayout()),
    fileFilter("", {}, {})
#endif
{
    apvts.addParameterListener(PluginParameters::PLAYBACK_MODE, this);
    apvts.addParameterListener(PluginParameters::IS_LOOPING, this);
    apvts.addParameterListener(PluginParameters::LOOPING_HAS_START, this);
    apvts.addParameterListener(PluginParameters::LOOPING_HAS_END, this);
    apvts.state.addListener(this);
    deviceManager.addAudioCallback(this);
    formatManager.registerBasicFormats();
    fileFilter = WildcardFileFilter(formatManager.getWildcardForAllFormats(), {}, {});
    setProperLatency();
    pitchDetector.addListener(this);
}

JustaSampleAudioProcessor::~JustaSampleAudioProcessor()
{
    deviceManager.removeAudioCallback(this);
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

void JustaSampleAudioProcessor::setCurrentProgram(int)
{
}

const juce::String JustaSampleAudioProcessor::getProgramName(int)
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

void JustaSampleAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

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

void JustaSampleAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    pv(PluginParameters::WIDTH) = editorWidth;
    pv(PluginParameters::HEIGHT) = editorHeight;
    pv(PluginParameters::FILE_SAMPLE_LENGTH) = sampleBuffer.getNumSamples();
    pv(PluginParameters::USING_FILE_REFERENCE) = usingFileReference;
    
    auto stateXml = deviceManager.createStateXml();
    if (stateXml)
        pv(PluginParameters::SAVED_DEVICE_SETTINGS) = stateXml->toString();

    auto mos = std::make_unique<MemoryOutputStream>(destData, true);
    mos->writeInt(0); // apvts size
    mos->writeInt(0); // sample size
    size_t initialSize = mos->getDataSize();

    apvts.state.writeToStream(*mos);
    size_t apvtsSize = mos->getDataSize() - initialSize;

    std::unique_ptr<AudioFormatWriter> formatWriter{ nullptr };
    if (!usingFileReference && sampleBuffer.getNumSamples() > 0)
    {
        WavAudioFormat wavFormat;
        formatWriter = std::unique_ptr<AudioFormatWriter>(wavFormat.createWriterFor(&*mos, bufferSampleRate, sampleBuffer.getNumChannels(), PluginParameters::STORED_BITRATE, {}, 0));
        formatWriter->writeFromAudioSampleBuffer(sampleBuffer, 0, sampleBuffer.getNumSamples());
    }
    size_t sampleSize = mos->getDataSize() - apvtsSize - initialSize;

    bool positionMoved = mos->setPosition(0);
    assert(positionMoved); // this should hopefully never fail
    mos->writeInt(int(apvtsSize));
    mos->writeInt(int(sampleSize));

    // Sadly, this is necessary because the formatWriter destroys it automatically and for some reason that's problematic
    if (formatWriter)
        mos.release();
}

void JustaSampleAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    MemoryInputStream mis(data, sizeInBytes, false);
    int apvtsSize = mis.readInt();
    int sampleSize = mis.readInt();

    if (sizeInBytes != apvtsSize + sampleSize + 8)
        return; // format issue

    SubregionStream apvtsStream{ &mis, mis.getPosition(), apvtsSize, false };
    auto tree = ValueTree::readFromStream(apvtsStream);
    if (tree.isValid())
    {
        apvts.replaceState(tree);
        usingFileReference = p(PluginParameters::USING_FILE_REFERENCE);
        XmlDocument deviceSettingsDocument{ p(PluginParameters::SAVED_DEVICE_SETTINGS) };
        deviceManager.initialise(2, 0, &*deviceSettingsDocument.getDocumentElement(), true);

        String filePath = p(PluginParameters::FILE_PATH);
        int expectedSampleLength = p(PluginParameters::FILE_SAMPLE_LENGTH);
        if (usingFileReference && filePath.isNotEmpty())
        {
            bool fileLoaded = loadFile(filePath, true, expectedSampleLength);
            if (!fileLoaded || sampleBuffer.getNumSamples() != expectedSampleLength)
            {
                openFileChooser("File was not found. Please locate " + File(filePath).getFileName(), 
                    FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this, filePath, expectedSampleLength](const FileChooser& chooser)
                    {
                        auto file = chooser.getResult();
                        formatReader = std::unique_ptr<AudioFormatReader>(formatManager.createReaderFor(file));
                        if (formatReader)
                        {
                            if (formatReader->lengthInSamples == expectedSampleLength)
                            {
                                loadFile(file.getFullPathName(), false);
                                pv(PluginParameters::FILE_PATH) = file.getFullPathName();
                            }
                            else
                            {
                                loadSampleAndReset(file.getFullPathName(), false, false);
                            }
                        }
                    });
            }
        }
        else
        {
            WavAudioFormat wavFormat;
            auto sampleStream = std::make_unique<SubregionStream>(&mis, mis.getPosition(), sampleSize, false);
            auto wavFormatReader = std::unique_ptr<AudioFormatReader>(wavFormat.createReaderFor(&*sampleStream, false));
            if (wavFormatReader)
            {
                sampleBuffer.setSize(wavFormatReader->numChannels, int(wavFormatReader->lengthInSamples));
                bufferSampleRate = wavFormatReader->sampleRate;
                wavFormatReader->read(&sampleBuffer, 0, int(wavFormatReader->lengthInSamples), 0, true, true);
                updateSamplerSound(sampleBuffer);
                sampleStream.release(); // since the format reader destroys it automatically
            }
        }
    }
}

void JustaSampleAudioProcessor::audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels, float* const*, int, int numSamples, const AudioIODeviceCallbackContext&)
{
    if (shouldRecord)
    {
        if (!isRecording)
        {
            isRecording = true;
            recordingBufferList.clear();
            recordingBufferQueue.emplace(RecordingBufferChange::CLEAR);
            recordingSize = 0;
        }
        if (isRecording && numInputChannels && numSamples)
        {
            const int accumulatingMax = recordingSampleRate / PluginParameters::FRAME_RATE;
            if (accumulatingRecordingSize > accumulatingMax)
            {
                flushAccumulatedBuffer();
            }
            else
            {
                accumulatingRecordingBuffer.setSize(
                    accumulatingRecordingSize == 0 ? numInputChannels : jmin<int>(numInputChannels, accumulatingRecordingBuffer.getNumChannels()), 
                    jmax<int>(accumulatingRecordingBuffer.getNumSamples(), accumulatingRecordingSize + numSamples), true);
                for (int i = 0; i < accumulatingRecordingBuffer.getNumChannels(); i++)
                    accumulatingRecordingBuffer.copyFrom(i, accumulatingRecordingSize, inputChannelData[i], numSamples);
                accumulatingRecordingSize += numSamples;
            }
        }
    }
    else if (isRecording)
    {
        recordingFinished();
    }
}

void JustaSampleAudioProcessor::audioDeviceAboutToStart(AudioIODevice* device)
{
    recordingSampleRate = int(device->getCurrentSampleRate());
}

void JustaSampleAudioProcessor::audioDeviceStopped()
{
    if (isRecording)
    {
        recordingFinished();
    }
}

void JustaSampleAudioProcessor::recordingFinished()
{
    if (recordingBufferList.empty())
        return;

    if (accumulatingRecordingSize > 0)
        flushAccumulatedBuffer();

    int numChannels = recordingBufferList[0]->getNumChannels();
    int size = 0;
    AudioBuffer<float> recordingBuffer{ numChannels, recordingSize };
    for (int i = 0; i < recordingBufferList.size(); i++)
    {
        numChannels = jmin<int>(numChannels, recordingBufferList[i]->getNumChannels());
        recordingBuffer.setSize(numChannels, recordingBuffer.getNumSamples()); // This will only potentially decrease the channel count
        for (int ch = 0; ch < numChannels; ch++)
            recordingBuffer.copyFrom(ch, size, *recordingBufferList[i], ch, 0, recordingBufferList[i]->getNumSamples());
        size += recordingBufferList[i]->getNumSamples();
    }

    isRecording = false;
    bufferSampleRate = recordingSampleRate;
    sampleBuffer = std::move(recordingBuffer);
    samplePath = "";
    loadSampleAndReset("", true);
}

void JustaSampleAudioProcessor::flushAccumulatedBuffer()
{
    int numChannels = accumulatingRecordingBuffer.getNumChannels();
    std::unique_ptr<AudioBuffer<float>> recordingBuffer = std::make_unique<AudioBuffer<float>>(numChannels, accumulatingRecordingSize);
    for (int i = 0; i < numChannels; i++)
        recordingBuffer->copyFrom(i, 0, accumulatingRecordingBuffer, i, 0, accumulatingRecordingSize);
    recordingBufferQueue.emplace(RecordingBufferChange::ADD, *recordingBuffer);
    recordingBufferList.emplace_back(std::move(recordingBuffer));
    recordingSize += accumulatingRecordingSize;
    accumulatingRecordingSize = 0;
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
        PluginParameters::CHORUS_RATE, PluginParameters::CHORUS_RATE, PluginParameters::CHORUS_RATE_RANGE, 1.f));
    layout.add(std::make_unique<AudioParameterFloat>(
        PluginParameters::CHORUS_DEPTH, PluginParameters::CHORUS_DEPTH, PluginParameters::CHORUS_DEPTH_RANGE, 0.25f));
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

bool JustaSampleAudioProcessor::loadSampleAndReset(const String& path, bool reloadSample, bool makeNewFormatReader, int tries)
{
    resetParameters = true;
    bool fileLoaded = reloadSample || loadFile(path, makeNewFormatReader);
    if (fileLoaded)
    {
        usingFileReference = sampleBufferNeedsReference();

        // This is some convoluted code to handle the logic of making the user choose a file for the sample
        // I'm writing it this way because this function is the natural place to handle this, however it's awkward because the file chooser callbacks this 
        if (usingFileReference && reloadSample && path.isEmpty())
        {
            bool tryLoad = true;
            if (tries < 2)
            {
                openFileChooser("Save the sample to a file, since it is too large to store in the plugin data",
                    FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles, [this, path, tries](const FileChooser& chooser) -> void {
                        File file = chooser.getResult();
                        std::unique_ptr<FileOutputStream> stream = std::make_unique<FileOutputStream>(file);
                        if (file.hasWriteAccess() && stream->openedOk())
                        {
                            // Write the sample to file
                            stream->setPosition(0);
                            stream->truncate();

                            WavAudioFormat wavFormat;
                            std::unique_ptr<AudioFormatWriter> formatWriter{ wavFormat.createWriterFor(
                                &*stream, bufferSampleRate, sampleBuffer.getNumChannels(),
                                PluginParameters::STORED_BITRATE, {}, 0) };
                            formatWriter->writeFromAudioSampleBuffer(sampleBuffer, 0, sampleBuffer.getNumSamples());
                            stream.release();

                            samplePath = file.getFullPathName();
                        }
                        loadSampleAndReset(samplePath, true, false, tries + 1);
                    }, true);
            }
            else
            {
                tryLoad = false; // We give up after 2 tries
            }
            
            if (path.isEmpty() && tryLoad)
            {
                resetParameters = false;
                return false;
            }
        }

        // Otherwise this is a pretty normal but useful procedure
        if (samplePath == pv(PluginParameters::FILE_PATH).toString())
            apvts.state.sendPropertyChangeMessage(PluginParameters::FILE_PATH);
        else if (reloadSample) // This is to notify the editor properly in non-file cases
            apvts.state.setPropertyExcludingListener(this, PluginParameters::FILE_PATH, samplePath, &undoManager);
        else
            pv(PluginParameters::FILE_PATH) = path;
        pv(PluginParameters::SAMPLE_START) = 0;
        pv(PluginParameters::SAMPLE_END) = sampleBuffer.getNumSamples() - 1;

        apvts.getParameterAsValue(PluginParameters::IS_LOOPING) = false;

        pv(PluginParameters::LOOPING_HAS_START) = false;
        pv(PluginParameters::LOOPING_HAS_END) = false;
        pv(PluginParameters::LOOP_START) = 0;
        pv(PluginParameters::LOOP_END) = sampleBuffer.getNumSamples() - 1;
    }
    resetParameters = false;
    return fileLoaded;
}

bool JustaSampleAudioProcessor::loadFile(const String& path, bool makeNewFormatReader, int expectedSampleLength)
{
    const auto file = File(path);
    if (makeNewFormatReader)
        formatReader = std::unique_ptr<AudioFormatReader>(formatManager.createReaderFor(file));
    if (!formatReader->lengthInSamples || (expectedSampleLength > 0 && formatReader->lengthInSamples != expectedSampleLength))
        return false;
    if (formatReader && path != samplePath)
    {
        sampleBuffer.setSize(formatReader->numChannels, int(formatReader->lengthInSamples));
        formatReader->read(&sampleBuffer, 0, int(formatReader->lengthInSamples), 0, true, true);
        bufferSampleRate = formatReader->sampleRate;
        samplePath = path;
        updateSamplerSound(sampleBuffer);
        return true;
    }
    return false;
}

bool JustaSampleAudioProcessor::sampleBufferNeedsReference() const
{
    double totalFileBits = double(sampleBuffer.getNumSamples()) * sampleBuffer.getNumChannels() * PluginParameters::STORED_BITRATE;
    return totalFileBits > PluginParameters::MAX_FILE_SIZE;
}

void JustaSampleAudioProcessor::openFileChooser(const String& message, int flags, std::function<void(const FileChooser&)> callback, bool wavOnly)
{
    fileChooser = std::make_unique<FileChooser>(message, File::getSpecialLocation(File::userDesktopDirectory), wavOnly ? "*.wav" : formatManager.getWildcardForAllFormats());
    MessageManager::callAsync([this, flags, callback]() { fileChooser->launchAsync(flags, callback); });
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
    synth.addSound(new CustomSamplerSound(apvts, sample, int(bufferSampleRate)));
}

void JustaSampleAudioProcessor::valueTreePropertyChanged(ValueTree&, const Identifier& property)
{
    if (property.toString() == PluginParameters::FILE_PATH)
    {
        auto& path = p(PluginParameters::FILE_PATH);
        if (samplePath != path.toString())
        {
            loadFile(path);
        }
    }
    else if (property.toString() == PluginParameters::LOOPING_HAS_START)
    {
        if (p(PluginParameters::LOOPING_HAS_START))
        {
            updateLoopStartPortionBounds();
        }
    }
    else if (property.toString() == PluginParameters::LOOPING_HAS_END)
    {
        if (p(PluginParameters::LOOPING_HAS_END))
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
            // Check bounds for the looping start and end sections
            if (p(PluginParameters::LOOPING_HAS_START))
            {
                updateLoopStartPortionBounds();
            }
            if (p(PluginParameters::LOOPING_HAS_END))
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
    Value viewStart = pv(PluginParameters::UI_VIEW_START);
    Value loopStart = pv(PluginParameters::LOOP_START);
    Value loopEnd = pv(PluginParameters::LOOP_END);
    Value sampleStart = pv(PluginParameters::SAMPLE_START);
    Value sampleEnd = pv(PluginParameters::SAMPLE_END);
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
    Value viewEnd = pv(PluginParameters::UI_VIEW_END);
    Value loopStart = pv(PluginParameters::LOOP_START);
    Value loopEnd = pv(PluginParameters::LOOP_END);
    Value sampleStart = pv(PluginParameters::SAMPLE_START);
    Value sampleEnd = pv(PluginParameters::SAMPLE_END);
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

bool JustaSampleAudioProcessor::pitchDetectionRoutine(int startSample, int endSample)
{
    if (!sampleBuffer.getNumSamples())
        return false;
   
    pitchDetector.setData(sampleBuffer, startSample, endSample, bufferSampleRate);
    const bool useThread = true;
    if (useThread)
    {
        pitchDetector.startThread();
    }
    else
    {
        pitchDetector.detectPitch();
        exitSignalSent();
    }
    return true; // success (currently there are no failure conditions)
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
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JustaSampleAudioProcessor();
}

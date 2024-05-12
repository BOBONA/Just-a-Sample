#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginParameters.h"

#include "Utilities/ProtectYourEars.h"

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
    apvts.state.addListener(this);

    deviceRecorder.addListener(this);
    pitchDetector.addListener(this);

    formatManager.registerBasicFormats();
    fileFilter = WildcardFileFilter(formatManager.getWildcardForAllFormats(), {}, {});

    setProperLatency();
}

JustaSampleAudioProcessor::~JustaSampleAudioProcessor()
{
    deviceRecorder.removeListener(this);
    pitchDetector.removeListener(this);
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
    LookAndFeel::setDefaultLookAndFeel(&lookAndFeel);
    return new JustaSampleAudioProcessorEditor(*this);
}

void JustaSampleAudioProcessor::prepareToPlay(double sampleRate, int)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
    setProperLatency();
    resetSamplerVoices();
}

void JustaSampleAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any spare memory, etc.
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

void JustaSampleAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    spv(PluginParameters::WIDTH) = editorWidth;
    spv(PluginParameters::HEIGHT) = editorHeight;
    spv(PluginParameters::FILE_SAMPLE_LENGTH) = sampleBuffer.getNumSamples();
    spv(PluginParameters::USING_FILE_REFERENCE) = usingFileReference;
    
    auto stateXml = deviceManager.createStateXml();
    if (stateXml)
        spv(PluginParameters::SAVED_DEVICE_SETTINGS) = stateXml->toString();

    auto mos = std::make_unique<MemoryOutputStream>(destData, true);
    mos->writeInt(0);  // apvts size
    mos->writeInt(0);  // sample size
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
    assert(positionMoved);  // this should never fail
    mos->writeInt(int(apvtsSize));
    mos->writeInt(int(sampleSize));

    // Format writer deletes the underlying stream, so we need to release the safe pointer
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
        usingFileReference = sp(PluginParameters::USING_FILE_REFERENCE);
        XmlDocument deviceSettingsDocument{ sp(PluginParameters::SAVED_DEVICE_SETTINGS) };
        deviceManager.initialise(2, 0, &*deviceSettingsDocument.getDocumentElement(), true);

        String filePath = sp(PluginParameters::FILE_PATH);
        int expectedSampleLength = sp(PluginParameters::FILE_SAMPLE_LENGTH);
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
                                spv(PluginParameters::FILE_PATH) = file.getFullPathName();
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

void JustaSampleAudioProcessor::setProperLatency()
{
    setProperLatency(PluginParameters::getPlaybackMode(apvts.getParameterAsValue(PluginParameters::PLAYBACK_MODE).getValue()));
}

void JustaSampleAudioProcessor::setProperLatency(PluginParameters::PLAYBACK_MODES mode)
{
    int latencySamples = 0;
    if (mode == PluginParameters::ADVANCED)
    {
        latencySamples += BufferPitcher::EXPECTED_PADDING;  // This is for the buffer pitcher padding
        if (apvts.getParameterAsValue(PluginParameters::IS_LOOPING).getValue() && sp(PluginParameters::LOOPING_HAS_END))
        {
            latencySamples += BufferPitcher::EXPECTED_PADDING;  // This is for the release buffer pitcher's padding
            if (PluginParameters::PREPROCESS_RELEASE_BUFFER)  // Currently set to false, since true was not working well
            {
                latencySamples += PluginParameters::CROSSFADE_SMOOTHING;  // This is the for preprocessing the release buffer's crossfade samples
            }
        }
    }

    setLatencySamples(latencySamples);
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
        if (samplePath == spv(PluginParameters::FILE_PATH).toString())
            apvts.state.sendPropertyChangeMessage(PluginParameters::FILE_PATH);
        else if (reloadSample) // This is to notify the editor properly in non-file cases
            apvts.state.setPropertyExcludingListener(this, PluginParameters::FILE_PATH, samplePath, &undoManager);
        else
            spv(PluginParameters::FILE_PATH) = path;
        spv(PluginParameters::SAMPLE_START) = 0;
        spv(PluginParameters::SAMPLE_END) = sampleBuffer.getNumSamples() - 1;

        apvts.getParameterAsValue(PluginParameters::IS_LOOPING) = false;

        spv(PluginParameters::LOOPING_HAS_START) = false;
        spv(PluginParameters::LOOPING_HAS_END) = false;
        spv(PluginParameters::LOOP_START) = 0;
        spv(PluginParameters::LOOP_END) = sampleBuffer.getNumSamples() - 1;
    }
    resetParameters = false;
    return fileLoaded;
}

bool JustaSampleAudioProcessor::loadFile(const String& path, bool makeNewFormatReader, int expectedSampleLength)
{
    const auto file = File(path);
    if (makeNewFormatReader)
        formatReader = std::unique_ptr<AudioFormatReader>(formatManager.createReaderFor(file));
    if (!formatReader || !formatReader->lengthInSamples ||                                      // File could not be loaded or is empty
        (expectedSampleLength > 0 && formatReader->lengthInSamples != expectedSampleLength) ||  // File does not match expected length
        path == samplePath)                                                                     // File is already loaded       
        return false;

    sampleBuffer.setSize(formatReader->numChannels, int(formatReader->lengthInSamples));
    formatReader->read(&sampleBuffer, 0, int(formatReader->lengthInSamples), 0, true, true);
    bufferSampleRate = formatReader->sampleRate;
    samplePath = path;
    updateSamplerSound(sampleBuffer);
    return true;
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
        auto& path = sp(PluginParameters::FILE_PATH);
        if (samplePath != path.toString())
        {
            loadFile(path);
        }
    }
    else if (property.toString() == PluginParameters::LOOPING_HAS_START)
    {
        if (sp(PluginParameters::LOOPING_HAS_START))
        {
            updateLoopStartPortionBounds();
        }
    }
    else if (property.toString() == PluginParameters::LOOPING_HAS_END)
    {
        if (sp(PluginParameters::LOOPING_HAS_END))
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
            if (sp(PluginParameters::LOOPING_HAS_START))
            {
                updateLoopStartPortionBounds();
            }
            if (sp(PluginParameters::LOOPING_HAS_END))
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
    Value viewStart = spv(PluginParameters::UI_VIEW_START);
    Value loopStart = spv(PluginParameters::LOOP_START);
    Value loopEnd = spv(PluginParameters::LOOP_END);
    Value sampleStart = spv(PluginParameters::SAMPLE_START);
    Value sampleEnd = spv(PluginParameters::SAMPLE_END);
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
    Value viewEnd = spv(PluginParameters::UI_VIEW_END);
    Value loopStart = spv(PluginParameters::LOOP_START);
    Value loopEnd = spv(PluginParameters::LOOP_END);
    Value sampleStart = spv(PluginParameters::SAMPLE_START);
    Value sampleEnd = spv(PluginParameters::SAMPLE_END);
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
    return int(sp(PluginParameters::UI_VIEW_END)) - int(sp(PluginParameters::UI_VIEW_START));
}

//==============================================================================
void JustaSampleAudioProcessor::recordingFinished(AudioBuffer<float> recordingBuffer, int recordingSampleRate)
{
    sampleBuffer = std::move(recordingBuffer);
    bufferSampleRate = recordingSampleRate;
    samplePath = "";
    loadSampleAndReset("", true);
}

//==============================================================================
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
    return true;  // success (currently there are no failure conditions)
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

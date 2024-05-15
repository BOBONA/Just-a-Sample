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
#include "Utilities/ProtectYourEars.h"
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
    // All apvts.state properties should be saved properly by this point
    auto stateXml = deviceManager.createStateXml();
    if (stateXml)
        spv(PluginParameters::SAVED_DEVICE_SETTINGS) = stateXml->toString();

    // Then, write empty "header" information to the stream
    auto mos = std::make_unique<MemoryOutputStream>(destData, true);
    mos->writeInt(0);  // apvts size
    mos->writeInt(0);  // sample size
    size_t initialSize = mos->getDataSize();

    // Write the APVTS
    apvts.state.writeToStream(*mos);
    size_t apvtsSize = mos->getDataSize() - initialSize;

    // If we're not using a file reference, write the sample buffer to the stream
    std::unique_ptr<AudioFormatWriter> formatWriter{ nullptr };
    if (!sp(PluginParameters::USING_FILE_REFERENCE) && sampleBuffer.getNumSamples())
    {
        WavAudioFormat wavFormat;
        formatWriter = std::unique_ptr<AudioFormatWriter>(wavFormat.createWriterFor(&*mos, bufferSampleRate, sampleBuffer.getNumChannels(), PluginParameters::STORED_BITRATE, {}, 0));
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
    MemoryInputStream mis(data, sizeInBytes, false);
    int apvtsSize = mis.readInt();
    int sampleSize = mis.readInt();

    if (sizeInBytes != apvtsSize + sampleSize + 8)
        return; // format issue

    // Read the APVTS
    SubregionStream apvtsStream{ &mis, mis.getPosition(), apvtsSize, false };
    auto tree = ValueTree::readFromStream(apvtsStream);
    if (tree.isValid())
    {
        apvts.replaceState(tree);

        XmlDocument deviceSettingsDocument{ sp(PluginParameters::SAVED_DEVICE_SETTINGS) };
        deviceManager.initialise(2, 0, &*deviceSettingsDocument.getDocumentElement(), true);

        // Either load the sample from the file reference or from the stream directly
        String filePath = sp(PluginParameters::FILE_PATH);
        if (sp(PluginParameters::USING_FILE_REFERENCE) && filePath.isNotEmpty())
        {
            bool fileLoaded = loadSampleFromPath(filePath, false, sp(PluginParameters::SAMPLE_HASH));
            if (!fileLoaded)  // Either the file was not found, loaded incorrectly, or the hash was incorrect
            {
                openFileChooser("File was not found. Please locate " + File(filePath).getFileName(), 
                    FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this, filePath](const FileChooser& chooser)
                    {
                        auto file = chooser.getResult();
                        loadSampleFromPath(file.getFullPathName(), false, sp(PluginParameters::SAMPLE_HASH), true);
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
                juce::AudioBuffer<float> loadedSample{ int(wavFormatReader->numChannels), int(wavFormatReader->lengthInSamples) };
                wavFormatReader->read(&loadedSample, 0, int(wavFormatReader->lengthInSamples), 0, true, true);
                loadSample(loadedSample, wavFormatReader->sampleRate, false);

                sampleStream.release();  // This is necessary since the format reader will destroy the object automatically
            }
        }
    }
}

void JustaSampleAudioProcessor::loadSample(juce::AudioBuffer<float>& sample, int sampleRate, bool resetParameters)
{
    sampleBuffer = std::move(sample);
    bufferSampleRate = sampleRate;
    spv(PluginParameters::SAMPLE_HASH) = getSampleHash(sampleBuffer);

    if (resetParameters)
    {
        spv(PluginParameters::USING_FILE_REFERENCE) = sampleBufferNeedsReference();

        spv(PluginParameters::SAMPLE_START) = 0;
        spv(PluginParameters::SAMPLE_END) = sampleBuffer.getNumSamples() - 1;

        pv(PluginParameters::IS_LOOPING) = false;
        spv(PluginParameters::LOOPING_HAS_START) = false;
        spv(PluginParameters::LOOPING_HAS_END) = false;
        spv(PluginParameters::LOOP_START) = 0;
        spv(PluginParameters::LOOP_END) = sampleBuffer.getNumSamples() - 1;
    }

    resetSamplerVoices();
    synth.clearSounds();
    synth.addSound(new CustomSamplerSound(apvts, sampleBuffer, int(bufferSampleRate)));
}

bool JustaSampleAudioProcessor::loadSampleFromPath(const juce::String& path, bool resetParameters, const juce::String& expectedHash, bool continueWithWrongHash)
{
    const juce::File file{ path };
    const std::unique_ptr<AudioFormatReader> formatReader{ formatManager.createReaderFor(file) };

    if (!formatReader || !formatReader->lengthInSamples)
        return false;

    // Load the file and check the hash
    juce::AudioBuffer<float> newSample{ int(formatReader->numChannels), int(formatReader->lengthInSamples) };
    formatReader->read(&newSample, 0, int(formatReader->lengthInSamples), 0, true, true);
    juce::String sampleHash = getSampleHash(newSample);
    if (expectedHash.isNotEmpty() && sampleHash != expectedHash && !continueWithWrongHash)
        return false;

    loadSample(newSample, formatReader->sampleRate, resetParameters || (sampleHash != expectedHash && continueWithWrongHash));
    spv(PluginParameters::FILE_PATH) = path;
    return true;
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

bool JustaSampleAudioProcessor::canLoadFileExtension(const String& filePath)
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

void JustaSampleAudioProcessor::openFileChooser(const String& message, int flags, std::function<void(const FileChooser&)> callback, bool wavOnly)
{
    fileChooser = std::make_unique<FileChooser>(message, File::getSpecialLocation(File::userDesktopDirectory), wavOnly ? "*.wav" : formatManager.getWildcardForAllFormats());
    MessageManager::callAsync([this, flags, callback]() { fileChooser->launchAsync(flags, callback); });
}

void JustaSampleAudioProcessor::haltVoices()
{
    for (int i = 0; i < synth.getNumVoices(); i++)
    {
        SynthesiserVoice* voice = synth.getVoice(i);
        voice->stopNote(1, false);
    }
}

void JustaSampleAudioProcessor::valueTreePropertyChanged(ValueTree&, const Identifier& property)
{
    if (property.toString() == PluginParameters::LOOPING_HAS_START)
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
    // This is convenient but has the problem of values being incorrect until their callback runs
    // This is not necessarily a problem (though it causes a visual movement on the looping changes)
    // but is theoretically problematic, since I'd rather guarantee correctness at all times
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
    return int(sp(PluginParameters::UI_VIEW_END)) - int(sp(PluginParameters::UI_VIEW_START)) + 1;
}

//==============================================================================
void JustaSampleAudioProcessor::recordingFinished(juce::AudioBuffer<float> recordingBuffer, int recordingSampleRate)
{
    // If the recording is too large, we prompt to save it to a file, otherwise load it into the plugin simply
    if (sampleBufferNeedsReference(recordingBuffer))
    {
        openFileChooser("Recording too large for plugin state, save to a file",
            FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles, 
            [this, recordingBuffer, recordingSampleRate](const FileChooser& chooser) -> void {
                File file = chooser.getResult();
                std::unique_ptr<FileOutputStream> stream = std::make_unique<FileOutputStream>(file);
                if (file.hasWriteAccess() && stream->openedOk())
                {
                    stream->setPosition(0);
                    stream->truncate();

                    WavAudioFormat wavFormat;
                    std::unique_ptr<AudioFormatWriter> formatWriter{ wavFormat.createWriterFor(
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
        spv(PluginParameters::FILE_PATH) = "";
    }
}

//==============================================================================
bool JustaSampleAudioProcessor::startPitchDetectionRoutine(int startSample, int endSample)
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
        exitSignalSent();  // When not using a thread, we send the exit signal manually
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

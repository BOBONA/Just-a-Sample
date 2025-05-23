/*
  ==============================================================================

    DeviceRecorder.h
    Created: 11 May 2024 12:09:24pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "readerwriterqueue/readerwriterqueue.h"

/** A struct to represent a change in the recording buffer. This is used to update the UI with the latest recording data. */
struct RecordingBufferChange
{
    enum RecordingBufferChangeType
    {
        CLEAR,
        ADD
    };

    explicit RecordingBufferChange(RecordingBufferChangeType type, const juce::AudioBuffer<float>& buffer = {}) :
        type(type),
        addedBuffer(buffer)
    {
    }

    RecordingBufferChangeType type;
    juce::AudioBuffer<float> addedBuffer;
};

/** A listener interface for the DeviceRecorder class. */
class DeviceRecorderListener
{
public:
    DeviceRecorderListener() = default;
    virtual ~DeviceRecorderListener() = default;

    /** A callback once a recording has started. */
    virtual void recordingStarted() = 0;

    /** A callback once a recording has finished. 
        This will be called if the user or device manager stops the recording. 
        The recording buffer will be passed to the listener, along with the sample rate of the recording.
        This is called on the message thread.
    */
    virtual void recordingFinished(juce::AudioBuffer<float> recording, int recordingSampleRate) = 0;
};

using RecordingQueue = moodycamel::ReaderWriterQueue<RecordingBufferChange, 16384>;

/** This class provides a convenient encapsulation for device recording logic.
    Due to the reliance on callbacks from the AudioDeviceManager, a user of the class cannot directly start or stop
    the recording process.
*/
class DeviceRecorder final : public juce::AudioIODeviceCallback
{
public:
    explicit DeviceRecorder(juce::AudioDeviceManager& deviceManager) : deviceManager(deviceManager)
    {
        deviceManager.addAudioCallback(this);
    }

    ~DeviceRecorder() override
    {
        deviceManager.removeAudioCallback(this);
    }

    void addListener(DeviceRecorderListener* listener)
    {
        listeners.add(listener);
    }

    void removeListener(DeviceRecorderListener* listener)
    {
        listeners.remove(listener);
    }

    /** Start recording audio from the active device, set shouldRecordToQueue to false if you don't need the queue for UI updates */
    void startRecording(bool shouldRecordToQueue = true)
    {
        this->recordToQueue = shouldRecordToQueue;
        shouldRecord = true;
    }

    void stopRecording()
    {
        shouldRecord = false;
    }

    /** Check if the device is currently recording */
    bool isRecordingDevice() const
    {
        return isRecording;
    }

    /** This indicates whether the device is trying to record or not.
        Essentially, this can be controlled synchronously while isRecording is controlled asynchronously.
     */
    bool shouldRecordDevice() const
    {
        return shouldRecord;
    }

    /** Retrieve a queue of recording updates, which can be used to update the UI */
    RecordingQueue& getRecordingBufferQueue()
    {
        return recordingBufferQueue;
    }

private:
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels, float* const* /* outputChannelData */, int /* numOutputChannels */, int numSamples, const juce::AudioIODeviceCallbackContext&) override 
    {
        if (shouldRecord)
        {
            if (!isRecording)
            {
                isRecording = true;
                recordingBufferList.clear();
                if (recordToQueue)
                    recordingBufferQueue.emplace(RecordingBufferChange::CLEAR);
                recordingSize = 0;
                listeners.call(&DeviceRecorderListener::recordingStarted);
            }
            if (isRecording && numInputChannels && numSamples)
            {
                accumulatingRecordingBuffer.setSize(
                    accumulatingRecordingSize == 0 ? numInputChannels : juce::jmin<int>(numInputChannels, accumulatingRecordingBuffer.getNumChannels()),
                    juce::jmax<int>(accumulatingRecordingBuffer.getNumSamples(), accumulatingRecordingSize + numSamples), true);
                for (int i = 0; i < accumulatingRecordingBuffer.getNumChannels(); i++)
                    accumulatingRecordingBuffer.copyFrom(i, accumulatingRecordingSize, inputChannelData[i], numSamples);
                accumulatingRecordingSize += numSamples;

                const int accumulatingMax = recordingSampleRate / PluginParameters::FRAME_RATE;
                if (accumulatingRecordingSize > accumulatingMax)
                {
                    flushAccumulatedBuffer();
                }
            }
        }
        else if (isRecording)
        {
            recordingFinished();
        }
    }

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override 
    {
        recordingSampleRate = int(device->getCurrentSampleRate());
    }

    void audioDeviceStopped() override 
    {
        if (isRecording)
        {
            recordingFinished();
        }
    }

    void recordingFinished()
    {
        if (recordingBufferList.empty())
            return;

        if (accumulatingRecordingSize > 0)
            flushAccumulatedBuffer();

        int numChannels = recordingBufferList[0]->getNumChannels();
        int size = 0;
        juce::AudioBuffer<float> recordingBuffer{ numChannels, recordingSize };
        for (const auto& i : recordingBufferList)
        {
            numChannels = juce::jmin<int>(numChannels, i->getNumChannels());
            recordingBuffer.setSize(numChannels, recordingBuffer.getNumSamples()); // This will only potentially decrease the channel count
            for (int ch = 0; ch < numChannels; ch++)
                recordingBuffer.copyFrom(ch, size, *i, ch, 0, i->getNumSamples());
            size += i->getNumSamples();
        }

        isRecording = false;
        listeners.call(&DeviceRecorderListener::recordingFinished, std::move(recordingBuffer), recordingSampleRate);
    }

    /** Flush the accumulating recording buffer to the recording buffer list.
        We accumulate samples before pushing to the list so that the UI thread doesn't get
        outpaced.
    */
    void flushAccumulatedBuffer()
    {
        int numChannels = accumulatingRecordingBuffer.getNumChannels();
        auto recordingBuffer = std::make_unique<juce::AudioBuffer<float>>(numChannels, accumulatingRecordingSize);
        for (int i = 0; i < numChannels; i++)
            recordingBuffer->copyFrom(i, 0, accumulatingRecordingBuffer, i, 0, accumulatingRecordingSize);
        if (recordToQueue)
            recordingBufferQueue.emplace(RecordingBufferChange::ADD, *recordingBuffer);
        recordingBufferList.emplace_back(std::move(recordingBuffer));
        recordingSize += accumulatingRecordingSize;
        accumulatingRecordingSize = 0;
    }

    juce::LightweightListenerList<DeviceRecorderListener> listeners;
    juce::AudioDeviceManager& deviceManager;

    bool shouldRecord{ false };
    bool isRecording{ false };
    int recordingSampleRate{ 0 };
    int recordingSize{ 0 };

    juce::AudioBuffer<float> accumulatingRecordingBuffer;  // Necessary since the GUI thread cannot keep up unless the callbacks are accumulated
    int accumulatingRecordingSize{ 0 };
    std::vector<std::unique_ptr<juce::AudioBuffer<float>>> recordingBufferList;

    bool recordToQueue{ true };
    #pragma warning(disable: 4324)	// structure was padded due to __declspec(align())
    RecordingQueue recordingBufferQueue{ 10 }; 
};
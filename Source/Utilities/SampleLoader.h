/*
  ==============================================================================

    SampleLoader.h
    Created: 14 Jun 2024 5:37:01pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "BufferUtils.h"

/** A utility thread for asynchronously loading samples */
class LoaderThread final : public juce::Thread
{
public:
    LoaderThread(std::unique_ptr<juce::AudioFormatReader> formatReader, int threadID) : Thread("Loader_Thread_" + juce::String(threadID)),
        reader{std::move(formatReader)}
    {
    }

    ~LoaderThread() override
    {
        stopThread(-1);
    }

    void cancel()
    {
        canceled = true;
    }

    juce::AudioBuffer<float>* releaseSample() { return newSample.release(); }
    juce::AudioFormatReader* releaseReader() { return reader.release(); }
    const juce::String& getLoadedSampleHash() const { return sampleHash; }

private:
    void run() override
    {
        newSample = std::make_unique<juce::AudioBuffer<float>>(int(reader->numChannels), int(reader->lengthInSamples));
        reader->read(&*newSample, 0, int(reader->lengthInSamples), 0, true, true);
        sampleHash = getSampleHash(*newSample);

        if (canceled)
            newSample = nullptr;

        signalThreadShouldExit();
    }

    std::unique_ptr<juce::AudioFormatReader> reader;
    std::unique_ptr<juce::AudioBuffer<float>> newSample;
    juce::String sampleHash;
    bool canceled{ false };
};

/** Asynchronously loads a sample */
class SampleLoader final : public juce::Thread::Listener
{
public:
    void loadSample(std::unique_ptr<juce::AudioFormatReader> formatReader, 
        const std::function<void(const std::unique_ptr<juce::AudioBuffer<float>>& loadedSample, const juce::String& sampleHash, const std::unique_ptr<juce::AudioFormatReader>& reader)>& onCompletion)
    {
        loading = true;
        completionCallback = onCompletion;
        for (const auto& thread : threads)
        {
            thread->cancel();
            thread->removeListener(this);
        }
        
        auto newThread = std::make_unique<LoaderThread>(std::move(formatReader), ++ids);
        newThread->addListener(this);
        newThread->startThread();
        threads.emplace_back(std::move(newThread));
    }

    bool isLoading() const { return loading; }

private:
    void exitSignalSent() override
    {
        juce::MessageManager::callAsync([this]() -> void {
            auto& lastThread = threads[threads.size() - 1];
            completionCallback(std::unique_ptr<juce::AudioBuffer<float>>(lastThread->releaseSample()), lastThread->getLoadedSampleHash(), std::unique_ptr<juce::AudioFormatReader>(lastThread->releaseReader()));
            loading = false;
        });
    }

    //==============================================================================
    int ids{ 0 };
    std::vector<std::unique_ptr<LoaderThread>> threads;

    std::function<void(std::unique_ptr<juce::AudioBuffer<float>>, const juce::String&, std::unique_ptr<juce::AudioFormatReader>)> completionCallback;
    bool loading{ false };
};

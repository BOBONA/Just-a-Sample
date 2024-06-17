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
    LoaderThread(juce::AudioFormatReader* formatReader, int threadID) : Thread("Loader_Thread_" + juce::String(threadID)),
        reader{ formatReader }
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

    juce::AudioBuffer<float>* getLoadedSample() const { return newSample; }
    const juce::String& getLoadedSampleHash() const { return sampleHash; }

private:
    void run() override
    {
        newSample = new juce::AudioBuffer<float>{ int(reader->numChannels), int(reader->lengthInSamples) };
        reader->read(newSample, 0, int(reader->lengthInSamples), 0, true, true);
        sampleHash = getSampleHash(*newSample);

        if (canceled)
            delete newSample;

        signalThreadShouldExit();
    }

    juce::AudioFormatReader* reader{ nullptr };
    juce::AudioBuffer<float>* newSample{ nullptr };
    juce::String sampleHash;
    bool canceled{ false };
};

/** Asynchronously loads a sample */
class SampleLoader final : public juce::Thread::Listener
{
public:
    void loadSample(juce::AudioFormatReader* formatReader, 
        const std::function<void(const std::unique_ptr<juce::AudioBuffer<float>>& loadedSample, const juce::String& sampleHash)>& onCompletion)
    {
        loading = true;
        completionCallback = onCompletion;
        for (const auto& thread : threads)
        {
            thread->cancel();
            thread->removeListener(this);
        }

        auto newThread = std::make_unique<LoaderThread>(formatReader, ++ids);
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
            completionCallback(std::unique_ptr<juce::AudioBuffer<float>>(lastThread->getLoadedSample()), lastThread->getLoadedSampleHash());
        });
        loading = false;
    }

    //==============================================================================
    int ids{ 0 };
    std::vector<std::unique_ptr<LoaderThread>> threads;

    std::function<void(std::unique_ptr<juce::AudioBuffer<float>>, juce::String)> completionCallback;
    bool loading{ false };
};

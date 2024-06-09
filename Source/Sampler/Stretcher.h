/*
  ==============================================================================

    Stretcher.h
    Created: 27 May 2024 4:09:49pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <Bungee.h>

class BungeeStretcher
{
public:
    explicit BungeeStretcher(const juce::AudioBuffer<float>& sampleBuffer, int bufferSampleRate, int applicationSampleRate)
    {
        bungee = std::make_unique<Bungee::Stretcher<Bungee::Basic>>(Bungee::SampleRates{ bufferSampleRate, applicationSampleRate }, sampleBuffer.getNumChannels());
        inputData.setSize(1, sampleBuffer.getNumChannels() * bungee->maxInputFrameCount());
    }

    void initialize(const juce::AudioBuffer<float>& sampleBuffer, long double sampleStart, int bufferSampleRate, int applicationSampleRate, float initialRatio = 1, float initialSpeed = 1)
    {
        buffer = &sampleBuffer;
        pitchRatio = initialRatio;
        speedFactor = initialSpeed;

        if (initialRatio < 0.25f)
        {
            pitchRatio = 0.25f;
            resamplingHack = 0.25f / initialRatio;
        }
        else
        {
            resamplingHack = 1.f;
        }

        positionSpeed = speedFactor * bufferSampleRate / applicationSampleRate;

        bungee = std::make_unique<Bungee::Stretcher<Bungee::Basic>>(Bungee::SampleRates{ int(bufferSampleRate / resamplingHack), applicationSampleRate }, buffer->getNumChannels());
        output.data = nullptr;
        inputData.setSize(1, buffer->getNumChannels() * bungee->maxInputFrameCount());
        outputIndex = 0;

        preroll(sampleStart);
    }

    /** Pre-rolls the stretcher to a new position. Use this before you plan to move the position. */
    void preroll(float newPosition)
    {
        request = Bungee::Request{ newPosition, speedFactor * resamplingHack, pitchRatio, true };
        bungee->preroll(request);

        while (!output.data || std::isnan(output.request[Bungee::OutputChunk::begin]->position) || newPosition > output.request[Bungee::OutputChunk::end]->position)
        {
            auto input = bungee->specifyGrain(request);

            if (buffer->getNumChannels() * input.frameCount() >= inputData.getNumSamples())  // Can happen with extreme ratios
                inputData.setSize(1, buffer->getNumChannels() * input.frameCount());

            int begin = juce::jlimit<int>(newPosition, buffer->getNumSamples(), input.begin);
            int end = juce::jlimit<int>(newPosition, buffer->getNumSamples(), input.end);

            inputData.clear();
            if (begin < end)
            {
                for (int ch = 0; ch < buffer->getNumChannels(); ch++)
                    inputData.copyFrom(0, ch * input.frameCount() + begin - input.begin, *buffer, ch, begin, end - begin);
            }

            bungee->analyseGrain(inputData.getReadPointer(0), input.frameCount());
            bungee->synthesiseGrain(output);
            bungee->next(request);

            outputIndex = std::round((newPosition - output.request[Bungee::OutputChunk::begin]->position) / positionSpeed);
        }
    }

    /** Fetches the next sample. Call this for each channel before advancing. */
    float nextSample(int channel, bool advance = true)
    {
        if (outputIndex >= output.frameCount)
        {
            auto [begin, end] = bungee->specifyGrain(request);

            begin = juce::jlimit<int>(0, buffer->getNumSamples(), begin);
            end = juce::jlimit<int>(0, buffer->getNumSamples(), end);

            inputData.clear();  // Preferring simplicity over maximum efficiency
            for (int ch = 0; ch < buffer->getNumChannels(); ch++)
                inputData.copyFrom(0, ch * (end - begin), *buffer, ch, begin, end - begin);

            bungee->analyseGrain(inputData.getReadPointer(0), end - begin);
            bungee->synthesiseGrain(output);
            bungee->next(request);

            outputIndex = 0;
        }

        // DBG("" << outputIndex << "/" << output.frameCount);

        float result = output.data[channel * output.channelStride + outputIndex];
        if (advance)
            outputIndex++;

        return result;
    }

    /** Returns the speed at which the stretcher advances through the buffer, relative to the buffer's sample rate. */
    float getPositionSpeed() const { return positionSpeed; }

private:
    const juce::AudioBuffer<float>* buffer{ nullptr };
    float pitchRatio{ 1. };
    float speedFactor{ 1. };
    float positionSpeed{ 0. };  // speedFactor * bufferSampleRate / applicationSampleRate

    // 0.25 appears to be a limit, but this can be circumvented for some reason?
    float resamplingHack{ 1.f };

    std::unique_ptr<Bungee::Stretcher<Bungee::Basic>> bungee;
    Bungee::Request request{};
    Bungee::OutputChunk output{};

    juce::AudioBuffer<float> inputData;
    int outputIndex{ 0 };
};

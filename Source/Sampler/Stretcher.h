/*
  ==============================================================================

    Stretcher.h
    Created: 27 May 2024 4:09:49pm
    Author:  binya

  ==============================================================================
*/

#pragma once
#include <Bungee.h>

// Bungee sets a hard limit on the pitch ratio to simplify memory management. We can increase this limit before building
// and use a resampling hack when necessary (the hack is not great because it requires reallocation of the stretcher).
// This must be set to the value in Timing.cpp (internal to Bungee)
static constexpr int bungeeMaxPitchOctaves = 4;
static constexpr float bungeeMinimumRatio = 1.f / (1 << bungeeMaxPitchOctaves);

class BungeeStretcher
{
public:
    explicit BungeeStretcher(const juce::AudioBuffer<float>& sampleBuffer, int sampleRate) : buffer(&sampleBuffer),
        bufferSampleRate(sampleRate)
    {
    }

    /** Allocates a new stretcher and the input buffer. */
    void setSampleRate(int appSampleRate)
    {
        bungee = std::make_unique<Bungee::Stretcher<Bungee::Basic>>(Bungee::SampleRates{ bufferSampleRate, appSampleRate }, buffer->getNumChannels());
        inputData.setSize(1, buffer->getNumChannels() * bungee->maxInputFrameCount(), false, false, true);  // Note, maxInputFrameCount has a reported overflow issue
        previousInputRate = bufferSampleRate;
        applicationSampleRate = appSampleRate;
    }

    void initialize(long double sampleStart, float initialRatio = 1, float initialSpeed = 1)
    {
        setPitchAndSpeed(initialRatio, initialSpeed);

        // We only reallocate when necessary (if resamplingHack requires it, as it's not good for real-time performance)
        int inputRate = int(bufferSampleRate / resamplingHack);
        if (inputRate != previousInputRate)
        {
            bungee = std::make_unique<Bungee::Stretcher<Bungee::Basic>>(Bungee::SampleRates{ inputRate, applicationSampleRate }, buffer->getNumChannels());
            inputData.setSize(1, buffer->getNumChannels() * bungee->maxInputFrameCount(), false, false, true);  // maxInputFrameCount has a reported overflow issue
            previousInputRate = inputRate;
        }

        output = Bungee::OutputChunk{};
        outputIndex = 0;

        preroll(sampleStart);
    }

    /** Pre-rolls the stretcher to a new position. Use this before you plan to move the position. */
    void preroll(double newPosition)
    {
        request = Bungee::Request{ newPosition, speedFactor * resamplingHack, pitchRatio, true };
        bungee->preroll(request);

        while (!output.data || std::isnan(output.request[Bungee::OutputChunk::begin]->position) || 
            newPosition > output.request[Bungee::OutputChunk::end]->position || newPosition < output.request[Bungee::OutputChunk::begin]->position)
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
            request.pitch = pitchRatio;
            request.speed = speedFactor * resamplingHack;
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

        float result = output.data[channel * output.channelStride + outputIndex];
        if (advance)
            outputIndex++;

        return result;
    }

    void setPitchAndSpeed(float newPitchRatio, float newSpeedFactor)
    {
        pitchRatio = newPitchRatio;
        speedFactor = newSpeedFactor;

        if (newPitchRatio < bungeeMinimumRatio)
        {
            pitchRatio = bungeeMinimumRatio;
            resamplingHack = bungeeMinimumRatio / newPitchRatio;
        }
        else
        {
            resamplingHack = 1.f;
        }
        
        positionSpeed = speedFactor * bufferSampleRate / applicationSampleRate;
    }

    /** Returns the speed at which the stretcher advances through the buffer, relative to the buffer's sample rate. */
    float getPositionSpeed() const { return positionSpeed; }

private:
    const juce::AudioBuffer<float>* buffer{ nullptr };
    int bufferSampleRate{ 0 };
    int applicationSampleRate{ 0 };

    float pitchRatio{ 1. };
    float speedFactor{ 1. };
    float positionSpeed{ 0. };  // speedFactor * bufferSampleRate / applicationSampleRate

    // We use a little hack to ignore Bungee's maxPitchOctaves limit
    float resamplingHack{ 1.f };
    int previousInputRate{ 0 };

    std::unique_ptr<Bungee::Stretcher<Bungee::Basic>> bungee;
    Bungee::Request request{};
    Bungee::OutputChunk output{};

    juce::AudioBuffer<float> inputData;
    int outputIndex{ 0 };
};

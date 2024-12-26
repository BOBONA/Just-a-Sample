/*
  ==============================================================================

    BufferUtils.h
    Created: 19 May 2024 7:46:31pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

/** Given a source and destination buffer with different number of channels, mix channels appropriately to
    get a reasonable output. If mixMono is true, all channels are mixed together to mono.
*/
inline void mixToBuffer(const juce::AudioBuffer<float>& src, juce::AudioBuffer<float>& dest, int startSample, int numSamples, bool mixMono)
{
    if (mixMono)
    {
        // Mix all source channels to mono
        for (int ch = 0; ch < src.getNumChannels(); ch++)
        {
            for (int outputCh = 0; outputCh < dest.getNumChannels(); outputCh++)
            {
                dest.addFrom(outputCh, startSample, src.getReadPointer(ch), numSamples, 1.f / float(src.getNumChannels()));
            }
        }
    }
    else if (dest.getNumChannels() < src.getNumChannels())
    {
        // Down mix from more source channels to fewer destination channels
        float ratio = float(src.getNumChannels()) / dest.getNumChannels();
        for (int outputCh = 0; outputCh < dest.getNumChannels(); outputCh++)
        {
            int startCh = int(outputCh * ratio);
            int endCh = int((outputCh + 1) * ratio);

            for (int ch = startCh; ch < endCh; ch++)
            {
                dest.addFrom(outputCh, startSample, src.getReadPointer(ch), numSamples, 1.f / (endCh - startCh));
            }
        }
    }
    else if (dest.getNumChannels() >= src.getNumChannels())
    {
        // Up mix from fewer source channels to more destination channels
        float ratio = float(dest.getNumChannels()) / src.getNumChannels();
        for (int inputCh = 0; inputCh < src.getNumChannels(); inputCh++)
        {
            int startCh = int(inputCh * ratio);
            int endCh = int((inputCh + 1) * ratio);

            for (int outputCh = startCh; outputCh < endCh; outputCh++)
            {
                dest.addFrom(outputCh, startSample, src.getReadPointer(inputCh), numSamples);
            }
        }
    }
}

/**
  Silences the buffer if bad or loud values are detected in the output buffer.
  Use this during debugging to avoid blowing out your eardrums on headphones.
  If the output value is out of the range [-1, +1] it will be hard clipped.

  Credit to https://gist.github.com/hollance/b7219e49c6fc16fa2af05c4a72c186ef for this quick solution.
 */
inline void protectYourEars(float* buffer, int sampleCount)
{
    if (buffer == nullptr) { return; }
    bool firstWarning = true;
    for (int i = 0; i < sampleCount; ++i) 
    {
        float x = buffer[i];
        bool silence = false;
        if (std::isnan(x)) 
        {
            DBG("!!! WARNING: nan detected in audio buffer, silencing !!!");
            silence = true;
        }
        else if (std::isinf(x)) 
        {
            DBG("!!! WARNING: inf detected in audio buffer, silencing !!!");
            silence = true;
        }
        else if (x < -2.0f || x > 2.0f)  // screaming feedback
        {  
            DBG("!!! WARNING: sample out of range, silencing !!!");
            silence = true;
        }
        else if (x < -1.0f) 
        {
            if (firstWarning) 
            {
                DBG("!!! WARNING: sample out of range, clamping !!!");
                firstWarning = false;
            }
            buffer[i] = -1.0f;
        }
        else if (x > 1.0f) 
        {
            if (firstWarning) 
            {
                DBG("!!! WARNING: sample out of range, clamping !!!");
                firstWarning = false;
            }
            buffer[i] = 1.0f;
        }
        if (silence) 
        {
            memset(buffer, 0, sampleCount * sizeof(float));
            return;
        }
    }
}

/** Uses MD-5 hashing to generate an identifier for the AudioBuffer */
inline juce::String getSampleHash(const juce::AudioBuffer<float>& buffer)
{
    juce::MemoryBlock memoryBlock;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        memoryBlock.append(buffer.getReadPointer(channel), buffer.getNumSamples() * sizeof(float));
    }

    juce::MD5 md5{ memoryBlock };
    return md5.toHexString();
}
#pragma once

#include <JuceHeader.h>

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
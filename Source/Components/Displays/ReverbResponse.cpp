/*
  ==============================================================================

    ReverbResponse.cpp
    Created: 17 Jan 2024 11:16:38pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>
#include "ReverbResponse.h"

ReverbResponse::ReverbResponse(AudioProcessorValueTreeState& apvts, int sampleRate) : apvts(apvts), sampleRate(sampleRate), responseThread(apvts, sampleRate / 10) // less accurate but faster
{
    setBufferedToImage(true);
    apvts.addParameterListener(PluginParameters::REVERB_SIZE, this);
    apvts.addParameterListener(PluginParameters::REVERB_DAMPING, this);
    apvts.addParameterListener(PluginParameters::REVERB_LOWPASS, this);
    apvts.addParameterListener(PluginParameters::REVERB_HIGHPASS, this);
    apvts.addParameterListener(PluginParameters::REVERB_PREDELAY, this);
    apvts.addParameterListener(PluginParameters::REVERB_MIX, this);

    responseThread.startThread();

    startTimerHz(60);
}

ReverbResponse::~ReverbResponse()
{
    apvts.removeParameterListener(PluginParameters::REVERB_SIZE, this);
    apvts.removeParameterListener(PluginParameters::REVERB_DAMPING, this);
    apvts.removeParameterListener(PluginParameters::REVERB_LOWPASS, this);
    apvts.removeParameterListener(PluginParameters::REVERB_HIGHPASS, this);
    apvts.removeParameterListener(PluginParameters::REVERB_PREDELAY, this);
    apvts.removeParameterListener(PluginParameters::REVERB_MIX, this);
}

void ReverbResponse::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // draw the RMS values
    g.setColour(disabled(lnf.WAVEFORM_COLOR));
    for (int i = 0; i < bounds.getWidth(); i++)
    {
        float normalizedRMS = jmap<float>(rmsRecordings[int(float(i) * rmsRecordingsEffectiveSize / bounds.getWidth())], 0.f, 1.f, 0.f, float(getHeight()));
        g.drawVerticalLine(i, (getHeight() - normalizedRMS) / 2.f, (getHeight() + normalizedRMS) / 2.f);
    }
}

void ReverbResponse::resized()
{
    if (firstResponse)
    {
        firstResponse = false;
        responseThread.calculateRMS(getWidth());
    }
}

void ReverbResponse::enablementChanged()
{
    repaint();
}

void ReverbResponse::parameterChanged(const String& parameterID, float newValue)
{
    responseThread.calculateRMS(getWidth());
}

void ReverbResponse::timerCallback()
{
    // process response thread changes
    bool rmsRecordingsChanged{ false };
    ResponseChange* change{ nullptr };
    while ((change = responseThread.responseChangeQueue.peek()) != nullptr)
    {
        if (change->type == ResponseChange::VALUE)
        {
            if (rmsRecordings.size() <= change->index)
                rmsRecordingsEffectiveSize++;
            rmsRecordings.set(change->index, change->newValue);
        }
        else if (change->type == ResponseChange::DECREASE_SIZE)
        {
            rmsRecordingsEffectiveSize = change->index;
        }
        responseThread.responseChangeQueue.pop();
        rmsRecordingsChanged = true;
    }

    if (rmsRecordingsChanged)
    {
        repaint();
    }
}

ResponseThread::ResponseThread(AudioProcessorValueTreeState& apvts, int sampleRate) : Thread("Reverb_Response_Thread"), apvts(apvts), sampleRate(sampleRate)
{

}

ResponseThread::~ResponseThread()
{
    stopThread(1000);
}

void ResponseThread::run()
{
    while (!threadShouldExit())
    {
        wait(-1); // wait until the RMS value needs to be updated, and notify() is called
        updateRMS.clear();
        int samplesPerPixel = int(sampleRate * DISPLAY_TIME / width);
        int pixelIndex = 0;
        int samples = 0;
        float squaredSum = 0;

        // fetch impulse magnitude
        initializeImpulse();
        reverb.initialize(1, sampleRate);
        AudioSampleBuffer buffer;
        CustomSamplerSound samplerSound{ apvts, buffer, sampleRate };
        reverb.updateParams(samplerSound);
        reverb.process(impulse, impulse.getNumSamples());
        while (samples < impulse.getNumSamples())
        {
            if (updateRMS.test()) // exit the loop if a new request has been made
                break;
            float sample = impulse.getSample(0, samples);
            squaredSum += sample * sample;
            samples++;
            if (samples % samplesPerPixel == 0)
            {
                responseChangeQueue.enqueue(ResponseChange(ResponseChange::VALUE, pixelIndex++, sqrtf(squaredSum / samplesPerPixel)));
                squaredSum = 0;
            }
        }

        // fetch responses magnitude
        for (int i = 0; i < EMPTY_RATIO; i++)
        {
            if (updateRMS.test()) // exit the loop if a new request has been made
                break;
            empty.clear();
            reverb.process(empty, empty.getNumSamples());
            while (samples < empty.getNumSamples() * (i + 1) + impulse.getNumSamples())
            {
                if (updateRMS.test()) // exit the loop if a new request has been made
                    break;
                float sample = empty.getSample(0, (samples - impulse.getNumSamples()) % empty.getNumSamples());
                squaredSum += sample * sample;
                samples++;
                if (samples % samplesPerPixel == 0)
                {
                    responseChangeQueue.enqueue(ResponseChange(ResponseChange::VALUE, pixelIndex++, sqrtf(squaredSum / samplesPerPixel)));
                    squaredSum = 0;
                }
            }
        }
    }
}

void ResponseThread::initializeImpulse()
{
    impulse.setSize(1, int(sampleRate * IMPULSE_TIME), false, false);
    impulse.clear();
    for (int i = 0; i < impulse.getNumSamples(); i += 2)
    {
        impulse.setSample(0, i, 1);
    }
    empty.setSize(1, 1 + (sampleRate * DISPLAY_TIME - impulse.getNumSamples()) / EMPTY_RATIO);
}

void ResponseThread::calculateRMS(int windowWidth)
{
    if (windowWidth < width)
    {
        responseChangeQueue.enqueue(ResponseChange(ResponseChange::DECREASE_SIZE, windowWidth));
    }
    width = windowWidth;
    updateRMS.test_and_set();
    notify();
}

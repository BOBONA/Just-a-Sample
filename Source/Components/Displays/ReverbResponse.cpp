/*
  ==============================================================================

    ReverbResponse.cpp
    Created: 17 Jan 2024 11:16:38pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>
#include "ReverbResponse.h"

ReverbResponse::ReverbResponse(AudioProcessorValueTreeState& apvts, int sampleRate) : apvts(apvts), sampleRate(sampleRate), responseThread(sampleRate) // decreasing the sample rate is much faster but gives inaccurate response for the filters
{
    setBufferedToImage(true);
    apvts.addParameterListener(PluginParameters::REVERB_SIZE, this);
    apvts.addParameterListener(PluginParameters::REVERB_DAMPING, this);
    apvts.addParameterListener(PluginParameters::REVERB_LOWS, this);
    apvts.addParameterListener(PluginParameters::REVERB_HIGHS, this);
    apvts.addParameterListener(PluginParameters::REVERB_PREDELAY, this);
    apvts.addParameterListener(PluginParameters::REVERB_MIX, this);

    responseThread.size = apvts.getParameterAsValue(PluginParameters::REVERB_SIZE).getValue();
    responseThread.damping = apvts.getParameterAsValue(PluginParameters::REVERB_DAMPING).getValue();
    responseThread.lows = apvts.getParameterAsValue(PluginParameters::REVERB_LOWS).getValue();
    responseThread.highs = apvts.getParameterAsValue(PluginParameters::REVERB_HIGHS).getValue();
    responseThread.predelay = apvts.getParameterAsValue(PluginParameters::REVERB_PREDELAY).getValue();
    responseThread.mix = apvts.getParameterAsValue(PluginParameters::REVERB_MIX).getValue();
    responseThread.startThread();

    startTimerHz(60);
}

ReverbResponse::~ReverbResponse()
{
    apvts.removeParameterListener(PluginParameters::REVERB_SIZE, this);
    apvts.removeParameterListener(PluginParameters::REVERB_DAMPING, this);
    apvts.removeParameterListener(PluginParameters::REVERB_LOWS, this);
    apvts.removeParameterListener(PluginParameters::REVERB_HIGHS, this);
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
        float rms = rmsRecordings[int(float(i) * rmsRecordingsEffectiveSize / bounds.getWidth())];
        float height = jmap<float>(rms, 0.f, 1.f, 0.f, float(getHeight()));
        g.drawVerticalLine(i, (getHeight() - height) / 2.f, (getHeight() + height) / 2.f);
    }
}

void ReverbResponse::resized()
{
    responseThread.calculateRMS(getWidth());
}

void ReverbResponse::enablementChanged()
{
    repaint();
}

void ReverbResponse::parameterChanged(const String& parameterID, float newValue)
{
    if (parameterID == PluginParameters::REVERB_SIZE)
        responseThread.size = newValue;
    else if (parameterID == PluginParameters::REVERB_DAMPING)
        responseThread.damping = newValue;
    else if (parameterID == PluginParameters::REVERB_LOWS)
        responseThread.lows = newValue;
    else if (parameterID == PluginParameters::REVERB_HIGHS)
        responseThread.highs = newValue;
    else if (parameterID == PluginParameters::REVERB_PREDELAY)
        responseThread.predelay = newValue;
    else if (parameterID == PluginParameters::REVERB_MIX)
        responseThread.mix = newValue;
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

ResponseThread::ResponseThread(int sampleRate) : Thread("Reverb_Response_Thread"), sampleRate(sampleRate)
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
        int samplesPerPixel = int(sampleRate / SAMPLE_RATE_RATIO * DISPLAY_TIME / width);
        int pixelIndex = 0;
        int samples = 0;
        float squaredSum = 0;

        // fetch impulse magnitude
        initializeImpulse();
        reverb.initialize(1, int(sampleRate / SAMPLE_RATE_RATIO));
        reverb.updateParams(size, damping, predelay, lows, highs, mix);
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
    // exponential chirp (seems to provide most accurate frequency response)
    impulse.setSize(1, int(sampleRate / SAMPLE_RATE_RATIO * IMPULSE_TIME), false, false);
    for (int i = 1; i <= impulse.getNumSamples(); i++)
    {
        impulse.setSample(0, i - 1,
            sinf(MathConstants<float>::twoPi * CHIRP_START * (powf(CHIRP_END / CHIRP_START, float(i) / impulse.getNumSamples()) - 1.f) /
                ((float(impulse.getNumSamples()) / i) * logf(CHIRP_END / CHIRP_START)))
        );
    }
    empty.setSize(1, int(1.f + (sampleRate / SAMPLE_RATE_RATIO * DISPLAY_TIME - impulse.getNumSamples()) / EMPTY_RATIO));
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

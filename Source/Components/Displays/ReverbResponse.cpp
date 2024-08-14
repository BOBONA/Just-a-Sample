/*
  ==============================================================================

    ReverbResponse.cpp
    Created: 17 Jan 2024 11:16:38pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>
#include "ReverbResponse.h"

ReverbResponse::ReverbResponse(APVTS& apvts, int sampleRate) : apvts(apvts), sampleRate(sampleRate),
    lowsAttachment(*apvts.getParameter(PluginParameters::REVERB_LOWS), [this](float newValue) { lows = newValue; repaint(); }, apvts.undoManager),
    highsAttachment(*apvts.getParameter(PluginParameters::REVERB_HIGHS), [this](float newValue) { highs = newValue; repaint(); }, apvts.undoManager),
    mixAttachment(*apvts.getParameter(PluginParameters::REVERB_MIX), [this](float newValue) { mix = newValue; repaint(); }, apvts.undoManager),
    responseThread(apvts, sampleRate),
    response(1, int(ReverbResponseThread::DISPLAY_TIME * sampleRate / ReverbResponseThread::SAMPLE_RATE_RATIO))
{
    lowsAttachment.sendInitialUpdate();
    highsAttachment.sendInitialUpdate();
    mixAttachment.sendInitialUpdate();

    response.clear();
    responseThread.startThread();

    setBufferedToImage(true);
    startTimerHz(60);
}

void ReverbResponse::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Calculate the values
    float numPointsPerPixel = 0.5f;
    int numPoints = int(numPointsPerPixel * getWidth());
    int samplesPerPoint = int(response.getNumSamples() / numPoints);

    juce::Array<float> responseValues;
    for (int i = 0; i < numPoints; i++)
    {
        int sampleIndex = i * samplesPerPoint;
        float rms = response.getRMSLevel(0, sampleIndex, juce::jmin(samplesPerPoint, response.getNumSamples() - sampleIndex));
        float multiplier = 0.5f * lows + 0.5f * highs;
        float yPos = juce::jmap<float>(rms * multiplier, 0.f, 0.7f, bounds.getHeight() / 2.f, getHeight() * 0.02f);
        responseValues.add(yPos);
    }

    // Draw the response
    juce::Path rmsPath;
    for (int i = 0; i < numPoints; i++)
    {
        float x = float(i) / numPointsPerPixel;
        float y = responseValues[i];

        if (i == 0)
            rmsPath.startNewSubPath(x, y);
        else
            rmsPath.lineTo(x, y);
    }
    for (int i = numPoints - 1; i >= 0; i--)
    {
        float x = float(i) / numPointsPerPixel;
        float y = responseValues[i];

        rmsPath.lineTo(x, getHeight() - y);
    }
    rmsPath.closeSubPath();

    g.setColour(Colors::DARK);
    g.fillRect(0.f, mix * getHeight() / 2.f, getWidth() * 0.01f, float(getHeight()) * (1 - mix));
    g.fillPath(rmsPath);

    if (!isEnabled())
        g.fillAll(Colors::BACKGROUND.withAlpha(0.5f));
}

void ReverbResponse::enablementChanged()
{
    repaint();
}

void ReverbResponse::timerCallback()
{
    // Process the response thread changes
    ReverbResponseChange* change;
    while ((change = responseThread.getResponseChangeQueue().peek()) != nullptr)
    {
        if (change->type == ReverbResponseChange::CLEAR)
        {
            responseChanges = std::queue<ReverbResponseChange>();
            response.clear();
            numSamples = 0;
        }
        else
        {
            responseChanges.push(*change);
        }

        responseThread.getResponseChangeQueue().pop();
    }

    bool changesMade{ false };
    while (!responseChanges.empty())
    {
        auto& [type, samples] = responseChanges.front();
        response.copyFrom(0, numSamples, samples, 0, 0, samples.getNumSamples());
        numSamples += samples.getNumSamples();
        responseChanges.pop();

        changesMade = true;
    }

    if (changesMade)
        repaint();
}

ReverbResponseThread::ReverbResponseThread(const APVTS& apvts, int sampleRate) : Thread("Reverb_Response_Thread"), sampleRate(sampleRate),
    sizeAttachment(*apvts.getParameter(PluginParameters::REVERB_SIZE), [this](float value) { size = value; reverbChanged = true; notify();  }, apvts.undoManager),
    dampingAttachment(*apvts.getParameter(PluginParameters::REVERB_DAMPING), [this](float value) { damping = value; reverbChanged = true; notify(); }, apvts.undoManager),
    delayAttachment(*apvts.getParameter(PluginParameters::REVERB_PREDELAY), [this](float value) { delay = value; reverbChanged = true; notify(); }, apvts.undoManager),
    mixAttachment(*apvts.getParameter(PluginParameters::REVERB_MIX), [this](float value) { mix = value; reverbChanged = true; notify(); }, apvts.undoManager),
    impulse(1, int(DISPLAY_TIME * sampleRate / SAMPLE_RATE_RATIO))
{
    sizeAttachment.sendInitialUpdate();
    dampingAttachment.sendInitialUpdate();
    delayAttachment.sendInitialUpdate();
    mixAttachment.sendInitialUpdate();

    initializeImpulse();
}

ReverbResponseThread::~ReverbResponseThread()
{
    stopThread(5000);
}

void ReverbResponseThread::initializeImpulse()
{
    // Exponential chirp (seems to provide most accurate response)
    impulse.clear();
    auto impulseSize = int(IMPULSE_TIME * sampleRate / SAMPLE_RATE_RATIO);
    for (int i = 1; i <= impulseSize; i++)
    {
        impulse.setSample(0, i - 1,
            sinf(juce::MathConstants<float>::twoPi * CHIRP_START * (powf(CHIRP_END / CHIRP_START, float(i) / impulseSize) - 1.f) /
                (float(impulseSize) / i * logf(CHIRP_END / CHIRP_START)))
        );
    }
}

void ReverbResponseThread::run()
{
    while (!threadShouldExit())
    {
        if (!reverbChanged)
            bool _ = wait(-1);  // Wait until the RMS value needs to be updated, and notify() is called

        reverbChanged = false;

        initializeImpulse();
        reverb.initialize(1, int(sampleRate / SAMPLE_RATE_RATIO));
        reverb.updateParams(size, damping, delay, 1.f, 1.f, mix);
        reverb.process(impulse, impulse.getNumSamples());
        responseChangeQueue.enqueue({ ReverbResponseChange::CLEAR });
        responseChangeQueue.enqueue({ ReverbResponseChange::NEW_SAMPLES, impulse });
    }
}

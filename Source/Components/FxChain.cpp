/*
  ==============================================================================

    FxChain.cpp
    Created: 5 Jan 2024 3:26:37pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>

#include "FxChain.h"

FxChain::FxChain(JustaSampleAudioProcessor& processor) :
    reverbModule(this, processor.apvts, "Reverb", PluginParameters::REVERB_ENABLED),
    distortionModule(this, processor.apvts, "Distortion", PluginParameters::DISTORTION_ENABLED),
    eqModule(this, processor.apvts, "EQ", PluginParameters::EQ_ENABLED),
    chorusModule(this, processor.apvts, "Chorus", PluginParameters::CHORUS_ENABLED),
    eqDisplay(processor.apvts, int(processor.getSampleRate())),
    reverbDisplay(processor.apvts, int(processor.getSampleRate())),
    distortionDisplay(processor.apvts, int(processor.getSampleRate())),
    fxPermAttachment(*processor.apvts.getParameter(PluginParameters::FX_PERM), [&](float newValue) { moduleOrder = PluginParameters::paramToPerm(int(newValue)); oldVal = int(newValue); resized(); }, & processor.undoManager)
{
    reverbModule.setDisplayComponent(&reverbDisplay);
    reverbModule.addRow({ ModuleControl{"Room size", PluginParameters::REVERB_SIZE}, {"Damping", PluginParameters::REVERB_DAMPING}, {"Delay", PluginParameters::REVERB_PREDELAY} });
    reverbModule.addRow({ ModuleControl{"Lows", PluginParameters::REVERB_LOWS}, {"Highs", PluginParameters::REVERB_HIGHS}, {"Mix", PluginParameters::REVERB_MIX} });
    reverbModule.setAlwaysOnTop(true);
    addAndMakeVisible(reverbModule);

    distortionModule.setDisplayComponent(&distortionDisplay, 40.f);
    distortionModule.addRow({ ModuleControl{"Density", PluginParameters::DISTORTION_DENSITY} });
    distortionModule.addRow({ ModuleControl{"Highpass", PluginParameters::DISTORTION_HIGHPASS}, {"Mix", PluginParameters::DISTORTION_MIX} });
    distortionModule.setAlwaysOnTop(true);
    addAndMakeVisible(distortionModule);

    eqModule.setDisplayComponent(&eqDisplay);
    eqModule.addRow({ ModuleControl{"Low Gain", PluginParameters::EQ_LOW_GAIN}, {"Mid Gain", PluginParameters::EQ_MID_GAIN}, {"High Gain", PluginParameters::EQ_HIGH_GAIN} });
    eqModule.setAlwaysOnTop(true);
    addAndMakeVisible(eqModule);

    chorusModule.addRow({ ModuleControl{"Rate", PluginParameters::CHORUS_RATE}, {"Depth", PluginParameters::CHORUS_DEPTH}, {"Center Delay", PluginParameters::CHORUS_CENTER_DELAY} });
    chorusModule.addRow({ ModuleControl{"Feedback", PluginParameters::CHORUS_FEEDBACK}, {"Mix", PluginParameters::CHORUS_MIX} });
    chorusModule.setAlwaysOnTop(true);
    addAndMakeVisible(chorusModule);

    fxPermAttachment.sendInitialUpdate();
    addMouseListener(this, true);
}

FxChain::~FxChain()
{
}

void FxChain::paint(juce::Graphics& g)
{
    if (dragging)
    {
        g.setColour(lnf.SAMPLE_BOUNDS_SELECTED_COLOR);
        g.fillRect(targetArea);
    }
}

void FxChain::resized()
{
    auto bounds = getLocalBounds();
    auto moduleWidth = bounds.getWidth() / 4;
    for (const auto& fxType : moduleOrder)
    {
        auto& module = getModule(fxType);
        auto moduleBounds = bounds.removeFromLeft(moduleWidth);
        if (!(dragging && &module == dragComp))
            module.setBounds(moduleBounds);
        else
            targetArea = moduleBounds;
    }
    if (dragging)
    {
        auto dragCompBounds = dragComp->getBounds();
        dragCompBounds.setPosition(jlimit<int>(0, getWidth() - dragComp->getWidth(), mouseX + dragOffset), 0);
        dragComp->setBounds(dragCompBounds);
        repaint();
    }
}

void FxChain::mouseDrag(const MouseEvent& event)
{
    if (dragging)
    {
        auto localEvent = event.getEventRelativeTo(this);
        mouseX = localEvent.x;
        auto dragCompBounds = dragComp->getBounds();
        dragCompBounds.setPosition(jlimit<int>(0, getWidth() - dragComp->getWidth(), mouseX + dragOffset), 0);

        // see if the chain's module order needs to be updated
        auto bounds = getLocalBounds();
        auto moduleWidth = bounds.getWidth() / 4;
        for (int i = 0; i < 4; i++)
        {
            auto moduleBounds = bounds.removeFromLeft(moduleWidth);
            if (i != dragCompIndex && moduleBounds.getX() <= dragCompBounds.getCentreX() && dragCompBounds.getCentreX() <= moduleBounds.getRight())
            {
                auto fxType = moduleOrder[i];
                moduleOrder[i] = moduleOrder[dragCompIndex];
                moduleOrder[dragCompIndex] = fxType;
                dragCompIndex = i;
            }
        }

        resized();
    }
}

void FxChain::dragStarted(const juce::String& moduleName, const juce::MouseEvent& event)
{
    dragging = true;
    if (moduleName == "Reverb")
    {
        dragTarget = PluginParameters::REVERB;
        dragComp = &reverbModule;
    }
    else if (moduleName == "Distortion")
    {
        dragTarget = PluginParameters::DISTORTION;
        dragComp = &distortionModule;
    }
    else if (moduleName == "EQ")
    {
        dragTarget = PluginParameters::EQ;
        dragComp = &eqModule;
    }
    else if (moduleName == "Chorus")
    {
        dragTarget = PluginParameters::CHORUS;
        dragComp = &chorusModule;
    }
    else
    {
        dragging = false; // shouldn't happen
    }

    if (dragComp)
    {
        for (int i = 0; i < 4; i++)
            if (&getModule(moduleOrder[i]) == dragComp)
                dragCompIndex = i;
        dragComp->toFront(true);
        mouseX = dragComp->getX();
        dragOffset = dragComp->getX() - event.getEventRelativeTo(this).x;
    }
}

void FxChain::dragEnded()
{
    dragging = false;
    int newVal = PluginParameters::permToParam(moduleOrder);
    if (newVal != oldVal)
        fxPermAttachment.setValueAsCompleteGesture(float(newVal));
    resized();
}

FxModule& FxChain::getModule(PluginParameters::FxTypes type)
{
    switch (type)
    {
    case PluginParameters::REVERB:
        return reverbModule;
    case PluginParameters::DISTORTION:
        return distortionModule;
    case PluginParameters::EQ:
        return eqModule;
    case PluginParameters::CHORUS:
        return chorusModule;
    default:
        return reverbModule; // shouldn't happen
    }
}

/*
  ==============================================================================

    FxChain.cpp
    Created: 5 Jan 2024 3:26:37pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>

#include "FxChain.h"

void FxChainShadows::resized()
{
    int innerShadowOffset = int(0.001f * getWidth());
    innerShadow.setOffset({ 0, innerShadowOffset }, 0);
    innerShadow.setOffset({ 0, -innerShadowOffset }, 1);
    dragShadow.setOffset({ innerShadowOffset, innerShadowOffset });
}

void FxChainShadows::paint(juce::Graphics& g)
{
    juce::Path innerShadowPath;
    innerShadowPath.addRectangle(getLocalBounds());
    innerShadow.render(g, innerShadowPath);

    auto parent = dynamic_cast<FxChain*>(getParentComponent());
    if (parent->isChainDragging())
    {
        juce::Path dragShadowPath;
        dragShadowPath.addRectangle(parent->getDragComp()->getBounds());
        dragShadow.render(g, dragShadowPath);
    }
}

FxChain::FxChain(JustaSampleAudioProcessor& processor) :
    eqDisplay(processor.APVTS(), int(processor.getSampleRate())),
    reverbDisplay(processor.APVTS(), int(processor.getSampleRate())),
    distortionDisplay(processor.APVTS(), int(processor.getSampleRate())),
    chorusDisplay(processor.APVTS(), int(processor.getSampleRate())),

    reverbModule(this, processor.APVTS(), "Reverb", PluginParameters::REVERB, PluginParameters::REVERB_ENABLED, PluginParameters::REVERB_MIX, reverbDisplay),
    distortionModule(this, processor.APVTS(), "Distortion", PluginParameters::DISTORTION, PluginParameters::DISTORTION_ENABLED, PluginParameters::DISTORTION_MIX, distortionDisplay),
    eqModule(this, processor.APVTS(), "Equalizer", PluginParameters::EQ, PluginParameters::EQ_ENABLED, eqDisplay),
    chorusModule(this, processor.APVTS(), "Chorus", PluginParameters::CHORUS, PluginParameters::CHORUS_ENABLED, PluginParameters::CHORUS_MIX, chorusDisplay),

    fxPermAttachment(*processor.APVTS().getParameter(PluginParameters::FX_PERM), [&](float newValue) { moduleOrder = PluginParameters::paramToPerm(int(newValue)); oldVal = int(newValue); resized(); }, &processor.getUndoManager())
{
    reverbModule.addRotary(PluginParameters::REVERB_SIZE, "Size", { 11.4f, 82.f }, 87.f);
    reverbModule.addRotary(PluginParameters::REVERB_DAMPING, "Damping", { 108.4f, 52.f }, 87.f);
    reverbModule.addRotary(PluginParameters::REVERB_PREDELAY, "Delay", { 208.f, 82.f }, 87.f, "ms");
    reverbModule.addRotary(PluginParameters::REVERB_LOWS, "Lows", { 302.f, 52.f }, 87.f);
    reverbModule.addRotary(PluginParameters::REVERB_HIGHS, "Highs", { 399.4f, 82.f }, 87.f);
    addAndMakeVisible(reverbModule);

    distortionModule.addRotary(PluginParameters::DISTORTION_DENSITY, "Density", { 99.f, 46.f }, 110.f);
    distortionModule.addRotary(PluginParameters::DISTORTION_HIGHPASS, "Highpass", { 289.f, 46.f }, 110.f);
    addAndMakeVisible(distortionModule);

    eqModule.addRotary(PluginParameters::EQ_LOW_GAIN, "Lows", { 44.5f, 46.f }, 110.f, "db");
    eqModule.addRotary(PluginParameters::EQ_MID_GAIN, "Mids", { 194.f, 46.f }, 110.f, "db");
    eqModule.addRotary(PluginParameters::EQ_HIGH_GAIN, "Highs", { 344.f, 46.f }, 110.f, "db");
    addAndMakeVisible(eqModule);

    chorusModule.addRotary(PluginParameters::CHORUS_RATE, "Rate", { 40.f, 82.f }, 87.f, "hz");
    chorusModule.addRotary(PluginParameters::CHORUS_DEPTH, "Depth", { 150.f, 52.f }, 87.f);
    chorusModule.addRotary(PluginParameters::CHORUS_CENTER_DELAY, "Delay", { 260.6f, 82.f }, 87.f, "ms");
    chorusModule.addRotary(PluginParameters::CHORUS_FEEDBACK, "Feedback", { 371.f, 52.f }, 87.f);
    addAndMakeVisible(chorusModule);

    shadows.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(shadows);

    fxPermAttachment.sendInitialUpdate();
    addMouseListener(this, true);
}

void FxChain::paint(juce::Graphics& g)
{
    g.setColour(Colors::SLATE);

    auto bounds = getLocalBounds().toFloat();

    auto dividerWidth = int(std::ceil(getWidth() * Layout::fxChainDivider));
    auto moduleWidth = int(std::round((bounds.getWidth() - 3 * dividerWidth) / 4.f));
    for (size_t i = 0; i < moduleOrder.size() - 1; i++)
    {
        bounds.removeFromLeft(moduleWidth);
        auto divider = bounds.removeFromLeft(dividerWidth);
        g.fillRect(divider);
    }
}

void FxChain::resized()
{
    auto bounds = getLocalBounds().toFloat();

    auto dividerWidth = int(std::ceil(getWidth() * Layout::fxChainDivider));
    auto moduleWidth = int(std::round((bounds.getWidth() - 3 * dividerWidth) / 4.f));
    for (const auto& fxType : moduleOrder)
    {
        auto& module = getModule(fxType);
        auto moduleBounds = bounds.removeFromLeft(moduleWidth);
        bounds.removeFromLeft(dividerWidth);
        if (!(dragging && &module == dragComp))
            module.setBounds(moduleBounds.toNearestInt());
    }

    shadows.setBounds(getLocalBounds());

    if (dragging)
    {
        auto dragCompBounds = dragComp->getBounds();
        dragCompBounds.setPosition(juce::jlimit<int>(0, getWidth() - dragComp->getWidth(), mouseX + dragOffset), 0);
        dragComp->setBounds(dragCompBounds);
        repaint();
    }
}

void FxChain::mouseDrag(const juce::MouseEvent& event)
{
    if (!dragging)
        return;

    auto localEvent = event.getEventRelativeTo(this);
    mouseX = localEvent.x;
    auto dragCompBounds = dragComp->getBounds();
    dragCompBounds.setPosition(juce::jlimit<int>(0, getWidth() - dragComp->getWidth(), mouseX + dragOffset), 0);

    // See if the chain's module order needs to be updated
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

void FxChain::dragStarted(Component* component, const juce::MouseEvent& event)
{
    dragging = true;
    dragComp = component;
    dragTarget = dynamic_cast<FxModule*>(component)->getEffectType();
    
    for (int i = 0; i < 4; i++)
        if (&getModule(moduleOrder[i]) == dragComp)
            dragCompIndex = i;

    shadows.toFront(true);
    dragComp->toFront(true);
    mouseX = dragComp->getX();
    dragOffset = dragComp->getX() - event.getEventRelativeTo(this).x;
}

void FxChain::dragEnded()
{
    dragging = false;
    int newVal = permToParam(moduleOrder);
    if (newVal != oldVal)
        fxPermAttachment.setValueAsCompleteGesture(float(newVal));
    dragComp->toBehind(&shadows);

    resized();
    repaint();
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
    }
}

/*
  ==============================================================================

    SampleEditor.cpp
    Created: 19 Sep 2023 2:03:29pm
    Author:  binya

  ==============================================================================
*/

#include <JuceHeader.h>
#include "SampleEditor.h"

//==============================================================================
SampleEditorOverlay::SampleEditorOverlay(APVTS& apvts, juce::Array<CustomSamplerVoice*>& synthVoices) : synthVoices(synthVoices)
{
    viewStart = apvts.state.getPropertyAsValue(PluginParameters::UI_VIEW_START, apvts.undoManager);
    viewEnd = apvts.state.getPropertyAsValue(PluginParameters::UI_VIEW_END, apvts.undoManager);
    sampleStart = apvts.state.getPropertyAsValue(PluginParameters::SAMPLE_START, apvts.undoManager);
    sampleEnd = apvts.state.getPropertyAsValue(PluginParameters::SAMPLE_END, apvts.undoManager);
    isLooping = apvts.getParameterAsValue(PluginParameters::IS_LOOPING);
    loopingHasStart = apvts.state.getPropertyAsValue(PluginParameters::LOOPING_HAS_START, apvts.undoManager);
    loopingHasEnd = apvts.state.getPropertyAsValue(PluginParameters::LOOPING_HAS_END, apvts.undoManager);
    viewStart.addListener(this);
    viewEnd.addListener(this);
    sampleStart.addListener(this);
    sampleEnd.addListener(this);

    loopIcon.startNewSubPath(4, 0);
    loopIcon.lineTo(0, 0);
    loopIcon.lineTo(0, 10);
    loopIcon.lineTo(4, 10);
    loopIcon.startNewSubPath(11, 0);
    loopIcon.lineTo(15, 0);
    loopIcon.lineTo(15, 10);
    loopIcon.lineTo(11, 10);

    loopIconArrows.addTriangle(4, -3, 4, 3, 9, 0);
    loopIconArrows.addTriangle(11, 7, 11, 13, 6, 10);

    auto transform = juce::AffineTransform::scale(10.f / loopIcon.getBounds().getWidth()); // scaling to fit in 10px
    loopIcon.applyTransform(transform);
    loopIconArrows.applyTransform(transform);
}

SampleEditorOverlay::~SampleEditorOverlay()
{
    viewStart.removeListener(this);
    viewEnd.removeListener(this);
    sampleStart.removeListener(this);
    sampleEnd.removeListener(this);
}

void SampleEditorOverlay::paint(juce::Graphics& g)
{
    using namespace juce;
    if (sample)
    {
        int viewStartValue = viewStart.getValue();
        int viewEndValue = viewEnd.getValue();
        int pathStartSample = sampleStart.getValue();
        auto pathStart = lnf.EDITOR_BOUNDS_WIDTH + sampleToPosition(pathStartSample);
        auto generalScale = float(viewEndValue - viewStartValue) / getWidth();
        // set the voice paths correctly
        auto numPlaying = 0;
        for (auto i = 0; i < synthVoices.size(); i++)
        {
            auto voice = synthVoices[i];
            if (voice->getCurrentState() != STOPPED)
            {
                numPlaying++;
                if (voicePaths.find(voice) == voicePaths.end())
                {
                    auto& create = voicePaths[voice];
                    create.startNewSubPath(pathStart, getHeight() / 2.f);
                }
            }
            else
            {
                voicePaths.erase(voice);
            }
        }
        Path voicePositionsPath{};
        for (auto& voicePair : voicePaths)
        {
            auto voice = voicePair.first;
            // add the voice location pointer
            auto location = voice->getEffectiveLocation();
            auto pos = jmap<float>(location - viewStartValue, 0, viewEndValue - viewStartValue, 0, getWidth());
            voicePositionsPath.addLineSegment(Line<float>(pos, 0, pos, getHeight()), 1);
            // paint the pitch processed waveform
            if (voice->getPlaybackMode() == PluginParameters::PLAYBACK_MODES::ADVANCED && voice->getBufferPitcher())
            {
                int pathEndSample = pathStartSample + voice->getCurrentSample() - voice->getBufferPitcher()->startDelay;
                auto pathEnd = lnf.EDITOR_BOUNDS_WIDTH + sampleToPosition(pathEndSample);
                float scale = generalScale * voice->getSampleRateConversion();
                auto& voicePath = voicePaths[voice];
                for (auto i = voicePath.getBounds().getWidth(); i < pathEnd - pathStart; i++)
                {
                    auto sample = voice->getBufferPitcher()->startDelay + i * scale;
                    auto availableSamples = jmin<int>(voice->getBufferPitcher()->totalPitchedSamples, voice->getBufferPitcher()->processedBuffer.getNumSamples());
                    if (sample >= availableSamples)
                    {
                        break;
                    }
                    auto level = voice->getBufferPitcher()->processedBuffer.getSample(0, sample);
                    if (scale > 1)
                    {
                        level = FloatVectorOperations::findMaximum(voice->getBufferPitcher()->processedBuffer.getReadPointer(0, sample), jmin<int>(availableSamples - sample, scale));
                    }
                    if (level < -1) // data has not been copied over yet properly
                    {
                        break;
                    }
                    auto s = jmap<float>(level, 0, 1, 0, getHeight());
                    voicePath.lineTo(i + pathStart, (getHeight() - s) / 2);
                }
                g.setColour(lnf.PITCH_PROCESSED_WAVEFORM_COLOR.withAlpha(jlimit<float>(0.2f, 1.f, 1.5f / numPlaying)));
                g.strokePath(voicePath, PathStrokeType(lnf.PITCH_PROCESSED_WAVEFORM_THICKNESS));
            }
        }
        g.setColour(lnf.VOICE_POSITION_COLOR);
        g.strokePath(voicePositionsPath, PathStrokeType(1.f));
        // paint the start bound
        auto iconBounds = loopIcon.getBounds();
        int startPos = sampleToPosition(sampleStart.getValue());
        g.setColour(dragging && draggingTarget == EditorParts::SAMPLE_START ? lnf.SAMPLE_BOUNDS_SELECTED_COLOR : lnf.SAMPLE_BOUNDS_COLOR);
        g.fillPath(sampleStartPath, juce::AffineTransform::translation(startPos + lnf.EDITOR_BOUNDS_WIDTH / 2.f, 0));
        // paint the start icon
        if (isLooping.getValue())
        {
            g.fillRoundedRectangle(float(startPos), getHeight() - iconBounds.getHeight() - 4, iconBounds.getWidth() + 4, iconBounds.getHeight() + 4, 3.f);
            g.setColour(Colours::darkgrey);
            auto iconTranslation = juce::AffineTransform::translation(lnf.EDITOR_BOUNDS_WIDTH + startPos, getHeight() - iconBounds.getHeight() - 2);
            g.strokePath(loopIcon, PathStrokeType(1.6f, PathStrokeType::JointStyle::curved), iconTranslation);
            g.fillPath(loopIconArrows, iconTranslation);
        }
        // paint the end bound
        int endPos = sampleToPosition(sampleEnd.getValue());
        g.setColour(dragging && draggingTarget == EditorParts::SAMPLE_END ? lnf.SAMPLE_BOUNDS_SELECTED_COLOR : lnf.SAMPLE_BOUNDS_COLOR);
        g.fillPath(sampleEndPath, juce::AffineTransform::translation(endPos + 3 * lnf.EDITOR_BOUNDS_WIDTH / 2.f, 0));
        // paint the end icon
        if (isLooping.getValue())
        {
            g.fillRoundedRectangle(endPos - iconBounds.getWidth(), getHeight() - iconBounds.getHeight() - 4.f, iconBounds.getWidth() + 4, iconBounds.getHeight() + 4, 3.f);
            g.setColour(Colours::darkgrey);
            auto iconTranslation = juce::AffineTransform::translation(lnf.EDITOR_BOUNDS_WIDTH + endPos - iconBounds.getWidth(), getHeight() - iconBounds.getHeight() - 2);
            g.strokePath(loopIcon, PathStrokeType(1.6f, PathStrokeType::JointStyle::curved), iconTranslation);
            g.fillPath(loopIconArrows, iconTranslation);
        }
    }
}

void SampleEditorOverlay::resized()
{
    painterWidth = getWidth() - 2 * lnf.EDITOR_BOUNDS_WIDTH;
    sampleStartPath.clear();
    sampleStartPath.addLineSegment(juce::Line<float>(0, 0, 0, getHeight()), lnf.EDITOR_BOUNDS_WIDTH);
    sampleStartPath.addLineSegment(juce::Line<float>(0, 0, 8, 0), 4);
    sampleStartPath.addLineSegment(juce::Line<float>(0, getHeight(), 10, getHeight()), 4);

    sampleEndPath.clear();
    sampleEndPath.addLineSegment(juce::Line<float>(0, 0, 0, getHeight()), lnf.EDITOR_BOUNDS_WIDTH);
    sampleEndPath.addLineSegment(juce::Line<float>(-10, 0, 0, 0), 4);
    sampleEndPath.addLineSegment(juce::Line<float>(-10, getHeight(), 0, getHeight()), 4);

    voicePaths.clear();
}

void SampleEditorOverlay::mouseMove(const MouseEvent& event)
{
    EditorParts editorPart = getClosestPartInRange(event.x, event.y);
    switch (editorPart)
    {
    case EditorParts::SAMPLE_START:
    case EditorParts::SAMPLE_END:
        setMouseCursor(MouseCursor::LeftRightResizeCursor);
        break;
    case EditorParts::LOOP_START_BUTTON:
    case EditorParts::LOOP_END_BUTTON:
        DBG("TEST");
        break;
    default:
        setMouseCursor(MouseCursor::NormalCursor);
    }
}

void SampleEditorOverlay::mouseDown(const juce::MouseEvent& event)
{
    if (!sample)
    {
        return;
    }
    EditorParts closest = getClosestPartInRange(event.getMouseDownX(), event.getMouseDownY());
    if (closest != EditorParts::NONE)
    {
        dragging = true;
        draggingTarget = closest;
        repaint();
    }
}

void SampleEditorOverlay::mouseUp(const juce::MouseEvent& event)
{
    if (!sample)
    {
        return;
    }
    dragging = false;
    repaint();
}

void SampleEditorOverlay::mouseDrag(const juce::MouseEvent& event)
{
    if (!sample || !dragging)
    {
        return;
    }
    auto newSample = positionToSample(event.getMouseDownX() + event.getOffsetFromDragStart().getX() - lnf.EDITOR_BOUNDS_WIDTH);
    switch (draggingTarget)
    {
    case EditorParts::SAMPLE_START:
        sampleStart = juce::jlimit<int>(int(viewStart.getValue()), int(sampleEnd.getValue()), newSample);
        break;
    case EditorParts::SAMPLE_END:
        sampleEnd = juce::jlimit<int>(int(sampleStart.getValue()), int(viewEnd.getValue()), newSample);
        break;
    }
}

EditorParts SampleEditorOverlay::getClosestPartInRange(int x, int y)
{
    using map = std::map<EditorParts, juce::Point<float>>;
    using pair = std::pair<EditorParts, juce::Point<float>>;
    map targets = map();
    targets.insert(pair(EditorParts::SAMPLE_START, juce::Point<float>(sampleToPosition(int(sampleStart.getValue())) + lnf.EDITOR_BOUNDS_WIDTH - 1, INFINITY)));
    targets.insert(pair(EditorParts::SAMPLE_END, juce::Point<float>(sampleToPosition(int(sampleEnd.getValue())) + lnf.EDITOR_BOUNDS_WIDTH + 1, INFINITY)));
    // rework to have priorities for targets
    /*if (isLooping.getValue())
    {
        targets.insert(pair(EditorParts::LOOP_START_BUTTON, juce::Point<float>(sampleToPosition(int(sampleStart.getValue())) + lnf.EDITOR_BOUNDS_WIDTH - 1, getHeight() - lnf.DRAGGABLE_SNAP)));
        targets.insert(pair(EditorParts::LOOP_END_BUTTON, juce::Point<float>(sampleToPosition(int(sampleEnd.getValue())) + lnf.EDITOR_BOUNDS_WIDTH + 1, getHeight() - lnf.DRAGGABLE_SNAP)));
    }*/

    auto closest = EditorParts::NONE;
    auto closestDistance = INFINITY;
    for (auto p : targets)
    {
        auto distance = 0.f;
        if (p.second.getX() == INFINITY)
        {
            distance = std::abs(p.second.getY() - y);
        }
        else if (p.second.getY() == INFINITY)
        {
            distance = std::abs(p.second.getX() - x);
        }
        else
        {
            distance = std::sqrt(std::pow(p.second.getX() - x, 2) + std::pow(p.second.getY() - y, 2));
        }
        if (distance < closestDistance && distance <= lnf.DRAGGABLE_SNAP)
        {
            closest = p.first;
            closestDistance = distance;
        }
    }
    return closest;
}

void SampleEditorOverlay::valueChanged(juce::Value& value)
{
    if (value.refersToSameSourceAs(viewStart) || value.refersToSameSourceAs(viewEnd))
    {
        voicePaths.clear();
    }
    repaint();
}


float SampleEditorOverlay::sampleToPosition(int sample)
{
    int start = viewStart.getValue();
    int end = viewEnd.getValue();
    return juce::jmap<double>(sample - start, 0, end - start, 0, painterWidth);
}

int SampleEditorOverlay::positionToSample(float position)
{
    int start = viewStart.getValue();
    int end = viewEnd.getValue();
    return start + juce::jmap<double>(position, 0, painterWidth, 0, end - start);
}

void SampleEditorOverlay::setSample(juce::AudioBuffer<float>& sample)
{
    this->sample = &sample;
    voicePaths.clear();
}

//==============================================================================
SampleEditor::SampleEditor(APVTS& apvts, juce::Array<CustomSamplerVoice*>& synthVoices) : apvts(apvts), overlay(apvts, synthVoices)
{
    apvts.state.addListener(this);

    addAndMakeVisible(&painter);
    overlay.toFront(true);
    addAndMakeVisible(&overlay);
}

SampleEditor::~SampleEditor()
{
    apvts.state.removeListener(this);
}

void SampleEditor::paint(juce::Graphics& g)
{
}

void SampleEditor::resized()
{
    auto bounds = getLocalBounds();
    overlay.setBounds(bounds);
    bounds.removeFromLeft(lnf.EDITOR_BOUNDS_WIDTH);
    bounds.removeFromRight(lnf.EDITOR_BOUNDS_WIDTH);
    painter.setBounds(bounds);
}

void SampleEditor::valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property)
{
    if (property.toString() == PluginParameters::UI_VIEW_START || property.toString() == PluginParameters::UI_VIEW_END)
    {
        int start = treeWhosePropertyHasChanged.getProperty(PluginParameters::UI_VIEW_START);
        int stop = treeWhosePropertyHasChanged.getProperty(PluginParameters::UI_VIEW_END);
        painter.setSampleView(start, stop);
    }
}

void SampleEditor::updateSamplePosition()
{
    overlay.repaint();
}

void SampleEditor::setSample(juce::AudioBuffer<float>& sample, bool resetUI)
{
    if (resetUI)
    {
        painter.setSample(sample);
    }
    else
    {
        painter.setSample(sample, apvts.state.getProperty(PluginParameters::UI_VIEW_START), apvts.state.getProperty(PluginParameters::UI_VIEW_END));
    }
    overlay.setSample(sample);
}

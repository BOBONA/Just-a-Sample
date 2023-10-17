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
SampleEditorOverlay::SampleEditorOverlay(APVTS& apvts, juce::Array<int>& voicePositions) : voicePositions(voicePositions)
{
    viewStart = apvts.state.getPropertyAsValue(PluginParameters::UI_VIEW_START, apvts.undoManager);
    viewEnd = apvts.state.getPropertyAsValue(PluginParameters::UI_VIEW_END, apvts.undoManager);
    sampleStart = apvts.state.getPropertyAsValue(PluginParameters::SAMPLE_START, apvts.undoManager);
    sampleEnd = apvts.state.getPropertyAsValue(PluginParameters::SAMPLE_END, apvts.undoManager);
    viewStart.addListener(this);
    viewEnd.addListener(this);
    sampleStart.addListener(this);
    sampleEnd.addListener(this);
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
        // paints the voice positions
        int startP = viewStart.getValue();
        int stopP = viewEnd.getValue();
        Path path{};
        for (auto i = 0; i < voicePositions.size(); i++)
        {
            if (voicePositions[i] > 0)
            {
                auto pos = jmap<float>(voicePositions[i] - startP, 0, stopP - startP, 0, getWidth());
                path.addLineSegment(Line<float>(pos, 0, pos, getHeight()), 1);
            }
        }
        g.setColour(lnf.VOICE_POSITION_COLOR);
        g.strokePath(path, PathStrokeType(1.f));
        // paints the sample bounds
        int startPos = sampleToPosition(sampleStart.getValue());
        g.setColour(dragging && draggingTarget == EditorParts::SAMPLE_START ? lnf.SAMPLE_BOUNDS_SELECTED_COLOR : lnf.SAMPLE_BOUNDS_COLOR);
        g.fillPath(sampleStartPath, juce::AffineTransform::translation(startPos + lnf.EDITOR_BOUNDS_WIDTH / 2, 0));

        int endPos = sampleToPosition(sampleEnd.getValue());
        g.setColour(dragging && draggingTarget == EditorParts::SAMPLE_END ? lnf.SAMPLE_BOUNDS_SELECTED_COLOR : lnf.SAMPLE_BOUNDS_COLOR);
        g.fillPath(sampleEndPath, juce::AffineTransform::translation(endPos + 3 * lnf.EDITOR_BOUNDS_WIDTH / 2, 0));
    }
}

void SampleEditorOverlay::resized()
{
    painterWidth = getWidth() - 2 * lnf.EDITOR_BOUNDS_WIDTH;
    sampleStartPath.clear();
    sampleStartPath.addLineSegment(juce::Line<float>(0, 0, 0, getHeight()), lnf.EDITOR_BOUNDS_WIDTH);
    sampleStartPath.addLineSegment(juce::Line<float>(0, 0, 8, 0), 4);
    sampleStartPath.addLineSegment(juce::Line<float>(0, getHeight(), 8, getHeight()), 4);

    sampleEndPath.clear();
    sampleEndPath.addLineSegment(juce::Line<float>(0, 0, 0, getHeight()), lnf.EDITOR_BOUNDS_WIDTH);
    sampleEndPath.addLineSegment(juce::Line<float>(-8, 0, 0, 0), 2);
    sampleEndPath.addLineSegment(juce::Line<float>(-8, getHeight(), 0, getHeight()), 4);
}

void SampleEditorOverlay::mouseDown(const juce::MouseEvent& event)
{
    if (!sample)
    {
        return;
    }
    // in order to find the closest dragging target
    using map = std::map<EditorParts, juce::Point<float>>;
    using pair = std::pair<EditorParts, juce::Point<float>>;
    map targets = map();
    targets.insert(pair(EditorParts::SAMPLE_START, juce::Point<float>(sampleToPosition(int(sampleStart.getValue())) + lnf.EDITOR_BOUNDS_WIDTH - 1, INFINITY)));
    targets.insert(pair(EditorParts::SAMPLE_END, juce::Point<float>(sampleToPosition(int(sampleEnd.getValue())) + lnf.EDITOR_BOUNDS_WIDTH + 1, INFINITY)));

    auto closest = EditorParts::NONE;
    auto closestDistance = INFINITY;
    for (auto p : targets)
    {
        auto distance = 0.f;
        if (p.second.getX() == INFINITY) 
        {
            distance = std::abs(p.second.getY() - event.getMouseDownY());
        }
        else if (p.second.getY() == INFINITY) 
        {
            distance = std::abs(p.second.getX() - event.getMouseDownX());
        }
        else
        {
            distance = std::sqrt(std::pow(p.second.getX() - event.getMouseDownX(), 2) + std::pow(p.second.getY() - event.getMouseDownY(), 2));
        }
        if (distance < closestDistance && distance <= lnf.DRAGGABLE_SNAP) 
        {
            closest = p.first;
            closestDistance = distance;
        }
    }
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

void SampleEditorOverlay::valueChanged(juce::Value& value)
{
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
}

//==============================================================================
SampleEditor::SampleEditor(APVTS& apvts, juce::Array<int>& voicePositions) : apvts(apvts), overlay(apvts, voicePositions)
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

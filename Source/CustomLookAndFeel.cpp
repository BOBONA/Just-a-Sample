/*
  ==============================================================================

    LookAndFeel.cpp
    Created: 19 Sep 2023 4:48:33pm
    Author:  binya

  ==============================================================================
*/

#include "CustomLookAndFeel.h"

#include "Sampler/CustomSamplerVoice.h"

juce::Slider::SliderLayout CustomLookAndFeel::getSliderLayout(juce::Slider& slider)
{
    auto bounds = slider.getLocalBounds().toFloat();
    bounds = bounds.reduced(bounds.getWidth() * Layout::rotaryPadding);

    float textSize = bounds.getWidth() * Layout::rotaryTextSize;

    juce::Slider::SliderLayout layout;
    layout.sliderBounds = slider.getLocalBounds();
    layout.textBoxBounds = juce::Rectangle(bounds.getX(), (bounds.getHeight() - textSize / 3.5f) / 2.f, bounds.getWidth(), textSize).getLargestIntegerWithin();

    return layout;
}

juce::Label* CustomLookAndFeel::createSliderTextBox(juce::Slider& slider)
{
    using namespace juce;

    auto bounds = slider.getLocalBounds().toFloat();
    bounds = bounds.reduced(bounds.getWidth() * Layout::rotaryPadding);

    auto label = new Label();
    label->setColour(Label::textColourId, Colors::DARK);
    label->setColour(Label::textWhenEditingColourId, Colors::DARK);
    label->setColour(Label::outlineWhenEditingColourId, Colours::transparentWhite);
    label->setColour(TextEditor::highlightedTextColourId, Colors::DARK);
    label->setColour(TextEditor::highlightColourId, Colours::transparentWhite);
    label->setColour(CaretComponent::caretColourId, Colors::DARK);

    label->setJustificationType(Justification::centredBottom);
    label->setFont(getInriaSansBold().withHeight(bounds.getWidth() * Layout::rotaryTextSize));
    label->setHasFocusOutline(false);
    label->setKeyboardType(TextInputTarget::decimalKeyboard);
    label->setEditable(false, true, false);

    label->onEditorShow = [label]
    {
        auto editor = label->getCurrentTextEditor();
        editor->setJustification(Justification::centredBottom);
        editor->setIndents(0, 0);
        editor->moveCaretToEnd();
        editor->selectAll();

        // Some compensation to keep the text in a constant position
        auto editorBounds = editor->getBounds();
        editorBounds.setY(editorBounds.getY() - 2);
        editorBounds.translate(2, 0);
        editorBounds.expand(0, 2);
        editor->setBounds(editorBounds);
    };

    return label;
}

void CustomLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float, float, juce::Slider& slider)
{
    using namespace juce;

    auto bounds = Rectangle(x, y, width, height).toFloat();
    bounds = bounds.reduced(bounds.getWidth() * Layout::rotaryPadding);
    bounds.setHeight(bounds.getWidth() * Layout::rotaryHeightRatio);

    auto radius = bounds.getWidth() / 2.f;
    auto lineWidth = radius * 0.2f;

    constexpr float rotaryStartAngle = MathConstants<float>::pi * (1 + 5.f / 26.f);
    constexpr float rotaryEndAngle = rotaryStartAngle + MathConstants<float>::pi * (2.f - 5.f / 13.f);
    auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Draw the background arc
    const float lineOffset = lineWidth * 0.5f;
    Path backgroundArc;
    backgroundArc.addCentredArc(bounds.getCentreX(), bounds.getY() + radius + lineOffset / 2.f, radius - lineOffset, radius - lineOffset / 2.f,
        0.0f, rotaryStartAngle, rotaryEndAngle, true);

    g.setColour(Colors::DARK);
    g.strokePath(backgroundArc, PathStrokeType(lineWidth));

    // Draw the value arc
    float baseAngle = rotaryStartAngle;
    if (approximatelyEqual(slider.getRange().getStart(), -slider.getRange().getEnd()))
        baseAngle = MathConstants<float>::twoPi;

    Path valueArc;
    valueArc.addCentredArc(bounds.getCentreX(), bounds.getY() + radius + lineOffset / 2.f, radius - lineOffset, radius - lineOffset / 2.f,
        0.0f, baseAngle, toAngle, true);

    g.setColour(Colors::HIGHLIGHT);
    g.strokePath(valueArc, PathStrokeType(lineWidth));

    // Draw the thumb
    auto thumbWidth = bounds.getWidth() * 0.27f;
    auto thumbHeight = thumbWidth * 0.5f;
    auto thumbRound = bounds.getWidth() * 0.065f;
    auto thumb = Rectangle((bounds.getWidth() - thumbHeight) / 2.f, bounds.getY() + bounds.getHeight() - (thumbWidth - lineOffset) / 2.f, thumbHeight, thumbWidth);

    Path thumbPath;
    thumbPath.addRoundedRectangle(thumb, thumbRound);
    thumbPath.applyTransform(AffineTransform::rotation(toAngle + MathConstants<float>::pi, bounds.getWidth() / 2.f, bounds.getY() + (bounds.getWidth() + lineOffset) / 2.f));
    
    g.setColour(Colors::SLATE);
    g.fillPath(thumbPath, AffineTransform::translation(bounds.getX(), 0));
    g.setColour(Colors::DARK);
    g.strokePath(thumbPath, PathStrokeType(bounds.getWidth() * 0.016f), AffineTransform::translation(bounds.getX(), 0));

    // Draw the unit label
    g.setColour(Colors::DARK);
    g.setFont(getInriaSansBold().withHeight(20.f * bounds.getWidth() / Layout::standardRotarySize));

    var unit = slider.getProperties().getWithDefault(ComponentProps::LABEL_UNIT, "");
    if (slider.getValue() >= 1000)
        unit = slider.getProperties().getWithDefault(ComponentProps::GREATER_UNIT, unit);
    g.drawText(unit, Rectangle{ bounds.getX(), bounds.getY() + 0.78f * bounds.getHeight(), bounds.getWidth(), g.getCurrentFont().getAscent() }, Justification::centredBottom);
}

void CustomLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    const juce::Font font(getLabelFont(label));

    g.setColour(label.findColour(juce::Label::textColourId));
    g.setFont(font);

    auto textArea = label.getLocalBounds();

    g.drawText(label.getText(), textArea, label.getJustificationType(), false);
}

template <EnvelopeSlider Direction>
void EnvelopeSliderLookAndFeel<Direction>::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float, float, float, juce::Slider& slider)
{
    using namespace juce;

    auto bounds = Rectangle(x, y, width, height).toFloat();
    bounds = bounds.reduced(bounds.getWidth() * Layout::rotaryPadding);
    float exp = slider.getValue();

    Path curve;
    for (int i = 0; i < width; i++)
    {
        float xPos = float(i) / width;
        float yPos = CustomSamplerVoice::exponentialCurve(exp, xPos);

        if (Direction == EnvelopeSlider::release)
            yPos = 1 - yPos;

        if (i == 0)
            curve.startNewSubPath(i, bounds.getHeight() * (1 - yPos));
        else
            curve.lineTo(i, bounds.getHeight() * (1 - yPos));
    }

    g.setColour(Colors::DARK);
    g.strokePath(curve, PathStrokeType(0.08f * width, PathStrokeType::curved, PathStrokeType::rounded), curve.getTransformToScaleToFit(bounds, false));
}

const juce::Font& getInriaSans()
{
    static juce::Font inriaSans{ juce::FontOptions(juce::Typeface::createSystemTypefaceFor(BinaryData::InriaSansRegular_ttf, BinaryData::InriaSansRegular_ttfSize)) };

    return inriaSans;
}

const juce::Font& getInriaSansBold()
{
    static juce::Font inriaSansBold{ juce::FontOptions(juce::Typeface::createSystemTypefaceFor(BinaryData::InriaSansBold_ttf, BinaryData::InriaSansBold_ttfSize)) };

    return inriaSansBold;
}

juce::Path getOutlineFromSVG(const char* data)
{
    auto xml = juce::parseXML(data);
    jassert(xml != nullptr);
    return juce::Drawable::createFromSVG(*xml)->getOutlineAsPath();
}

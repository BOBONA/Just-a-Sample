/*
  ==============================================================================

    LookAndFeel.cpp
    Created: 19 Sep 2023 4:48:33pm
    Author:  binya

  ==============================================================================
*/

#include "CustomLookAndFeel.h"

#include "Sampler/CustomSamplerVoice.h"

CustomLookAndFeel::CustomLookAndFeel()
{
    setColour(juce::Label::textColourId, Colors::DARK);
    setColour(juce::Label::textWhenEditingColourId, Colors::DARK);
    setColour(juce::Label::outlineWhenEditingColourId, juce::Colours::transparentWhite);
    setColour(juce::TextEditor::highlightColourId, Colors::SLATE.withAlpha(0.15f));
    setColour(juce::TextEditor::highlightedTextColourId, Colors::DARK);
    setColour(juce::CaretComponent::caretColourId, Colors::DARK);
    setColour(juce::ComboBox::textColourId, Colors::DARK);
}

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
    label->setColour(TextEditor::highlightColourId, Colours::transparentWhite);

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
        auto editorBounds = editor->getBounds().toFloat();
        float offsetAmount = editorBounds.getHeight() * 0.125f;
        editorBounds.setY(editorBounds.getY() - offsetAmount);
        editorBounds.translate(offsetAmount, 0);
        editorBounds.expand(0, offsetAmount);
        editor->setBounds(editorBounds.toNearestInt());
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

    if (slider.getProperties().contains(ComponentProps::ROTARY_ICON))
    {
        auto iconPath = dynamic_cast<ReferenceCountedPath*>(slider.getProperties()[ComponentProps::ROTARY_ICON].getObject())->path;
        g.fillPath(iconPath, iconPath.getTransformToScaleToFit(Rectangle{ bounds.getX() + 0.025f * bounds.getWidth(), bounds.getY() + 0.78f * bounds.getHeight(), bounds.getWidth(), 12.f * bounds.getWidth() / Layout::standardRotarySize}, true));
    }
    else
    {
        var unit = slider.getProperties().getWithDefault(ComponentProps::ROTARY_UNIT, "");
        if (slider.getValue() >= 1000)
            unit = slider.getProperties().getWithDefault(ComponentProps::ROTARY_GREATER_UNIT, unit);
        g.setFont(getInriaSansBold().withHeight(20.f * bounds.getWidth() / Layout::standardRotarySize));
        g.drawText(unit, Rectangle{ bounds.getX(), bounds.getY() + 0.78f * bounds.getHeight(), bounds.getWidth(), g.getCurrentFont().getAscent() }, Justification::centredBottom);
    }
}

void CustomLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    using namespace juce;

    if (!label.isBeingEdited())
    {
        auto textArea = label.getLocalBounds();

        // Draw the background (if the colors are set)
        Path background;
        background.addRoundedRectangle(textArea.toFloat(), textArea.getHeight() * 0.2f);

        g.setColour(label.findColour(Label::backgroundColourId));
        g.fillPath(background);

        float borderSize = textArea.getHeight() * 0.05f;
        g.setColour(label.findColour(Label::outlineColourId));
        g.strokePath(background, PathStrokeType(borderSize), background.getTransformToScaleToFit(textArea.toFloat().reduced(borderSize), true));

        // Draw the text
        const Font font(getLabelFont(label));

        g.setColour(label.findColour(Label::textColourId));
        g.setFont(font);
        g.drawText(label.getText(), textArea, label.getJustificationType(), label.getProperties().contains(ComponentProps::LABEL_ELLIPSES));
    }
}

juce::Button* CustomLookAndFeel::createFilenameComponentBrowseButton(const juce::String& text)
{
    auto filenameAddButton = new juce::ShapeButton("", Colors::DARK, Colors::DARK, Colors::DARK);
    filenameAddButton->setShape(getOutlineFromSVG(BinaryData::IconAdd_svg), true, true, false);
    return filenameAddButton;
}

void CustomLookAndFeel::layoutFilenameComponent(juce::FilenameComponent& filenameComp, juce::ComboBox* filenameBox, juce::Button* browseButton)
{
    if (browseButton == nullptr || filenameBox == nullptr)
        return;

    auto compBounds = filenameComp.getLocalBounds().toFloat();

    auto addButtonBounds = compBounds.removeFromRight(filenameComp.getHeight() * 0.945f).reduced(0.158f * compBounds.getHeight(), 0.18f * compBounds.getHeight());
    browseButton->setBounds(addButtonBounds.toNearestInt());

    filenameBox->setBounds(compBounds.toNearestInt());
}

juce::Font CustomLookAndFeel::getComboBoxFont(juce::ComboBox& comboBox)
{
    return getInter().withHeight(comboBox.getHeight() * 0.73f);
}

juce::Label* CustomLookAndFeel::createComboBoxTextBox(juce::ComboBox& comboBox)
{
    using namespace juce;

    auto bounds = comboBox.getLocalBounds().toFloat();
    auto label = new Label();

    label->setFont(getInter().withHeight(bounds.getWidth() * Layout::rotaryTextSize));
    label->setJustificationType(Justification::centredLeft);
    label->setHasFocusOutline(false);
    label->setKeyboardType(TextInputTarget::decimalKeyboard);
    label->setEditable(false, true, false);
    label->getProperties().set(ComponentProps::LABEL_ELLIPSES, true);

    label->onEditorShow = [label]
        {
            auto editor = label->getCurrentTextEditor();
            editor->setJustification(Justification::centredLeft);
            editor->setIndents(0, 0);
            editor->moveCaretToEnd();

            // Some compensation to keep the text in a constant position
            auto editorBounds = editor->getBounds().toFloat();
            float offset = editorBounds.getHeight() * 0.125f;
            editorBounds.translate(-1, 0);
            editorBounds.expand(0, offset);
            editor->setBounds(editorBounds.toNearestInt());
        };

    return label;
}

void CustomLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    auto bounds = box.getLocalBounds().toFloat();
    bounds.removeFromRight(Layout::expandWidth * box.getHeight());

    label.setBounds(bounds.toNearestInt());
    label.setFont(getComboBoxFont(box));
}

void CustomLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown, int buttonX,
                                     int buttonY, int buttonW, int buttonH, juce::ComboBox& box)
{
    using namespace juce;

    auto bounds = box.getLocalBounds().toFloat();
    auto expandBounds = bounds.removeFromRight(Layout::expandWidth * bounds.getHeight()).reduced(0.272f * bounds.getHeight(), 0.321f * bounds.getHeight());

    if (isButtonDown)
        expandBounds.reduce(expandBounds.getHeight() * 0.05f, expandBounds.getHeight() * 0.05f);

    Path path;
    path.startNewSubPath(expandBounds.getTopLeft());
    path.lineTo(expandBounds.getCentreX(), expandBounds.getBottom());
    path.lineTo(expandBounds.getTopRight());

    g.setColour(Colors::DARK);
    g.strokePath(path, PathStrokeType(0.1f * box.getHeight(), PathStrokeType::curved, PathStrokeType::rounded));
}

void CustomLookAndFeel::drawComboBoxTextWhenNothingSelected(juce::Graphics& g, juce::ComboBox& box,
    juce::Label& label)
{
    auto textArea = getLabelBorderSize(label).subtractedFrom(label.getLocalBounds());
    auto font = label.getLookAndFeel().getLabelFont(label);

    g.setColour(findColour(juce::ComboBox::textColourId));
    g.setFont(font);
    g.drawText(box.getTextWhenNothingSelected(), textArea, label.getJustificationType());
}

juce::PopupMenu::Options CustomLookAndFeel::getOptionsForComboBoxPopupMenu(juce::ComboBox& box, juce::Label& label)
{
    return juce::PopupMenu::Options().withTargetComponent(&box)
        .withPreferredPopupDirection(juce::PopupMenu::Options::PopupDirection::downwards)
        .withItemThatMustBeVisible(box.getSelectedId())
        .withInitiallySelectedItem(box.getSelectedId())
        .withMinimumWidth(box.getWidth())
        .withMaximumNumColumns(1)
        .withStandardItemHeight(label.getHeight());
}

juce::Font CustomLookAndFeel::getPopupMenuFont()
{
    return getInter().withHeight(17.f);
}

void CustomLookAndFeel::drawPopupMenuBackground(juce::Graphics& graphics, int width, int height)
{
    graphics.fillAll(Colors::SLATE);
}

void CustomLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area, bool isSeparator,
    bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text,
    const juce::String& shortcutKeyText, const juce::Drawable* icon, const Colour* textColour)
{
    using namespace juce;

    auto bounds = area;

    if (isHighlighted && isActive)
    {
        g.setColour(Colors::DARKER_SLATE);
        g.fillRect(bounds);
    }

    bounds.reduce(jmin(5, area.getWidth() / 20), 0);

    auto font = getPopupMenuFont();
    auto maxFontHeight = float(bounds.getHeight() / 1.3f);
    if (font.getHeight() > maxFontHeight)
        font.setHeight(maxFontHeight);

    g.setColour(Colors::WHITE);
    g.setFont(font);

    auto iconArea = bounds.removeFromLeft(roundToInt(maxFontHeight)).toFloat();
    if (isTicked)
    {
        auto tick = getTickShape(1.0f);
        g.strokePath(tick, PathStrokeType(1.5f, PathStrokeType::curved, PathStrokeType::rounded), tick.getTransformToScaleToFit(iconArea.reduced(iconArea.getWidth() / 5, 0).toFloat(), true));
    }

    bounds.removeFromRight(3);
    g.drawText(text, bounds, Justification::centredLeft, true);
}

void CustomLookAndFeel::drawPopupMenuUpDownArrow(juce::Graphics& g, int width, int height, bool isScrollUpArrow)
{
    using namespace juce;

    auto hw = float(width * 0.5f);
    auto arrowW = float(height * 0.3f);
    auto y1 = float(height) * (isScrollUpArrow ? 0.6f : 0.3f);
    auto y2 = float(height) * (isScrollUpArrow ? 0.3f : 0.6f);

    Path p;
    p.addTriangle(hw - arrowW, y1,
        hw + arrowW, y1,
        hw, y2);

    g.setColour(Colors::DARK);
    g.fillPath(p);
}

juce::Path CustomLookAndFeel::getTickShape(float height)
{
    juce::Path tick;
    tick.startNewSubPath(height, 0.f);
    tick.lineTo(height * 0.4f, height);
    tick.lineTo(0.f, height * 0.5f);
    return tick;
}

//==============================================================================
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

//==============================================================================
juce::Slider::SliderLayout VolumeSliderLookAndFeel::getSliderLayout(juce::Slider& slider)
{
    auto borderSize = slider.getWidth() * 0.020f;

    juce::Slider::SliderLayout layout;
    layout.sliderBounds = slider.getLocalBounds();
    layout.sliderBounds.removeFromLeft(int(std::round(slider.getWidth() * 0.12f)));
    layout.sliderBounds.removeFromRight(int(std::round(borderSize + slider.getWidth() * 0.09f)));

    layout.textBoxBounds = slider.getLocalBounds().removeFromTop(int(std::round(0.21f * slider.getWidth())));
    layout.textBoxBounds.removeFromRight(int(std::round(slider.getWidth() * 0.25f)));

    return layout;
}

juce::Label* VolumeSliderLookAndFeel::createSliderTextBox(juce::Slider& slider)
{
    auto label = CustomLookAndFeel::createSliderTextBox(slider);
    label->setFont(getInriaSansBold().withHeight(slider.getWidth() * 0.225f));
    label->setJustificationType(juce::Justification::bottomRight);
    auto superOnEditorShow = label->onEditorShow;
    label->onEditorShow = [label, superOnEditorShow]
    {
        superOnEditorShow();

        auto editor = label->getCurrentTextEditor();
        auto editorBounds = editor->getBounds();
        editorBounds.translate(3, 0);
        editor->setBounds(editorBounds);
        editor->setJustification(juce::Justification::bottomRight); 
    };
    return label;
}

void VolumeSliderLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float, float, juce::Slider::SliderStyle, juce::Slider& slider)
{
    using namespace juce;

    auto bounds = slider.getBounds().withZeroOrigin().toFloat();

    auto borderSize = bounds.getWidth() * 0.020f;
    bounds = bounds.reduced(borderSize);

    // Get thumb dimensions
    auto thumbWidth = bounds.getWidth() * 0.203f;
    auto thumbHeight = thumbWidth * 0.5f;
    auto thumbRound = bounds.getWidth() * 0.032f;
    auto thumb = Rectangle(sliderPos - thumbWidth / 2.f, bounds.getHeight() - thumbHeight, thumbWidth, thumbHeight);

    // Draw fader shape
    Path sliderShape;
    float bottom = bounds.getHeight() - thumbHeight / 2.f;
    float right = bounds.getRight() - bounds.getWidth() * 0.08f;
    sliderShape.addTriangle(bounds.getX(), bottom, right, bottom, right, bottom - bounds.getWidth() * 0.4f);
    sliderShape = sliderShape.createPathWithRoundedCorners(2 * borderSize);

    g.setColour(Colors::HIGHLIGHT);
    float scale = sliderPos / (right - bounds.getX()) * 0.985f;
    g.fillPath(sliderShape, AffineTransform::scale(scale, scale * 0.98f, bounds.getX(), bottom));
    g.setColour(Colors::DARK);
    g.strokePath(sliderShape, PathStrokeType(borderSize, PathStrokeType::JointStyle::curved));

    // Draw thumb
    g.setColour(Colors::SLATE);
    g.fillRoundedRectangle(thumb, thumbRound);
    g.setColour(Colors::DARK);
    g.drawRoundedRectangle(thumb, thumbRound, 0.016f * bounds.getWidth());
}

//==============================================================================
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

const juce::Font& getInter()
{
    static juce::Font inter{ juce::FontOptions(juce::Typeface::createSystemTypefaceFor(BinaryData::InterRegular_ttf, BinaryData::InterRegular_ttfSize)) };

    return inter;
}

const juce::Font& getInterBold()
{
    static juce::Font interBold{ juce::FontOptions(juce::Typeface::createSystemTypefaceFor(BinaryData::InterBold_ttf, BinaryData::InterBold_ttfSize)) };

    return interBold;
}

juce::Path getOutlineFromSVG(const char* data)
{
    auto xml = juce::parseXML(data);
    jassert(xml != nullptr);
    return juce::Drawable::createFromSVG(*xml)->getOutlineAsPath();
}

/*
  ==============================================================================

    LookAndFeels.h
    Created: 7 Apr 2021 7:59:47pm
    Author:  Hiroyuki Baba

  ==============================================================================
*/

#pragma once

#define LF_DEBUG 0

class ViewZoomSlider : public LookAndFeel_V4
{
public:
    ViewZoomSlider(){}
    
    virtual int getSliderThumbRadius (Slider&) override{
        return 2;
    }
    
    void drawLinearSlider (Graphics& g, int x, int y, int width, int height,
                                           float sliderPos,
                                           float minSliderPos,
                                           float maxSliderPos,
                                           const Slider::SliderStyle style, Slider& slider) override
    {
        Point<float> startPoint (slider.isHorizontal() ? (float) x : (float) x + (float) width * 0.5f,
                                 slider.isHorizontal() ? (float) y + (float) height * 0.5f : (float) (height + y));

        Point<float> endPoint (slider.isHorizontal() ? (float) (width + x) : startPoint.x,
                               slider.isHorizontal() ? startPoint.y : (float) y);

        Path backgroundTrack;
        backgroundTrack.startNewSubPath (startPoint);
        backgroundTrack.lineTo (endPoint);
        g.setColour (slider.findColour (Slider::backgroundColourId));
        g.strokePath (backgroundTrack, { 3, PathStrokeType::curved, PathStrokeType::rounded });

        Path valueTrack;
        Point<float> valuePoint;

        {
            auto kx = slider.isHorizontal() ? sliderPos : ((float) x + (float) width * 0.5f);
            auto ky = slider.isHorizontal() ? ((float) y + (float) height * 0.5f) : sliderPos;

            valuePoint = { kx, ky };
        }

        {
            g.setColour (slider.findColour (Slider::thumbColourId));
            g.fillRect (Rectangle<float> (3, 10).withCentre (valuePoint));
        }
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ViewZoomSlider)
};

class ParamController : public LookAndFeel_V4
{
public:
    ParamController(){}
    
    void drawLinearSlider (Graphics& g, int x, int y, int width, int height,
                                           float sliderPos,
                                           float minSliderPos,
                                           float maxSliderPos,
                                           const Slider::SliderStyle style, Slider& slider) override
    {
        Point<float> startPoint (slider.isHorizontal() ? (float) x : (float) x + (float) width * 0.5f,
                                 slider.isHorizontal() ? (float) y + (float) height * 0.5f : (float) (height + y));

        Point<float> endPoint (slider.isHorizontal() ? (float) (width + x) : startPoint.x,
                               slider.isHorizontal() ? startPoint.y : (float) y);

        Path backgroundTrack;
        backgroundTrack.startNewSubPath (startPoint);
        backgroundTrack.lineTo (endPoint);
        g.setColour (slider.findColour (Slider::backgroundColourId));
        g.strokePath (backgroundTrack, { 3, PathStrokeType::curved, PathStrokeType::rounded });

        Path valueTrack;
        Point<float> valuePoint;

        {
            auto kx = slider.isHorizontal() ? sliderPos : ((float) x + (float) width * 0.5f);
            auto ky = slider.isHorizontal() ? ((float) y + (float) height * 0.5f) : sliderPos;

            valuePoint = { kx, ky };
        }

        {
            g.setColour (slider.findColour (Slider::thumbColourId));
            g.fillRect (Rectangle<float> (3, 10).withCentre (valuePoint));
        }
    }
    
    void drawRotarySlider (Graphics& g, int x, int y, int width, int height, float sliderPos,
                        const float rotaryStartAngle, const float rotaryEndAngle, Slider& slider) override
    {
        // slider pos seems to be 0 -> t
        double minV = slider.getMinimum();
        double maxV = slider.getMaximum();
        double intV = slider.getInterval();
        int numInterval = round((maxV - minV)/intV);
        int numIndices = numInterval + 1;
        
        auto outline = slider.findColour (Slider::rotarySliderOutlineColourId);
        //auto fill    = slider.findColour (Slider::rotarySliderFillColourId);

        auto bounds = Rectangle<int> (x, y, width, height).toFloat().reduced (0);

        auto radius = jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto arcRadius = radius;
        
#if LF_DEBUG
        g.setColour(Colours::green);
        g.fillRect(slider.getLocalBounds());
        
        g.setColour(Colours::blue);
        g.fillRect(x,y,width,height);
#endif
        
        if( numIndices > 16 ){
            // Contiguous version
            Path backgroundArc;
            backgroundArc.addCentredArc (bounds.getCentreX(),
                                         bounds.getCentreY(),
                                         arcRadius,
                                         arcRadius,
                                         0.0f,
                                         rotaryStartAngle,
                                         rotaryEndAngle,
                                         true);

            g.setColour (outline);
            g.strokePath (backgroundArc, PathStrokeType (3, PathStrokeType::curved, PathStrokeType::rounded));
        } else {
            // Descrete version
            for(int i = 0; i < numIndices; ++i){
                Path indice;
                double angle = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle)/numInterval*i;
                indice.startNewSubPath(bounds.getCentreX() + arcRadius*0.8 * std::cos (angle - MathConstants<float>::halfPi),
                              bounds.getCentreY() + arcRadius*0.8 * std::sin (angle - MathConstants<float>::halfPi));
                indice.lineTo(bounds.getCentreX() + arcRadius * std::cos (angle - MathConstants<float>::halfPi),
                              bounds.getCentreY() + arcRadius * std::sin (angle - MathConstants<float>::halfPi));
                g.setColour (outline);
                g.strokePath (indice, PathStrokeType (2, PathStrokeType::curved, PathStrokeType::rounded));

            }
        }


        {
            Path thumb;
            
            thumb.startNewSubPath(bounds.getCentreX(), bounds.getCentreY());
            thumb.lineTo(bounds.getCentreX() + arcRadius*0.8 * std::cos (toAngle - MathConstants<float>::halfPi),
                          bounds.getCentreY() + arcRadius*0.8 * std::sin (toAngle - MathConstants<float>::halfPi));
            g.setColour (slider.findColour (Slider::thumbColourId));
            g.strokePath (thumb, PathStrokeType (3, PathStrokeType::curved, PathStrokeType::rounded));

        }
    }
    
    void drawLabel (Graphics& g, Label& label) override
    {
        g.fillAll (label.findColour (Label::backgroundColourId));

        if (! label.isBeingEdited())
        {
            auto alpha = label.isEnabled() ? 1.0f : 0.5f;
            const Font font (getLabelFont (label));

            g.setColour (label.findColour (Label::textColourId).withMultipliedAlpha (alpha));
            g.setFont (font);

            auto textArea = getLabelBorderSize (label).subtractedFrom (label.getLocalBounds());

            g.drawFittedText (label.getText(), textArea, label.getJustificationType(),
                              jmax (1, (int) ((float) textArea.getHeight() / font.getHeight())),
                              label.getMinimumHorizontalScale());

            g.setColour (label.findColour (Label::outlineColourId).withMultipliedAlpha (alpha));
        }
        else if (label.isEnabled())
        {
            g.setColour (label.findColour (Label::outlineColourId));
        }
    }
    
    virtual juce::Slider::SliderLayout getSliderLayout (Slider& slider) override{
        // 1. compute the actually visible textBox size from the slider textBox size and some additional constraints

        auto textBoxPos = slider.getTextBoxPosition();


        auto localBounds = slider.getLocalBounds();

        auto textBoxWidth  = slider.getTextBoxWidth();
        auto textBoxHeight = slider.getTextBoxHeight();

        Slider::SliderLayout layout;

        // 2. set the textBox bounds

        if (textBoxPos != Slider::NoTextBox)
        {
            if (slider.isBar())
            {
                layout.textBoxBounds = localBounds;
            }
            else
            {
                layout.textBoxBounds.setWidth (textBoxWidth);
                layout.textBoxBounds.setHeight (textBoxHeight);

                if (textBoxPos == Slider::TextBoxLeft)           layout.textBoxBounds.setX (0);
                else if (textBoxPos == Slider::TextBoxRight)     layout.textBoxBounds.setX (localBounds.getWidth() - textBoxWidth);
                else /* above or below -> centre horizontally */ layout.textBoxBounds.setX ((localBounds.getWidth() - textBoxWidth) / 2);

                if (textBoxPos == Slider::TextBoxAbove)          layout.textBoxBounds.setY (0);
                else if (textBoxPos == Slider::TextBoxBelow)     layout.textBoxBounds.setY (localBounds.getHeight() - textBoxHeight);
                else /* left or right -> centre vertically */    layout.textBoxBounds.setY ((localBounds.getHeight() - textBoxHeight) / 2);
            }
        }

        // 3. set the slider bounds

        layout.sliderBounds = localBounds;

        if (slider.isBar())
        {
            layout.sliderBounds.reduce (1, 1);   // bar border
        }
        else
        {
            if (textBoxPos == Slider::TextBoxLeft)       layout.sliderBounds.removeFromLeft (textBoxWidth);
            else if (textBoxPos == Slider::TextBoxRight) layout.sliderBounds.removeFromRight (textBoxWidth);
            else if (textBoxPos == Slider::TextBoxAbove) layout.sliderBounds.removeFromTop (textBoxHeight);
            else if (textBoxPos == Slider::TextBoxBelow) layout.sliderBounds.removeFromBottom (textBoxHeight);

            const int thumbIndent = getSliderThumbRadius (slider);

            if (slider.isHorizontal())    layout.sliderBounds.reduce (thumbIndent, 0);
            else if (slider.isVertical()) layout.sliderBounds.reduce (0, thumbIndent);
        }

        return layout;
    }
    
    virtual int getSliderThumbRadius (Slider&) override{
        return 0;
    }
    
    virtual Label* createSliderTextBox (Slider& slider) override
    {
        auto* l = LookAndFeel_V4::createSliderTextBox (slider);
        
        // Set font
        l->setFont(Font(9));

        return l;
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamController)
};

/*
  ==============================================================================

    CustomComponents.h
    Created: 8 Apr 2021 8:30:06am
    Author:  Hiroyuki Baba

  ==============================================================================
*/

#pragma once

#include "LookAndFeels.h"

struct IIncludeSlider
{
    virtual ~IIncludeSlider(){}
    virtual Slider* getSlider() = 0;
};
struct IIncludeLabel
{
    virtual ~IIncludeLabel(){}
    virtual Label* getLabel() = 0;
};
struct IIncludeButton
{
    virtual ~IIncludeButton(){}
    virtual Button* getButton() = 0;
};

class MarginPaddingSystem
{
public:
    struct Margin{
        int top;
        int right;
        int bottom;
        int left;
    };
    struct Padding{
        int top;
        int right;
        int bottom;
        int left;
    };
protected:
    Margin _margin;
    Padding _padding;
public:
    MarginPaddingSystem() : _margin({0,0,0,0}), _padding({0,0,0,0}) {}
    MarginPaddingSystem(const Margin& m, const Padding& p) : _margin(m), _padding(p) {}
    MarginPaddingSystem(int m, int p) : _margin({m,m,m,m}), _padding({p,p,p,p}) {}
    MarginPaddingSystem(int mv, int mh, int pv, int ph) : _margin({mv,mh,mv,mh}), _padding({pv,ph,pv,ph}) {}
    
    /**
     * Get rectangle excluding margin
     */
    Rectangle<int> getBorderBox(const Rectangle<int>& outerBox) const {
        Rectangle<int> ret;
        ret.setX(outerBox.getX() + _margin.left);
        ret.setY(outerBox.getY() + _margin.top);
        ret.setWidth(outerBox.getWidth() - _margin.left - _margin.right);
        ret.setHeight(outerBox.getHeight() - _margin.top - _margin.bottom);
        return ret;
    }
    /**
     * Get rectangle excluding margin
     */
    Rectangle<int> getContentBox(const Rectangle<int>& outerBox) const {
        Rectangle<int> borderBox = getBorderBox(outerBox);
        Rectangle<int> ret;
        ret.setX(borderBox.getX() + _padding.left);
        ret.setY(borderBox.getY() + _padding.top);
        ret.setWidth(borderBox.getWidth() - _padding.left - _padding.right);
        ret.setHeight(borderBox.getHeight() - _padding.top - _padding.bottom);
        return ret;
    }
    
    Line<int> leftBorder(const Rectangle<int>& outerBox){
        return Line<int>(outerBox.getX() + _margin.left, outerBox.getY() + _margin.top,
                         outerBox.getX() + _margin.left, outerBox.getBottom() - _margin.bottom);
    }
    Line<int> rightBorder(const Rectangle<int>& outerBox){
        return Line<int>(outerBox.getRight() - _margin.right, outerBox.getY() + _margin.top,
                         outerBox.getRight() - _margin.right, outerBox.getBottom() - _margin.bottom);
    }
    Line<int> topBorder(const Rectangle<int>& outerBox){
        return Line<int>(outerBox.getX() + _margin.left, outerBox.getY() + _margin.top,
                         outerBox.getRight() - _margin.left, outerBox.getY() + _margin.top);
    }
    Line<int> bottomBorder(const Rectangle<int>& outerBox){
        return Line<int>(outerBox.getX() + _margin.left, outerBox.getBottom() - _margin.bottom,
                         outerBox.getRight() - _margin.left, outerBox.getBottom() - _margin.bottom);
    }
};

class Arrows : public Component{
    bool _isHorizontal;
public:
    Arrows(bool isHorizontal) : _isHorizontal(isHorizontal){}
    virtual void paint(Graphics& g) override{
        Rectangle<int> lborg = getLocalBounds();
        int len = jmin(lborg.getWidth(), lborg.getHeight());
        Rectangle<int> lb(len, len);
        lb.setCentre(lborg.getCentre());

        g.setColour(Colours::white);

        if(_isHorizontal){
            Line<float> l(lb.getCentreX(),lb.getCentreY(),lb.getX(),lb.getCentreY());
            Line<float> r(lb.getCentreX(),lb.getCentreY(),lb.getRight(),lb.getCentreY());
            g.drawArrow(l, 1, lb.getHeight()/3, lb.getHeight()/3);
            g.drawArrow(r, 1, lb.getHeight()/3, lb.getHeight()/3);
        }else{
            Line<float> u(lb.getCentreX(),lb.getCentreY(),lb.getCentreX(),lb.getY());
            Line<float> d(lb.getCentreX(),lb.getCentreY(),lb.getCentreX(),lb.getBottom());
            g.drawArrow(u, 1, lb.getHeight()/3, lb.getHeight()/3);
            g.drawArrow(d, 1, lb.getHeight()/3, lb.getHeight()/3);
        }
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Arrows)
};

class RotarySliderTypeA : public Component, public IIncludeSlider {
    SafePointer<Label> _title;
    SafePointer<Slider> _slider;
    ParamController _paramControllerLF;
    float regions_H[3] = {0.25,0.5,0.25}; // Title, Nob, Value
    MarginPaddingSystem _mps{2,2};
    bool border = false;
public:
    RotarySliderTypeA(const String& title){
        _title = new Label();
        _title->setText(title, NotificationType::dontSendNotification);
        _title->setJustificationType(Justification::centred);
        _title->setFont(Font(InternalParam::nobTitleFontSize));
        _slider = new Slider(Slider::SliderStyle::RotaryVerticalDrag,
                             Slider::TextEntryBoxPosition::TextBoxBelow);
        _slider->setLookAndFeel(&_paramControllerLF);
        
        addAndMakeVisible(_title);
        addAndMakeVisible(_slider);
    }
    virtual ~RotarySliderTypeA(){
        removeAllChildren();
        _title.deleteAndZero();
        _slider->setLookAndFeel(nullptr);
        _slider.deleteAndZero();
    }
    
    virtual void paint(Graphics& g) override{
        if(border){
            Rectangle<int> olb = getLocalBounds();
            Rectangle<int> blb = _mps.getBorderBox(olb);
            
            g.setColour(Colours::grey);
            Path border;
            border.addRoundedRectangle(blb.getX(), blb.getY(), blb.getWidth(), blb.getHeight(), 5);
            g.strokePath(border, { 1, PathStrokeType::curved, PathStrokeType::rounded });
        }
    }
    
    virtual void resized() override{
        Rectangle<int> olb = getLocalBounds();
        Rectangle<int> ilb = _mps.getContentBox(olb);
        _title->setBounds(ilb.removeFromTop(ilb.getHeight()*regions_H[0]));
        _slider->setBounds(ilb);
        _slider->setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxBelow, false, ilb.getWidth(), ilb.getHeight()*regions_H[2]);
    }
    
    virtual Slider* getSlider() override { return _slider.getComponent(); }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RotarySliderTypeA)
};

class ZoomSlider : public Component, public IIncludeSlider
{
    SafePointer<Arrows> _arrows;
    SafePointer<Slider> _slider;
    ViewZoomSlider _paramControllerLF;
    MarginPaddingSystem _mps{18,4,2,2};
    float regions_H[3] = {0.2,0.8}; // Title, Nob, Value
    bool border = true;
    
public:
    ZoomSlider(bool isHorizontal) {
        _arrows = new Arrows(isHorizontal);
        _slider = new Slider(Slider::SliderStyle::LinearHorizontal,
                             Slider::TextEntryBoxPosition::NoTextBox);
        _slider->setLookAndFeel(&_paramControllerLF);
        
        addAndMakeVisible(_arrows);
        addAndMakeVisible(_slider);
    }
    virtual ~ZoomSlider(){
        removeAllChildren();
        _slider->setLookAndFeel(nullptr);
        _slider.deleteAndZero();
        _arrows.deleteAndZero();
    }
    
    virtual void paint(Graphics& g) override{
        if(border){
            Rectangle<int> olb = getLocalBounds();
            Rectangle<int> blb = _mps.getBorderBox(olb);
            g.setColour(Colours::grey.darker().darker().darker());
            Path border;
            border.addRoundedRectangle(blb.getX(), blb.getY(), blb.getWidth(), blb.getHeight(), 5);
            g.strokePath(border, { 1, PathStrokeType::curved, PathStrokeType::rounded });
        }
    }
    
    virtual void resized() override{
        Rectangle<int> olb = getLocalBounds();
        Rectangle<int> ilb = _mps.getContentBox(olb);
        _arrows->setBounds(ilb.removeFromLeft(ilb.getWidth()*regions_H[0]));
        _slider->setBounds(ilb.reduced(2));
    }
    
    virtual Slider* getSlider() override { return _slider.getComponent(); }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZoomSlider)
};

class ImageToggleButton : public ToggleButton{
    Image _offImage;
    Image _onImage;
public:
    ImageToggleButton(const Image& offImage, const Image& onImage) : _offImage(offImage), _onImage(onImage) {}

    virtual void paintButton (Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        Rectangle<float> lb = getLocalBounds().toFloat();
        float len = jmin(lb.getWidth(), lb.getHeight());
        Rectangle<float> br(len, len);
        br.setCentre(lb.getCentre());
        if(getToggleState()){
            g.drawImage(_onImage, br);
        }else{
            g.drawImage(_offImage, br);
        }
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImageToggleButton)
};

class PlayControl : public Component, public IIncludeLabel
{
    SafePointer<ImageToggleButton> _playBtn;
    SafePointer<ImageToggleButton> _recBtn;
    SafePointer<Label> _tempoLabel;
    SafePointer<Label> _bpm;
    
    MarginPaddingSystem _mps{14,4,2,2};
public:
    PlayControl(){
        Image playImage = juce::ImageCache::getFromMemory (BinaryData::play_png, BinaryData::play_pngSize);
        Image stopImage = juce::ImageCache::getFromMemory (BinaryData::stop_png, BinaryData::stop_pngSize);
        _playBtn = new ImageToggleButton(playImage, stopImage);
        
        Image recoffImage = juce::ImageCache::getFromMemory (BinaryData::recoff_png, BinaryData::recoff_pngSize);
        Image reconImage  = juce::ImageCache::getFromMemory (BinaryData::recon_png,  BinaryData::recon_pngSize);
        _recBtn = new ImageToggleButton(recoffImage, reconImage);

        _tempoLabel = new Label();
        _tempoLabel->setText("100.0", NotificationType::dontSendNotification);
        _tempoLabel->setJustificationType(Justification::centred);
        _tempoLabel->setEditable(true);
        
        _bpm = new Label();
        _bpm->setText("bpm", NotificationType::dontSendNotification);
        _bpm->setJustificationType(Justification::centred);
        _bpm->setEditable(false);
        
        addAndMakeVisible(_playBtn);
        addAndMakeVisible(_recBtn);
        addAndMakeVisible(_tempoLabel);
        addAndMakeVisible(_bpm);
        
    }
    virtual ~PlayControl(){
        _playBtn.deleteAndZero();
        _recBtn.deleteAndZero();
        _tempoLabel.deleteAndZero();
        _bpm.deleteAndZero();
    }
    virtual void resized() override {
        Rectangle<int> lb = getLocalBounds();
        Rectangle<int> contentBox = _mps.getContentBox(lb);
        int w = contentBox.getWidth();
        Rectangle<int> playBtnRect = contentBox.removeFromLeft(w*0.2);
        Rectangle<int> recBtnRect  = contentBox.removeFromLeft(w*0.2);
        Rectangle<int> bpmValueRect = removeFromLeftRatio(contentBox, 0.5);
        
        _playBtn->setBounds(playBtnRect.reduced(2));
        _recBtn->setBounds(recBtnRect.reduced(2));
        _tempoLabel->setBounds(bpmValueRect.reduced(2));
        _bpm->setBounds(contentBox.reduced(2));
    }
    
    virtual void paint(Graphics& g) override {
        Rectangle<int> olb = getLocalBounds();
        Rectangle<int> blb = _mps.getBorderBox(olb);
        g.setColour(Colours::grey.darker().darker().darker());
        Path border;
        border.addRoundedRectangle(blb.getX(), blb.getY(), blb.getWidth(), blb.getHeight(), 5);
        g.strokePath(border, { 1, PathStrokeType::curved, PathStrokeType::rounded });
    }

    virtual Label* getLabel() override { return _tempoLabel; }
    virtual Button* getPlayButton() { return _playBtn; }
    virtual Button* getRecButton(){ return _recBtn; }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlayControl)
};

class HBox : public Component
{
public:
    enum Direction{
        NONE = 0x0,
        LEFT = 0x1,
        TOP = 0x2,
        RIGHT = 0x4,
        BOTTOM = 0x8,
    };
    
private:
    String _title = "";
    
    struct Entry{
        SafePointer<Component> c;
    };
    Array< Entry > _items;
    MarginPaddingSystem _mps; //{10,4,2,2};
    
    // Border
    int _borderDir = (int)NONE;
    Colour _borderColour;
    int _borderRadius = 0;
    
public:
    HBox(const MarginPaddingSystem& mps) : _mps(mps) {}
    HBox(SafePointer<Component> c, const MarginPaddingSystem& mps) : _mps(mps) { addItem(c); }
    HBox(SafePointer<Component> c, const MarginPaddingSystem& mps, int borderDirection, Colour borderColour) : _mps(mps) { addItem(c); setBorder(borderDirection, borderColour); }
    virtual ~HBox(){
        for(int i = 0; i < _items.size(); ++i){
            _items[i].c.deleteAndZero();
        }
    }
    void setTitle(const String& s){ _title = s; }
    void setBorder(int borderDirection, Colour borderColour, int borderRadius=0){
        _borderDir = borderDirection;
        _borderColour = borderColour;
        _borderRadius = borderRadius;
    }
    void addItem(SafePointer<Component> c) {
        _items.add({c});
        addAndMakeVisible(c);
    }
    Rectangle<int> getBBox(int i){
        Rectangle<int> lb = _mps.getContentBox(getLocalBounds());
        return {(int)(lb.getX() + lb.getWidth()/(float)_items.size()*i), lb.getY(), int(lb.getWidth()/(float)_items.size()), lb.getHeight()};
    }
    virtual void resized() override {
        for(int i = 0; i < _items.size(); ++i){
            Component* c = _items[i].c;
            c->setBounds(getBBox(i));
        }
    }
    virtual void paint(Graphics& g) override{
        if(false){
            // debug
            Colour cols[] = {Colours::blue, Colours::green, Colours::red, Colours::violet};
            for(int i = 0; i<_items.size(); ++i){
                g.setColour(cols[i]);
                g.fillRect(getBBox(i));
            }
        }
        if(_borderDir == (LEFT&RIGHT&TOP&BOTTOM)){
            // Only this case, radius is valid
            Rectangle<int> bb = _mps.getBorderBox(getLocalBounds());
            g.setColour(_borderColour);
            g.drawRoundedRectangle(bb.toFloat(), (float)_borderRadius, 1.0);
        }else{
            Rectangle<int> lb = getLocalBounds();
            if(_borderDir & LEFT){
                Line<int> ll = _mps.leftBorder(lb);
                g.setColour(_borderColour);
                g.drawLine(ll.toFloat());
            }
            if(_borderDir & TOP){
                Line<int> ll = _mps.topBorder(lb);
                g.setColour(_borderColour);
                g.drawLine(ll.toFloat());
            }
            if(_borderDir & RIGHT){
                Line<int> ll = _mps.rightBorder(lb);
                g.setColour(_borderColour);
                g.drawLine(ll.toFloat());
            }
            if(_borderDir & BOTTOM){
                Line<int> ll = _mps.bottomBorder(lb);
                g.setColour(_borderColour);
                g.drawLine(ll.toFloat());
            }
            if(_title!=""){
                g.setColour(ColourParam::labelColour);
                g.setFont(Font(10));
                g.drawText (_title, _mps.getBorderBox(lb).reduced(2, 2),
                            Justification::topLeft, true);
            }
        }
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HBox)
};


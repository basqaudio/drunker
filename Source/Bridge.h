/*
  ==============================================================================

    Bridge.h
    Created: 11 Apr 2021 2:59:36pm
    Author:  Hiroyuki Baba

  ==============================================================================
*/

#pragma once
#include "CustomComponents.h"

class SliderBridge
{
    Slider* _slider;
    bool _externalSlider = true;
    AudioParameterFloat* _param;
    std::unique_ptr<SliderParameterAttachment> _delayBinder;
public:
    SliderBridge(
        AudioParameterFloat* param,
        Slider::SliderStyle sliderStyle,
        Slider::TextEntryBoxPosition textPos) : SliderBridge(param, new Slider(sliderStyle, textPos))
    {
        _externalSlider = false;
    }
    SliderBridge(
        AudioParameterFloat* param, Slider* slider) : _slider(slider), _param(param) {
        //_slider->setTextBoxStyle (textPos,
        //                      false,
        //                      50, 10);
        _delayBinder.reset(new SliderParameterAttachment(*_param, *_slider));
    }
    
    virtual ~SliderBridge() {
        // As Attachment objects holds slider, attachment should be deleted firstly.
        // Hence, actually no need to use unique_ptr
        _delayBinder.reset();
        if(!_externalSlider) delete _slider;
        
    }
    Slider* getSlider(){ return _slider; }
};

class IButtonBridge {
public:
    virtual ~IButtonBridge(){}
    virtual Button* getButton() = 0;
};

class ToggleButtonBridge : public IButtonBridge, public AudioProcessorParameter::Listener
{
    juce::Component::SafePointer<Button> _btn;
    AudioParameterBool* _btnParam;
    std::function<void(bool)> _cb;
    std::unique_ptr<ButtonParameterAttachment> _binder;
public:
    ToggleButtonBridge(
        AudioParameterBool* btnParam,
        const String& btnName,
        std::function<void(bool)> cb) : _btnParam(btnParam), _cb(cb) {
        
        _btn = new ToggleButton(btnName);
        _binder.reset(new ButtonParameterAttachment(*_btnParam, *_btn));
        _btnParam->addListener(this);
    }
    virtual ~ToggleButtonBridge() {
        _binder.reset(); // this should done firstly before deleting button component.
        _btn.deleteAndZero();
        _btnParam->removeListener(this);
    }
    virtual void parameterValueChanged (int parameterIndex, float newValue) override {
        _cb(_btnParam->get());
    }
    virtual void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {}
    virtual Button* getButton() override { return _btn.getComponent(); }
};

class ImageToggleButtonBridge : public IButtonBridge, public AudioProcessorParameter::Listener
{
    Button* _btn;
    AudioParameterBool* _btnParam;
    bool _externalBtn = true;

    std::function<void(bool)> _cb;
    std::unique_ptr<ButtonParameterAttachment> _binder;
public:
    ImageToggleButtonBridge(
        AudioParameterBool* btnParam,
        const Image& offImage, const Image& onImage,
        std::function<void(bool)> cb) : ImageToggleButtonBridge(btnParam, new ImageToggleButton(offImage, onImage), cb)
    {
        _externalBtn = false;
    }
    ImageToggleButtonBridge(
        AudioParameterBool* btnParam,
        ImageToggleButton* btn,
        std::function<void(bool)> cb) : _btn(btn), _btnParam(btnParam),  _cb(cb) {
        _binder.reset(new ButtonParameterAttachment(*_btnParam, *_btn));
        _btnParam->addListener(this);
    }
    virtual ~ImageToggleButtonBridge() {
        _binder.reset(); // this should done firstly before deleting button component.
        if(!_externalBtn) delete _btn;
        _btnParam->removeListener(this);
    }
    virtual void parameterValueChanged (int parameterIndex, float newValue) override {
        if(_cb) _cb(_btnParam->get());
    }
    virtual void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {}
    virtual Button* getButton() override { return _btn; }
};

class FloatLabelBridge : public AudioProcessorParameter::Listener, public Label::Listener
{
    Label* _btn;
    AudioParameterFloat* _btnParam;
public:

    FloatLabelBridge(
        AudioParameterFloat* btnParam,
        Label* btn) : _btn(btn), _btnParam(btnParam) {
        _btnParam->addListener(this);
        _btn->addListener(this);
        // Initial value set
        _btnParam->setValueNotifyingHost(_btnParam->convertTo0to1(_btnParam->get())); // Re-set the current value
    }
    virtual ~FloatLabelBridge() {
        _btnParam->removeListener(this);
        _btn->removeListener(this);
    }
    // Notification direction
    // User -> Label -> Parameter -> Other
    //
    virtual void parameterValueChanged (int parameterIndex, float newValue) override {
        float bpm = _btnParam->convertFrom0to1(newValue);
        _btn->setText(String(bpm), NotificationType::dontSendNotification); // notification from GUI is only due to user input to eliminate the infinite loop of notification
    }
    virtual void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {}
    virtual void labelTextChanged (Label* labelThatHasChanged) override { // Shall be due to user input
        String s = _btn->getText();
        _btnParam->setValueNotifyingHost(_btnParam->convertTo0to1( s.getFloatValue()));
    }
};

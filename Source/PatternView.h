/*
  ==============================================================================

    PatternView.h
    Created: 9 Mar 2021 1:35:24pm
    Author:  Hiroyuki Baba

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Drummer.h"
#include "Helper.h"
#include "Common.h"

//==============================================================================

#if 0

class SequenceHaderView : public juce::Component
{
    SequenceDrummer::Sequence& _seq;
    std::unique_ptr<SliderBridge> _delayBridge;
    std::unique_ptr<SliderBridge> _gridSelector;
    
    ParameterManager& _pm;
    
public:
    SequenceHaderView(int trackId, ParameterManager& pm, SequenceDrummer::Sequence& seq) : _seq(seq), _pm(pm)
    {
        
        {
            std::function<void(float)> cb = [this](float newv){
                this->_seq._delay = (Duration)(newv*Durations::BEAT16);
                this->repaint();
            };
            
            AudioParameterFloat* p;
            pm.addParam( p = new AudioParameterFloat("SLIDER","slider", {-1.0, 1.0, 0.05}, (float)_seq._delay/Durations::BEAT16), pm.SEQ_TRACK_PARAM(trackId, 0));
            
            _delayBridge.reset(new SliderBridge(p,
                Slider::SliderStyle::RotaryVerticalDrag,
                Slider::TextEntryBoxPosition::TextBoxAbove));
            pm.addCallback(pm.SEQ_TRACK_PARAM(trackId, 0), cb);
            
            
            addAndMakeVisible(_delayBridge->getSlider());
        }
        
        {
            //1,2,3,4,5,6,7,8,9
            NormalisableRange<float> nr(1,11,1);
            
            AudioParameterFloat* p;
            pm.addParam( p = new AudioParameterFloat("GRID","gridSel", nr, Durations::BEAT4/_seq._gridIntervalDuration), pm.SEQ_TRACK_PARAM(trackId, 1));
            _gridSelector.reset(new SliderBridge(p,
                Slider::SliderStyle::RotaryVerticalDrag,
                Slider::TextEntryBoxPosition::TextBoxAbove));
            
            addAndMakeVisible(_gridSelector->getSlider());
        }
        
        setComponentSizes();
    }

    ~SequenceHaderView() override
    {
    }
    
    SequenceDrummer::Sequence& getPattern(){
        return _seq;
    }
    
    Rectangle<int> getRefArea(){
        Rectangle<int> lb = getLocalBounds();
        lb.setLeft(InternalParam::_controlAreaWidth + InternalParam::_hoffset);
        lb.setRight(lb.getRight() - InternalParam::_controlAreaWidth - InternalParam::_hoffset);
        lb.setTop(InternalParam::_voffset);
        lb.setBottom(lb.getBottom() - InternalParam::_voffset);
        Rectangle<int> ref(lb.getX(), lb.getY(), InternalParam::_widthForBEAT4, lb.getHeight());
        return ref;
    }

    void paint (juce::Graphics& g) override
    {
        Rectangle<int> lb = getLocalBounds();
        g.fillAll (ColourParam::seqHeaderBG);   // clear the background
        g.setColour(ColourParam::seqHeaderBG.darker());
        g.drawLine(lb.getX(), lb.getHeight(), lb.getWidth(), lb.getHeight(), 1);
        
    }
    
    Rectangle<int> getComponentSize(){
        Rectangle<int> r;
        r.setWidth(InternalParam::_controlAreaWidth);
        r.setHeight(InternalParam::_trackHeight);
        return r;
    }

    // Note : We do not use resized callback (bottom->up style sizing policy)
    void setComponentSizes()
    {
        // This method is where you should set the bounds of any child
        // components that your component contains..
        // Horizontal
        if(true){
            Rectangle<int> lb;
            lb.setWidth(InternalParam::_controlAreaWidth/2);
            lb.setHeight(InternalParam::_trackHeight );
            _delayBridge->getSlider()->setBounds(lb.reduced(InternalParam::baseUImargin));
            lb.setX(lb.getWidth());
            _gridSelector->getSlider()->setBounds(lb.reduced(InternalParam::baseUImargin));
        }else{
            Rectangle<int> lb;
            lb.setWidth(InternalParam::_controlAreaWidth);
            lb.setHeight(InternalParam::_trackHeight/2 );
            _delayBridge->getSlider()->setBounds(lb.reduced(InternalParam::baseUImargin));
            lb.setY(lb.getHeight());
            _gridSelector->getSlider()->setBounds(lb.reduced(InternalParam::baseUImargin));
        }
    }
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceHaderView)
};


class SequenceView  : public juce::Component, public Timer
{
    SequenceDrummer::Sequence& _seq;
    ViewConverter* _conv;
    //std::unique_ptr<ViewConverter> _conv;
    //Drummer::Duration _gridIntervalDuration;
    
    ParameterManager& _pm;
    SequenceDrummer::SequenceEntry* _selSeq;
    std::list<SequenceDrummer::SequenceEntry>::iterator _selSeqItr; // This is valid only when _selSeq != nullptr.
    
public:
    SequenceView(int trackId, ParameterManager& pm, SequenceDrummer::Sequence& seq, ViewConverter* conv) : _seq(seq), _conv(conv), _pm(pm), _selSeq(nullptr)
    {
        setName("SequenceView");
        
        startTimer(10000);

        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        //_conv.reset( new ViewConverter() );
        //_conv->xrange(0, Drummer::BEAT4);
        //_conv->yrange(0, 127);
        
        pm.addCallback(pm.SEQ_TRACK_PARAM(trackId, 0), [this](float v){ this->repaint(); });
        
        std::function<void(float)> cb = [this](float newv){
            LOG("GRID PARAM CHANGED", newv);
            _seq._gridIntervalDuration = (Duration)(Durations::BEAT4/newv);
            this->repaint();
        };
        pm.addCallback(pm.SEQ_TRACK_PARAM(trackId, 1), cb);
        
        // At this moment, parameter LOCK** shall be added
        _pm.addCallback(ParameterManager::LOCK_OFF_GRID_PARAM, [this](bool v){ this->repaint(); });
        
    }
    
    void LogAllTheComponentSizes() {
        Component* c = this;
        LOG("Stat log comp size");
        while(c != nullptr){
            Rectangle<int> lb = c->getLocalBounds();
            Rectangle<int> lbinp = c->getBoundsInParent();
            LOG("LOG comp sizes : ", String(typeid(c).name()), c->getName(), lb.toString(), lbinp.toString());
            c = c->getParentComponent();
            //getParentComponent();
        }
        LOG("End log comp size");
    }
    
    virtual void timerCallback() override
    {
        LogAllTheComponentSizes();
    }

    ~SequenceView() override
    {
    }
    
    SequenceDrummer::Sequence& getPattern(){
        return _seq;
    }


    void paint (juce::Graphics& g) override
    {
#ifdef DEBUG
        //LOG("paint called in SequenceView", g.getClipBounds().toString());
#endif
        bool lockOffGrid = _pm.getBoolParam(ParameterManager::LOCK_OFF_GRID_PARAM)->get();
        
        Rectangle<int> lb = getLocalBounds();
        //g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));   // clear the background
        g.fillAll(ColourParam::seqViewBG);
        g.setColour(ColourParam::seqViewBG.darker());
        g.drawLine(lb.getX(), lb.getHeight(), lb.getWidth(), lb.getHeight(), 1);
        
        //Rectangle<int> ref = getRefArea();
        
        int nGrid = static_cast<int>(_seq._length / _seq._gridIntervalDuration); // Limit : this hould be integer
        
        Rectangle<int> ptnRegion = _conv->getRefRect();
        ptnRegion.setWidth( _conv->convToScreenWidth(_seq._length));
        g.setColour(Colours::darkgrey);
        g.fillRect(ptnRegion);
        
 
        for(int gi = 0; gi < nGrid; ++gi){
            int gridPos = static_cast<int>(_seq._gridIntervalDuration*gi);
            int left   = _conv->convToScreenX(gridPos);
            int bottom = _conv->convToScreenY(0);
            int top_limit = _conv->convToScreenY(127);
            
            g.setColour(Colours::grey);
            if(gridPos%Durations::BEAT4==0){
                g.fillRect (Rectangle<float> ((float) left-1.0f, top_limit, 3.0f, bottom - top_limit));
            }else{
                g.drawVerticalLine(left, top_limit, bottom);
            }
        }
        
        for(auto it = _seq._seq.begin(); it != _seq._seq.end(); ++it){
            const SequenceDrummer::SequenceEntry& s = *it;
            int x = _conv->convToScreenX(s._pos + _seq._delay);
            int y = _conv->convToScreenY(s._vel);
            if(s._pos % _seq._gridIntervalDuration == 0 || (!lockOffGrid))
                g.setColour(Colours::lightblue);
            else
                g.setColour(Colours::grey);
            int rad = 15;
            g.fillEllipse(x-rad/2, y-rad/2, rad, rad);

        }
    }
    
    Rectangle<int> getComponentSize(){
        Rectangle<int> r;
        r.setWidth(InternalParam::_hoffset * 2 + (float)_seq.getLength()/Durations::BEAT4*InternalParam::_widthForBEAT4);
        r.setHeight(InternalParam::_trackHeight);
        return r;
    }
    
    void mouseDown(const MouseEvent &event) override{
        Point<int> pos = event.getPosition();
        //Rectangle<int> ref = getRefArea();
        float x = _conv->convFromScreenX(pos.x);
        float y = _conv->convFromScreenY(pos.y);
        
        auto it = std::find_if(_seq._seq.begin(), _seq._seq.end(), [x](const SequenceDrummer::SequenceEntry& s){ return std::abs(x - s._pos)<Durations::BEAT32; });
        
        if(it != _seq._seq.end()){
            
            bool onGrid = it->_pos % _seq._gridIntervalDuration == 0;
            bool lockOffGrid = _pm.getBoolParam(ParameterManager::LOCK_OFF_GRID_PARAM)->get();
            if(lockOffGrid && (!onGrid)){
                // Nothing to do
            }else{
                _selSeq = &(*it);
                _selSeqItr = it;
                _selSeq->_vel = jmin(jmax(0, int(y)),127);
                repaint();
            }
            
        }else{
            // Judge if to add new note
            
            int gridId = ::round(x / _seq._gridIntervalDuration); // TODO more intuitive
            int nGrid = static_cast<int>(_seq._length / _seq._gridIntervalDuration); // Limit : this hould be integer
            //_selGridId = ::floor(x * _pattern._nGrid);
            if(!(gridId < 0 || gridId >= nGrid)){
                Duration gridPos = _seq._gridIntervalDuration * gridId;
                auto ins_it = std::find_if(_seq._seq.begin(), _seq._seq.end(),
                                       [gridPos](const SequenceDrummer::SequenceEntry& s){ return gridPos <= s._pos; });
                if( (*ins_it)._pos != gridPos ){ // Already there
                    SequenceDrummer::SequenceEntry newEntry;
                    newEntry._pos = gridPos;
                    newEntry._vel = jmin(jmax(0, int(y)),127);
                    _selSeqItr = _seq._seq.insert(ins_it, newEntry);
                    _selSeq = &(*_selSeqItr);
                    //_pattern._onoff[_selGridId] = jmin(jmax(0, int(y)),127);
                    repaint();
                }
            }
        }
    }
    
    void mouseDrag(const MouseEvent &event) override{
        if(_selSeq){
            Point<int> pos = event.getPosition();
            //Rectangle<int> ref = getRefArea();
            float x = _conv->convFromScreenX(pos.x);
            float y = _conv->convFromScreenY(pos.y);
            int gridId = ::round(x / _seq._gridIntervalDuration); // TODO more intuitive
            int nGrid = static_cast<int>(_seq._length / _seq._gridIntervalDuration); // Limit : this hould be integer
            
            if(y <= -20){
                _seq._seq.erase(_selSeqItr);
                _selSeq = nullptr;
            }else{

                Duration gridPos = _seq._gridIntervalDuration * gridId;
                if((gridId >= 0 && gridId < nGrid) && gridPos != _selSeq->_pos){
                    auto ins_it = std::find_if(_seq._seq.begin(), _seq._seq.end(),
                         [gridPos](const SequenceDrummer::SequenceEntry& s){ return gridPos <= s._pos; });
                    if( (*ins_it)._pos != gridPos ){ // No element in this position
                        if( ins_it != _selSeqItr ){
                            SequenceDrummer::SequenceEntry newEntry = *_selSeq;
                            newEntry._vel = jmin(jmax(0, int(y)),127);
                            newEntry._pos = gridPos;
                            _seq._seq.erase(_selSeqItr);
                            _selSeqItr = _seq._seq.insert(ins_it, newEntry);
                            _selSeq = &(*_selSeqItr);
                        }else{
                            // this can be possible. just rewrite the pos of existing element
                            _selSeq->_vel = jmin(jmax(0, int(y)),127);
                            _selSeq->_pos = gridPos;
                        }
                    }else{ // Already there
                        _selSeq->_vel = jmin(jmax(0, int(y)),127);
                    }
                }else{
                    _selSeq->_vel = jmin(jmax(0, int(y)),127);
                }
            }
            repaint();
        }
    }

    void mouseUp(const MouseEvent &event) override{
        _selSeq = nullptr;
    }
    

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceView)
};

#endif

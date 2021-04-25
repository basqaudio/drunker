/*
  ==============================================================================

    MainView.cpp
    Created: 25 Mar 2021 10:02:14pm
    Author:  Hiroyuki Baba

  ==============================================================================
*/

#include "MainView.h"


void LoopEndIndicator::onPeriodicDrag(int distX, int distY){
    LOG("onPeriodicDrag", distX, distY);
    
    SequenceDrummer& sd = dynamic_cast<SequenceDrummer&>(_drummer);
    SequenceDrummer::Sequence& seq = sd.getSequence();
    int offsetX = distX;
    Duration offsetD = _conv->convFromScreenX(offsetX);
    int offsetBEAT4 = int(offsetD / (float)Durations::BEAT4);
    Duration candLength = jmax(_lengthAtMouseDown + offsetBEAT4 * Durations::BEAT4, Durations::BEAT4);
    if(seq.getLength() < candLength){
        _dammy->setVisible(false);
        seq.setLength(candLength);

        // Callthe parent class's resize and repaint
        _prcv.resized();
        _prcv.scrollToRightMost();
        getParentComponent()->repaint();
        
        MessageChannel::getInstance().notify(MessageChannel::ON_LOOP_END_CHANGE);
    }else if(seq.getLength() > candLength){
        // We do not immediately change the length, but show the dammy indicator to specify the length
        _dammy->setVisible(true);
        _dammy->setAlwaysOnTop(true);
        _dammy->setTopLeftPosition(_conv->convToScreenX(candLength), 0);
        _prcv.scrollToIfInvisible(candLength);
    }else{
        _dammy->setVisible(false);
    }
}

void LoopEndIndicator::onPeriodicDragEnd(int distX, int distY){
    if(_dammy->isVisible()){
        // This is used to judge the nessecity for negative scroll
        _dammy->setVisible(false);
        
        SequenceDrummer& sd = dynamic_cast<SequenceDrummer&>(_drummer);
        SequenceDrummer::Sequence& seq = sd.getSequence();
        int offsetX = distX;
        Duration offsetD = _conv->convFromScreenX(offsetX);
        int offsetBEAT4 = int(offsetD / (float)Durations::BEAT4);
        Duration candLength = jmax(_lengthAtMouseDown + offsetBEAT4 * Durations::BEAT4, Durations::BEAT4);

        seq.setLength(candLength);
        // Callthe parent class's resize and repaint
        _prcv.resized();
        _prcv.scrollToRightMost();
        getParentComponent()->repaint();
        
        MessageChannel::getInstance().notify(MessageChannel::ON_LOOP_END_CHANGE);
    }
}

PianoRollContainerView::PianoRollContainerView(MainContentView& mcv, ParameterManager& pm, Drummer& drummer, ScrollBar& hScroll, ViewConverter* conv) : HScrollingView(hScroll), _drummer(drummer), _pm(pm), _conv(conv){
    setName("PianoRollContainerView");
    
    _pr = new PianoRollView(pm, drummer, conv);
    getScolloingContainer()->addAndMakeVisible(_pr);
    
    _li = new LoopEndIndicator(*this, *this, mcv, drummer, conv);
    getScolloingContainer()->addAndMakeVisible(_li);
    _li->setAlwaysOnTop(true);
    getScolloingContainer()->addChildComponent(_li->getDammyComponent());
    
    _ti = new TimeIndicator(drummer, conv);
    getScolloingContainer()->addAndMakeVisible(_ti);
    _ti->setAlwaysOnTop(true);
}

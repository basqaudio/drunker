/*
  ==============================================================================

    ScrollingView.h
    Created: 26 Mar 2021 8:47:16pm
    Author:  Hiroyuki Baba

  ==============================================================================
*/

#pragma once


class HScrollingView : public Component, public ScrollBar::Listener{
    class ScrolledViewH : public Component{
        HScrollingView& _parent;
    public:
        ScrolledViewH(HScrollingView& parent) : _parent(parent){}
        virtual ~ScrolledViewH(){}
        virtual void paint(Graphics& g) override {
            _parent.scrollViewPaint(g);
        }
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScrolledViewH)
    };
    Component::SafePointer<ScrolledViewH> _scrollingContainer;
public:
    ScrollBar& _hScrollBar;
    HScrollingView(ScrollBar& hScrollBar) : _hScrollBar(hScrollBar){
        _hScrollBar.addListener(this);
        addAndMakeVisible(_scrollingContainer = new ScrolledViewH(*this));
    }
    virtual ~HScrollingView(){
        _hScrollBar.removeListener(this);
        _scrollingContainer.deleteAndZero();
    }
    virtual void scrollToRightMost(){
        _hScrollBar.scrollToBottom();
    }
    
    Component* getScolloingContainer(){ return _scrollingContainer.getComponent(); }
    
    virtual void scrollBarMoved (ScrollBar* scrollBarThatHasMoved,
                                 double newRangeStart) override
    {
        _scrollingContainer->setTopLeftPosition( -newRangeStart, 0);
    }
    
    virtual void scrollViewPaint(Graphics& g) {}
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HScrollingView)
};

class VScrollingView : public Component, public ScrollBar::Listener{
    class ScrolledViewV : public Component{
        VScrollingView& _parent;
    public:
        ScrolledViewV(VScrollingView& parent) : _parent(parent){}
        virtual ~ScrolledViewV(){}
        virtual void paint(Graphics& g) override {
            _parent.scrollViewPaint(g);
        }
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScrolledViewV)
    };
    Component::SafePointer<ScrolledViewV> _scrollingContainer;
public:
    ScrollBar& _vScrollBar;
    VScrollingView(ScrollBar& vScrollBar) : _vScrollBar(vScrollBar){
        _vScrollBar.addListener(this);
        addAndMakeVisible(_scrollingContainer = new ScrolledViewV(*this));
    }
    virtual ~VScrollingView(){
        _vScrollBar.removeListener(this);
        _scrollingContainer.deleteAndZero();
    }
    virtual void scrollToBottomMost(){
        _vScrollBar.scrollToBottom();
    }
    
    Component* getScolloingContainer(){ return _scrollingContainer.getComponent(); }
    
    virtual void scrollBarMoved (ScrollBar* scrollBarThatHasMoved,
                                 double newRangeStart) override
    {
        _scrollingContainer->setTopLeftPosition( 0, -newRangeStart);
    }
    
    virtual void scrollViewPaint(Graphics& g) {}
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VScrollingView)
};

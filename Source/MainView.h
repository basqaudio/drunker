/*
  ==============================================================================

    MainView.h
    Created: 16 Mar 2021 1:23:59pm
    Author:  Hiroyuki Baba

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Common.h"
#include "Drummer.h"
#include "colormap.h"
#include <set>
#include "ScrollingView.h"
#include "LookAndFeels.h"
#include "CustomComponents.h"
#include "Bridge.h"

class PianoRollContainerView;
class MainContentView;

class PianoRollTimeRuler : public HScrollingView
{
    Drummer& _drummer;
    ViewConverter* _conv;
public:
    PianoRollTimeRuler(ScrollBar& hScrollBar, Drummer& drummer, ViewConverter* conv) : HScrollingView(hScrollBar), _drummer(drummer), _conv(conv)
    {
    }
    virtual ~PianoRollTimeRuler(){}
    virtual void resized() override{
        Rectangle<int> lb = getLocalBounds();
        getScolloingContainer()->setBounds(applySizeIfBigger(getContentSize(), lb));
    }
    Rectangle<int> getContentSize(){
        SequenceDrummer& sd = dynamic_cast<SequenceDrummer&>(_drummer);
        Rectangle<int> cs;
        cs.setWidth(InternalParam::_hoffset * 2 + _conv->convToScreenWidth(sd.getSequence().getLength()));
        cs.setHeight(InternalParam::pianoRollTopTimeRulerHeight);
        return cs;
    }
    virtual void scrollViewPaint(Graphics& g) override {
        SequenceDrummer& sd = dynamic_cast<SequenceDrummer&>(_drummer);
        SequenceDrummer::Sequence& seq = sd.getSequence();
        Rectangle<int> lb = getLocalBounds();
        g.fillAll(ColourParam::timeRulerBG);

        int n = 1;
        for(Duration d = 0; d <= seq.getLength(); d += Durations::BEAT4){
            int x = _conv->convToScreenX(d);
            g.setColour(ColourParam::timeRulerGridLine);
            g.drawLine(x,0,x,lb.getHeight(),3);
            
            g.setColour(ColourParam::labelColour);
            g.drawFittedText(String(n), x + 3, 2, 20, 10, Justification::left, 1);
            ++n;
        }
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRollTimeRuler)
};

class TimeIndicator : public Component, public Timer
{
    Drummer& _drummer;
    ViewConverter* _conv;
public:
    TimeIndicator(Drummer& drummer, ViewConverter* conv) : _drummer(drummer), _conv(conv) {
        startTimer(1000/24.0);
    }
    virtual ~TimeIndicator(){}
    virtual void timerCallback() override {
        this->setTopLeftPosition(_conv->convToScreenX(_drummer.getLocalTimeInDuration()), 0);
    }
    virtual void paint(Graphics& g) override {
        g.fillAll(Colours::white);
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimeIndicator)
};

class LoopEndIndicator : public Component, public InfiniteDragger::DragKeeperListener
{
    PianoRollContainerView& _prcv;
    Drummer& _drummer;
    ViewConverter* _conv;
    Duration _lengthAtMouseDown;
    std::unique_ptr<InfiniteDragger> _dragKeeper;
    class DammyIndicator : public Component {
    public:
        DammyIndicator(){}
        virtual ~DammyIndicator(){}
        virtual void paint(Graphics& g) override {
            g.fillAll(ColourParam::loopEndIndColor.withAlpha((float)0.5));
        }
    };
    SafePointer<DammyIndicator> _dammy;
public:
    LoopEndIndicator(PianoRollContainerView& prcv, Component& refScrollH, Component& refScrollV, Drummer& drummer, ViewConverter* conv) : _prcv(prcv), _drummer(drummer), _conv(conv) {
        this->setMouseCursor(MouseCursor(MouseCursor::StandardCursorType::LeftRightResizeCursor));
        
        _dragKeeper.reset(new InfiniteDragger(&refScrollH, &refScrollV));
        _dragKeeper->addListener(this);
        this->addMouseListener(_dragKeeper.get(), false);
        
        // Dummy indicator to show for negative dragging case
        _dammy = new DammyIndicator();
        _dammy->setInterceptsMouseClicks(false, false); // Dammy and not intercept any mouse events
    }
    Component* getDammyComponent(){
        return _dammy;
    }
    virtual ~LoopEndIndicator(){
        _dammy.deleteAndZero();
    }
    virtual void paint(Graphics& g) override {
        g.fillAll(ColourParam::loopEndIndColor);
    }
    
    virtual void onPeriodicDragStart() override{
        SequenceDrummer& sd = dynamic_cast<SequenceDrummer&>(_drummer);
        SequenceDrummer::Sequence& seq = sd.getSequence();
        _lengthAtMouseDown = seq.getLength();
    }
    virtual void onPeriodicDrag(int distX, int distY) override;
    virtual void onPeriodicDragEnd(int, int) override;
    
    virtual void resized() override{
        Rectangle<int> lb = getLocalBounds();
        _dammy->setBounds(lb);
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoopEndIndicator)
};

class PatternDnDComponent : public Component
{
    String _midiFilePath;
    Drummer& _drummer;
public:
    PatternDnDComponent(Drummer& drummer) : _drummer(drummer) {}
    virtual ~PatternDnDComponent(){}
    void mouseDown(const MouseEvent &event) override;
    
    void mouseDrag(const MouseEvent &event) override{
        DragAndDropContainer::performExternalDragDropOfFiles( { _midiFilePath }, false, nullptr, [ this ] ( void ) {
                DBG( "clip dropped" );
        });
    }

    void mouseUp(const MouseEvent &event) override{
    }
    
    void paint (Graphics& g) override {
        Image image = juce::ImageCache::getFromMemory (BinaryData::dand_icon_png, BinaryData::dand_icon_pngSize);

        g.drawImage(image, getLocalBounds().reduced(10).toFloat());
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PatternDnDComponent)
};

class PianoRollView : public Component
{
    Drummer& _drummer;
    ParameterManager& _pm;
    ViewConverter* _conv;
    
    enum MouseManupilation{
        MM_NONE = 0,
        MM_REGION_SELECT,
        MM_POSITION_DRAG,
        MM_POSITION_COPY_DRAG,
        MM_DURATION_DRAG_TAIL,
        MM_DURATION_DRAG_HEAD
    };
    
    Point<int> _selStart;
    Rectangle<int> _selRegion;
    SequenceDrummer::Selections _selsOnStart;
    
    MouseManupilation _mm = MM_NONE;
    
    struct ModifiesKeyState {
        bool altDown = false;
    } _modKeyState;
    
public:
    PianoRollView(ParameterManager& pm, Drummer& drummer, ViewConverter* conv) : _drummer(drummer), _pm(pm), _conv(conv){
        setName("PianoRollContainerView");
        setWantsKeyboardFocus(true);
        
        // GLOBAL_GRID_PARAM needs to be ready at this stage.
        pm.addCallback(pm.GLOBAL_GRID_PARAM, [this](float v,bool){
            SequenceDrummer& sd = dynamic_cast<SequenceDrummer&>(this->_drummer);
            SequenceDrummer::Sequence& seq = sd.getSequence();
            seq._gridIntervalDuration = Durations::BEAT4 / v;
            this->repaint();
        });
    }
    
    virtual ~PianoRollView(){
        // TODO : Remove callback to pm
    }
    
    void resized() override{

    }
    
    void paint(Graphics& g) override{
        SequenceDrummer& sd = dynamic_cast<SequenceDrummer&>(_drummer);
        SequenceDrummer::Sequence& seq = sd.getSequence();
        bool lockOffGrid = _pm.getBoolParam(ParameterManager::LOCK_OFF_GRID_PARAM)->get();

        g.fillAll (ColourParam::seqViewBG);   // clear the background
        
        static const int blackWhite[12] = {1,0,1,0,1,1,0,1,0,1,0,1}; // 0 : black, 1 : white
        

        for(int i = 0; i < 128; ++i){
            g.setColour(ColourParam::seqViewGridLine);
            g.drawLine(_conv->convToScreenX(0), _conv->convToScreenY(i), _conv->convToScreenX(sd.getSequence().getLength()), _conv->convToScreenY(i));
            
            if(blackWhite[i%12] == 1){
                // only white key
                g.setColour(ColourParam::seqViewWhiteKeyBG);
                g.fillRect(_conv->convToScreenX(0), _conv->convToScreenY(i+1), _conv->convToScreenX(sd.getSequence().getLength()) - _conv->convToScreenX(0), -_conv->convToScreenHeight(1));
            }
        }
        g.setColour(ColourParam::seqViewGridLine);
        for(Duration p = 0; p <= seq.getLength(); p += seq._gridIntervalDuration){
            
            if(p%Durations::BEAT4==0){
                g.drawLine(_conv->convToScreenX(p), _conv->convToScreenY(0), _conv->convToScreenX(p), _conv->convToScreenY(128), 3);
            }else{
                g.drawLine(_conv->convToScreenX(p), _conv->convToScreenY(0), _conv->convToScreenX(p), _conv->convToScreenY(128));
            }
        }
        


        // No read lock required in main thread process, as no write done outside main thread.
        SequenceDrummer::SeqStorageCItr it;
        for(it = seq.getStorage().begin(); it != seq.getStorage().end(); ++it){
            if(it->second._pos >= seq.getLength()) break;
            float x_grid = _conv->convToScreenX(it->second._pos - it->second._nudge); // intentinally use float as much as possible to smooth rendering.
            float x_act  = _conv->convToScreenX(it->second._pos);
            int y_p = _conv->convToScreenY(it->second._note+1); // This is the "upper" end of the rectangle for seq._note.
            bool onGrid = (it->second._pos - it->second._nudge) % seq._gridIntervalDuration == 0;
            colormap::COLOUR c = colormap::GetColour(it->second._vel, 0, 127);
            if(lockOffGrid && (!onGrid)){
                g.setColour(Colours::grey);
            }else{
                if( sd.isSelected(it) ){
                    g.setColour(Colour::fromRGB(c.r*255, c.g*255, c.b*255).brighter(1.0).brighter(1.0));
                }else{
                    g.setColour(Colour::fromRGB(c.r*255, c.g*255, c.b*255));
                }
            }
            
            if(it->second._nudge == 0){
                g.fillRect((int)x_grid, y_p, (int)_conv->convToScreenWidth(it->second._duration), (int)-_conv->convToScreenHeight(1));
            }else{
                Path path;
                float w = _conv->convToScreenWidth(it->second._duration);
                int h = -_conv->convToScreenHeight(1);
                path.startNewSubPath(x_grid, y_p);
                path.lineTo(x_act, y_p + h/2);
                path.lineTo(x_grid, y_p + h);
                path.lineTo(x_grid + w, y_p + h);
                path.lineTo(x_act + w, y_p + h/2);
                path.lineTo(x_grid + w, y_p);
                path.closeSubPath();
                
                g.fillPath(path);
            }
        }
        
        if(_mm == MM_REGION_SELECT){
            g.setColour(Colours::white);
            g.drawRect(_selRegion);
        }
    }
    
    Rectangle<int> getContentSize(){
        SequenceDrummer& sd = dynamic_cast<SequenceDrummer&>(_drummer);
        Rectangle<int> cs;
        cs.setWidth(InternalParam::_hoffset * 2 + _conv->convToScreenWidth(sd.getSequence().getLength()));
        cs.setHeight(-_conv->convToScreenHeight(128));
        return cs;
    }
    
    
    // Cursor seleciton model : Follow logic way in principle.
    //  - Default state:
    //    - On mouse move:
    //      - If mouse goes into right edge of note: horizationtal resize cursor
    //      - Otherwise default cursor
    //  - Duration change mode:
    //    - ON mouse move:
    //      - N/A
    //    - On drag:
    //      - Horizontal resize cursor until mouse up
    //  - Position drage mode:
    //    - On mouse move:
    //      - N/A
    //    - ON drag:
    //      - alt on:
    //        - Copy cursor
    //      - alt off:
    //        - Default cusor
    //    - On alt on
    //      - copy cusor ( to enable cursor change w/o drag)
    //    - On alt off
    //      - default cursor
    //  - Range selection mode:
    //    - No active config for any event.
    
    
    bool keyPressed(const KeyPress &k) override {
        if( k.isKeyCode(KeyPress::deleteKey) || k.isKeyCode(KeyPress::backspaceKey) ){
            SequenceDrummer& sd = dynamic_cast<SequenceDrummer&>(this->_drummer);

            sd.removeSelected();
            repaint();

            return true;
        }else if(k.isKeyCode(KeyPress::leftKey) || k.isKeyCode(KeyPress::rightKey)){
            if(k.getModifiers().isAltDown()){
                SequenceDrummer& sd = dynamic_cast<SequenceDrummer&>(this->_drummer);
                Duration delta = (k.isKeyCode(KeyPress::leftKey) ? -1 : 1)*Durations::TICK;
                for(SequenceDrummer::SelectionItr sit = sd.getSelection().begin(); sit != sd.getSelection().end(); ++sit){
                    Duration gridPos = sit->_selSeqItr->second._pos - sit->_selSeqItr->second._nudge;
                    SequenceDrummer::SequenceEntry e = sit->_selSeqItr->second;
                    e._nudge += delta;
                    e._nudge = jlimit(InternalParam::minNudge*Durations::TICK, InternalParam::maxNudge*Durations::TICK, e._nudge);
                    e._pos = gridPos + e._nudge;
                    sd.moveSelectedNote(sit, e);
                }
                sd.fixSelection();
                repaint();
                return true;
            }
        }
        return false;
    }
    
    virtual void modifierKeysChanged (const ModifierKeys& modifiers) override
    {
        if(_modKeyState.altDown != modifiers.isAltDown()){
            // Alt changed (down or up)
            _modKeyState.altDown = modifiers.isAltDown();
            if(_modKeyState.altDown){
                onAltDown();
            }else{
                onAltUp();
            }
        }
    }
    void onAltDown(){
        if(_mm == MM_POSITION_DRAG){
            this->setMouseCursor(MouseCursor(MouseCursor::StandardCursorType::CopyingCursor));
        }
    }
    void onAltUp(){
        if(_mm == MM_POSITION_DRAG){
            this->setMouseCursor(MouseCursor(MouseCursor::StandardCursorType::ParentCursor));
        }
    }
    void mouseMove(const MouseEvent &event) override{
        // Greedy check the hits. At least we can limit the search candidate to certain note
        if(_mm == MM_NONE){
            SequenceDrummer& sd = dynamic_cast<SequenceDrummer&>(_drummer);
            SequenceDrummer::Sequence& seq = sd.getSequence();
            Point<int> pos = event.getPosition();
            int note = (int)(_conv->convFromScreenY(pos.y));
            bool spCursorSet = false;
            for(SequenceDrummer::SeqStorageCItr it = seq.getStorage().begin(); it != seq.getStorage().end(); ++it)
            {
                if(it->second._note != note) continue;
                Rectangle<int> bb = getNoteBBox(it->second);
                if( abs(pos.x - bb.getRight()) <= 2 ){
                    this->setMouseCursor(MouseCursor(MouseCursor::StandardCursorType::LeftRightResizeCursor));
                    spCursorSet = true;
                    break;
                }
            }
            if(!spCursorSet)
                this->setMouseCursor(MouseCursor(MouseCursor::StandardCursorType::ParentCursor));
        }else if(_mm == MM_DURATION_DRAG_TAIL){
            // N/A
        }else if(_mm == MM_POSITION_DRAG){
            // N/A
        }else if(_mm == MM_REGION_SELECT){
            // N/A
        }
    }
    
    void mouseDown(const MouseEvent &event) override{
        Point<int> pos = event.getPosition();
        //Rectangle<int> ref = getRefArea();
        Duration x = _conv->convFromScreenX(pos.x);
        int note = (int)(_conv->convFromScreenY(pos.y));
        SequenceDrummer& sd = dynamic_cast<SequenceDrummer&>(_drummer);
        SequenceDrummer::Sequence& seq = sd.getSequence();
        
        _selStart = pos;
        _mm = MM_NONE; // default. In this mode, nothing proessed while dragging.
                
        // Selection policy
        // Common rule : Offgrid note is not selected when lockOffGrid is ON (common and independent rule)
        // 0. With Ctrl key(or add mode)
        //    Add a note and select it solely.
        // 1. Without shift key
        //    if the selected note is one of the already selected notes
        //        do not clear.
        //    else (incl. no selection case)
        //        clear
        // 2. with shift key
        //    if the selected note is one of the already selected notes
        //        clear the election of that note
        //    else
        //        add the selected note if selected.
        //    common : Disable dragging process untill next mouse down
        auto it = std::find_if(seq.getStorage().begin(), seq.getStorage().end(), [x, note](const SequenceDrummer::SeqStoragePair& p){ return p.second._note==note && ( (p.second._pos-p.second._nudge) <= x) && ( x <= (p.second._pos-p.second._nudge) + p.second._duration); });

        if(event.mods.isCommandDown()){
            // Addition of new note
            int gridId = ::round(x / seq._gridIntervalDuration); // TODO more intuitive
            int nGrid = static_cast<int>(seq.getLength() / seq._gridIntervalDuration); // Limit : this hould be integer
            //_selGridId = ::floor(x * _pattern._nGrid);
            if(gridId >=0 && gridId < nGrid){
                Duration gridPos = seq._gridIntervalDuration * gridId; // equals actual pos with nudge = 0
                SequenceDrummer::SequenceEntry newEntry;
                newEntry._note = note;
                newEntry._pos = gridPos;
                newEntry._nudge = 0;
                newEntry._vel = 127;
                newEntry._duration = Durations::BEAT32;
                auto selSeqItr = seq.insert(SequenceDrummer::SeqStoragePair(newEntry._pos,newEntry));
                
                sd.clearSelection();
                sd.addSelection(selSeqItr);
            }
        }else if(!event.mods.isShiftDown()){
            // Witout shift key
            if(it == seq.getStorage().end()){
                // no selection
                sd.clearSelection();
                _mm = MM_REGION_SELECT; // Region select mode
            }else if( sd.isSelected(it) ){
                // already selected
                _mm = abs(getNoteBBox(it->second).getRight() - pos.x) <= 2 ? MM_DURATION_DRAG_TAIL : MM_POSITION_DRAG;
            }else{
                // other note selected
                sd.clearSelection();
                sd.addSelection(it);
                _mm = abs(getNoteBBox(it->second).getRight() - pos.x) <= 2 ? MM_DURATION_DRAG_TAIL : MM_POSITION_DRAG;

            }
        }else{
            // With shift key
            if(it == seq.getStorage().end()){
                // Not selected
                // Do nothing
                _mm = MM_REGION_SELECT; // And also addition mode
            }else if( sd.isSelected(it)){
                // Already selected note
                sd.deleteSelection(it);
            }else{
                // New selection
                sd.addSelection(it);
            }
        }
        
        bool lockOffGrid = _pm.getBoolParam(ParameterManager::LOCK_OFF_GRID_PARAM)->get();
        if(lockOffGrid){
            // Delete selection for the off grid notes if any
            Duration gridIntervalDuration = seq._gridIntervalDuration;
            sd.unSelectIf([gridIntervalDuration](const SequenceDrummer::SequenceEntry& e){ return (e._pos - e._nudge) % gridIntervalDuration != 0; });
        }
        
        if(_mm == MM_REGION_SELECT){
            // Remeber the current selection set
            _selsOnStart = sd.getSelection();
        }else if(_mm == MM_POSITION_DRAG){
            sd.stash();
            sd.duplicateSelection();
            sd.labelCurrentAs(false);
        }
        
        repaint();
    }
    
    Rectangle<int> getNoteBBox(const SequenceDrummer::SequenceEntry& s){
        int y = _conv->convToScreenY(s._note+1);
        int x = _conv->convToScreenX(s._pos - s._nudge); // We always judge collision detection based on grid-based position.
        int w = _conv->convToScreenWidth(s._duration);
        int h = -_conv->convToScreenHeight(1);
        return Rectangle<int>(x,y,w,h);
    }
    
    void mouseDrag(const MouseEvent &event) override{
        if(_mm == MM_REGION_SELECT){
            SequenceDrummer& sd = dynamic_cast<SequenceDrummer&>(_drummer);
            SequenceDrummer::Sequence& seq = sd.getSequence();
            // Region selection mode
            Point<int> pos = event.getPosition();
            _selRegion.setX( jmin(pos.x, _selStart.x));
            _selRegion.setY( jmin(pos.y, _selStart.y));
            _selRegion.setWidth( abs(pos.x - _selStart.x) );
            _selRegion.setHeight( abs(pos.y - _selStart.y) );
            
            // Rule : as per Logic :
            // If the note within the  selecition region and
            //   - If such note is already selected in initial selection set, unselect it
            //   - Otherwise select it.
            SequenceDrummer::Selections _newSel = _selsOnStart;
            for(SequenceDrummer::SeqStorageCItr it = seq.getStorage().begin(); it != seq.getStorage().end(); ++it)
            {
                if(_selRegion.intersects(getNoteBBox(it->second))){
                    if(SequenceDrummer::FindSelection(_newSel, it)){
                        _newSel.remove_if([it](const SequenceDrummer::Selection& s){ return s._selSeqItr == it; });
                    }else{
                        _newSel.push_back({it,it->second});
                    }
                }
            }
            
            sd.setSelection(_newSel);
            
            repaint();
        }else if(_mm == MM_DURATION_DRAG_TAIL){
            // Actually, cursor config is not needed as when the this mode is triggered, cursor is already horizontal resize cursor.
            this->setMouseCursor(MouseCursor(MouseCursor::StandardCursorType::LeftRightResizeCursor));
            
            SequenceDrummer& sd = dynamic_cast<SequenceDrummer&>(_drummer);
            // Region selection mode
            Point<int> pos = event.getPosition();
            int deltaScreenX = pos.x - _selStart.x; // either positive or negative
            Duration deltaDuration = _conv->convFromScreenWidth(deltaScreenX);
            if( sd.changeDurationSelected(deltaDuration) ){
                repaint();
            }
        }else if(_mm == MM_POSITION_DRAG){
            if(event.mods.isAltDown())
                this->setMouseCursor(MouseCursor(MouseCursor::StandardCursorType::CopyingCursor));
            else
                this->setMouseCursor(MouseCursor(MouseCursor::StandardCursorType::ParentCursor));

            // Dragging selected notes
            float deltaX = _conv->convFromScreenWidth(event.getDistanceFromDragStartX());
            int deltaNote = _conv->convFromScreenHeight(event.getDistanceFromDragStartY());
            SequenceDrummer& sd = dynamic_cast<SequenceDrummer&>(_drummer);
            sd.selectFrontBack( !event.mods.isAltDown() );
            if( sd.dragSelected(deltaX, deltaNote) ){
                repaint();
            }
        }
    }

    void mouseUp(const MouseEvent &event) override{
        SequenceDrummer& sd = dynamic_cast<SequenceDrummer&>(_drummer);
        if(_mm == MM_POSITION_DRAG){
            sd.selectFrontBack( !event.mods.isAltDown() );
        }
        sd.fixSelection();
        if(_mm == MM_REGION_SELECT){
            // To delete the selection rectangle, repaint after setting _mm to MM_NONE
            _mm = MM_NONE;
            repaint();
        }else{
            _mm = MM_NONE;
        }
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRollView)
};


class PianoRollContainerView : public HScrollingView //public Component, public ScrollBar::Listener
{
    Drummer& _drummer;
    ParameterManager& _pm;
    SafePointer<PianoRollView> _pr;
    SafePointer<TimeIndicator> _ti;
    SafePointer<LoopEndIndicator> _li;
    ViewConverter* _conv;
public:
    PianoRollContainerView(MainContentView& mcv, ParameterManager& pm, Drummer& drummer, ScrollBar& hScroll, ViewConverter* conv);
    
    virtual ~PianoRollContainerView(){
        removeAllChildren();
        
        _ti.deleteAndZero();
        _li.deleteAndZero();
        _pr.deleteAndZero();
    }
    
    void resized() override{
        SequenceDrummer& sd = dynamic_cast<SequenceDrummer&>(_drummer);
        SequenceDrummer::Sequence& seq = sd.getSequence();
        
        Rectangle<int> contentSize = getContentSize();
        getScolloingContainer()->setBounds( applySizeIfBigger(contentSize, getLocalBounds()) );
        _pr->setBounds( applySizeIfBigger(contentSize, getLocalBounds()) );
        
        _hScrollBar.setRangeLimits(0.0, getScolloingContainer()->getWidth());
        _hScrollBar.setCurrentRange(_hScrollBar.getCurrentRangeStart(), getWidth());

        _li->setTopLeftPosition(_conv->convToScreenX(seq.getLength()),0);
        _li->setSize(InternalParam::loopEndIndWidth, getHeight());
        
        _ti->setSize(1,getHeight());
    }
    
    Rectangle<int> getContentSize(){
        Rectangle<int> cs = _pr->getContentSize();
        cs.setRight( cs.getRight() + InternalParam::loopEndIndWidth);
        
        return cs;
    }
    
    void scrollToIfInvisible(Duration d){
        int targetPos = _conv->convToScreenX(d);
        int startPos = _hScrollBar.getCurrentRangeStart();
        int width = _hScrollBar.getCurrentRangeSize();
        if( targetPos < startPos || targetPos > startPos + width){
            // Out of visivility
            _hScrollBar.setCurrentRangeStart(targetPos);
        }
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRollContainerView)
};

class PianoRollHeaderContainerView : public Component, public MidiKeyboardState::Listener
{
    Drummer& _drummer;
    ParameterManager& _pm;
    MidiKeyboardState _keyState;
    SafePointer<MidiKeyboardComponent> _kb;
    ViewConverter* _conv;
public:
    PianoRollHeaderContainerView(ParameterManager& pm, Drummer& drummer, ViewConverter* conv) : _drummer(drummer), _pm(pm), _conv(conv){
        setName("PianoRollHeaderContainerView");
        addAndMakeVisible(_kb = new MidiKeyboardComponent(_keyState, juce::MidiKeyboardComponent::verticalKeyboardFacingRight ));
        _keyState.addListener(this);
        
    }
    
    virtual ~PianoRollHeaderContainerView()
    {
        _kb.deleteAndZero();
    }
    
    virtual void handleNoteOn (MidiKeyboardState* source,
                               int midiChannel, int midiNoteNumber, float velocity) override
    {
        bool lockOffGrid = _pm.getBoolParam(ParameterManager::LOCK_OFF_GRID_PARAM)->get();
        SequenceDrummer& sd = dynamic_cast<SequenceDrummer&>(_drummer);
        Duration gridIntervalDuration = sd.getSequence()._gridIntervalDuration;
        
        sd.clearSelection();
        sd.selectIf([midiNoteNumber, lockOffGrid, gridIntervalDuration](const SequenceDrummer::SequenceEntry& e){ return e._note == midiNoteNumber && (lockOffGrid ? (e._pos - e._nudge) % gridIntervalDuration == 0 : true); });
        Component* pianoRoll = getParentComponent()->findChildWithID("PianoRollContainerView");
        if(pianoRoll){
            pianoRoll->repaint();
            pianoRoll->grabKeyboardFocus();
        }
        
    }
    
    virtual void handleNoteOff (MidiKeyboardState* source,
                                int midiChannel, int midiNoteNumber, float velocity) override
    {
        
    }
    
    void paint(Graphics& g) override{
        g.fillAll (ColourParam::seqHeaderBG);   // clear the background
    }
    
    void resized() override{
        Rectangle<int> lb = getLocalBounds();
        int overallheight = -_conv->convToScreenHeight(128); // Positive y returns negative height.
        float oneKeyHeight  = -_conv->convToScreenHeight(12)/7.0;
        _kb->setBounds(0, _conv->convToScreenY(128), lb.getWidth(), overallheight);
        _kb->setKeyWidth(oneKeyHeight); // This
        _kb->setAvailableRange(0, 127);
        _kb->setScrollButtonsVisible(false);
        
#ifdef DEBUG
        float totalWidth = _kb->getTotalKeyboardWidth();
        LOG("KB_DEBUG", totalWidth, overallheight, oneKeyHeight);
        for(int i = 0; i < 128; ++i)
            LOG("KB_POS", i, _kb->getKeyStartPosition (i));

#endif
    }
    
    Rectangle<int> getContentSize(){
        Rectangle<int> cs;
        cs.setHeight(-_conv->convToScreenHeight(128));
        cs.setWidth( InternalParam::pianoRollHeaderWidth );
        return cs;
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRollHeaderContainerView)
};



class UpperBar : public Component
{
    //Drummer& _drummer;
    SequenceDrummer& _sd;
    std::unique_ptr<Component> _dndArea;
    std::unique_ptr<HBox> _leftBox;
    std::unique_ptr<HBox> _gridBox;

    std::unique_ptr<IButtonBridge> _lockBtnBridge;
    std::unique_ptr<SliderBridge> _gridSelectorBridge;
    std::unique_ptr<SliderBridge> _velocityChangerBridge;
    std::unique_ptr<SliderBridge> _nudgeChangerBridge;
    std::unique_ptr<ZoomSlider> _vzoomNob;
    std::unique_ptr<SliderBridge> _vzoomSliderBridge;
    std::unique_ptr<ZoomSlider> _hzoomNob;
    std::unique_ptr<SliderBridge> _hzoomSliderBridge;
    std::unique_ptr<IButtonBridge> _playBtnBridge;
    std::unique_ptr<IButtonBridge> _recBtnBridge;
    SafePointer<PlayControl> _playCont;
    ViewZoomSlider _viewZoomSlierLF;
    ParamController _paramControllerLF;

    ParameterManager& _pm;
public:
    UpperBar(ParameterManager& pm, SequenceDrummer& drummer) : _sd(drummer), _pm(pm){
        setName("UpperBar");
        _dndArea.reset(new HBox(new PatternDnDComponent(drummer),{1,1},HBox::LEFT,ColourParam::upViewBoundColour));

        
        // Left
        {
            _leftBox.reset(new HBox({1,1,1,1}));
            _leftBox->setBorder(HBox::RIGHT, ColourParam::upViewBoundColour);
            addAndMakeVisible(_leftBox.get());
            {
                AudioParameterFloat* p = pm.getFloatParam(ParameterManager::VELOCITY_PARAM);
                RotarySliderTypeA* velocityNob = new RotarySliderTypeA("Velocity");
                _leftBox->addItem(velocityNob);
                _velocityChangerBridge.reset(new SliderBridge(p, velocityNob->getSlider()));
                _sd.addCallback(std::bind(&UpperBar::onSelectionChange, this));
                pm.addCallback(pm.VELOCITY_PARAM, std::bind(&UpperBar::onVelocityChanged,this,std::placeholders::_1), std::bind(&UpperBar::onVelocityGestureChanged,this,std::placeholders::_1));
            }
            {
                AudioParameterFloat* p = pm.getFloatParam(ParameterManager::NUDGE_PARAM);
                RotarySliderTypeA* nudgeNob = new RotarySliderTypeA("Nudge");
                _leftBox->addItem(nudgeNob);
                _nudgeChangerBridge.reset(new SliderBridge(p, nudgeNob->getSlider()));
                pm.addCallback(pm.NUDGE_PARAM, std::bind(&UpperBar::onNudgeChanged,this,std::placeholders::_1, std::placeholders::_2), std::bind(&UpperBar::onNudgeGestureChanged,this,std::placeholders::_1));
            }
        }
        
        // Center
        {
            std::function<void(bool)> cb = [this](bool v){
                // Hoge
                LOG("PLAYSTOP",(int)v);
            };
            _playCont = new PlayControl();
            addAndMakeVisible(_playCont);
            AudioParameterBool* pp = pm.getBoolParam(ParameterManager::PLAYSTOP_PARAM);
            _playBtnBridge.reset(new ImageToggleButtonBridge(pp, dynamic_cast<ImageToggleButton*>(_playCont->getPlayButton()), cb));

            AudioParameterBool* pr = pm.getBoolParam(ParameterManager::RECORD_PARAM);
            _recBtnBridge.reset(new ImageToggleButtonBridge(pr, dynamic_cast<ImageToggleButton*>(_playCont->getRecButton()), cb));

            AudioParameterFloat* pt = pm.getFloatParam(ParameterManager::TEMPO_PARAM);
            new FloatLabelBridge(pt, _playCont->getLabel());
        }
        
        // Right
        {
            _gridBox.reset(new HBox({{1,1,1,1},{1,1,1,25}}));
            _gridBox->setBorder(HBox::RIGHT|HBox::LEFT, ColourParam::upViewBoundColour);
            _gridBox->setTitle("Grid");
            addAndMakeVisible(_gridBox.get());

            
            {
                NormalisableRange<float> nr(1,16,1);
                
                pm.setNotifyingHost(ParameterManager::GLOBAL_GRID_PARAM, Durations::BEAT4/drummer.getSequence()._gridIntervalDuration);
                AudioParameterFloat* p = pm.getFloatParam(ParameterManager::GLOBAL_GRID_PARAM);
                RotarySliderTypeA* gridNob = new RotarySliderTypeA("Div.");
                _gridBox->addItem(gridNob);
                
                _gridSelectorBridge.reset(new SliderBridge(p, gridNob->getSlider()));
            }
            
            {
                std::function<void(bool)> cb = [this](bool v){
                    this->_sd.setLockOffGrid(v);
                    this->getParentComponent()->repaint();
                };
                
                pm.setNotifyingHost(ParameterManager::LOCK_OFF_GRID_PARAM, (float)_sd.getLockOffGrid());
                AudioParameterBool* p = pm.getBoolParam(ParameterManager::LOCK_OFF_GRID_PARAM);
                
                Image onImage = juce::ImageCache::getFromMemory (BinaryData::lockoffgrid_lock_png, BinaryData::lockoffgrid_lock_pngSize);
                Image offImage = juce::ImageCache::getFromMemory (BinaryData::lockoffgrid_unlock_png, BinaryData::lockoffgrid_unlock_pngSize);
                
                ImageToggleButton* lockBtn = new ImageToggleButton(offImage, onImage);
                _gridBox->addItem(new HBox(lockBtn,{20,0}));
                
                _lockBtnBridge.reset(new ImageToggleButtonBridge(p, lockBtn, cb));
            }
        }
        {
            AudioParameterFloat* p = pm.getFloatParam(ParameterManager::VZOOM_PARAM);
            _vzoomNob.reset(new ZoomSlider(false));
            addAndMakeVisible(_vzoomNob.get());
            _vzoomSliderBridge.reset(new SliderBridge(p, _vzoomNob->getSlider()));
        }
        {
            AudioParameterFloat* p = pm.getFloatParam(ParameterManager::HZOOM_PARAM);
            _hzoomNob.reset(new ZoomSlider(true));
            addAndMakeVisible(_hzoomNob.get());
            _hzoomSliderBridge.reset(new SliderBridge(p, _hzoomNob->getSlider()));
        }
        
        addAndMakeVisible(_dndArea.get());

    }
    virtual ~UpperBar(){
        LOG("UpperBar destructed");
        
        removeAllChildren();
    }
    
    void onSelectionChange(){
        {
            int minVel = std::accumulate(_sd.getSelection().begin(), _sd.getSelection().end(), 128, [](int acc, const SequenceDrummer::Selection& s) { return (int)( acc < s._selSeqItr->second._vel ? acc :   s._selSeqItr->second._vel); });
            if(minVel < 128){
                // No selection will result in 128
                _pm.setNotifyingHost(_pm.VELOCITY_PARAM, minVel);
                LOG("Selectiong update", minVel);
            }
        }
        {
            double sumNudge = std::accumulate(_sd.getSelection().begin(), _sd.getSelection().end(), 0, [](double acc, const SequenceDrummer::Selection& s) { return (double)( acc + s._mouseDownSnapShot._nudge); });
            if(_sd.getSelection().size() > 0){
                double avgNudge = sumNudge / _sd.getSelection().size();
                _pm.setNotifyingHost(_pm.NUDGE_PARAM, round(avgNudge/Durations::TICK));
                LOG("Selectiong update", avgNudge);
            }
        }
    }
    
    void onVelocityChanged(float v){
        int orgMinVel = std::accumulate(_sd.getSelection().begin(), _sd.getSelection().end(), 128, [](int acc, const SequenceDrummer::Selection& s) { return (int)( acc < s._mouseDownSnapShot._vel ? acc : s._mouseDownSnapShot._vel); });
        if(orgMinVel < 128){
            int delta = v - orgMinVel;
            for(SequenceDrummer::SelectionItr sit = _sd.getSelection().begin(); sit != _sd.getSelection().end(); ++sit){
                _sd.getSequence().updateVelocity(sit->_selSeqItr, jmin(jmax(0, sit->_mouseDownSnapShot._vel + delta),127));
            }
            getParentComponent()->repaint();
        }
    }
    
    void onVelocityGestureChanged(bool isStarting){
        _sd.fixSelection();
    }
    
    void onNudgeChanged(float v, bool isInUserGesture){
        if(_sd.getSelection().size()==0) return;
        if(!isInUserGesture) return;
        
        Duration dv = v * Durations::TICK;
        double sumNudge = std::accumulate(_sd.getSelection().begin(), _sd.getSelection().end(), 0, [](double acc, const SequenceDrummer::Selection& s) { return (double)( acc + s._mouseDownSnapShot._nudge); });
        double avgNudge = sumNudge / _sd.getSelection().size();
        for(SequenceDrummer::SelectionItr sit = _sd.getSelection().begin(); sit != _sd.getSelection().end(); ++sit){
            Duration delta = (Duration)(dv - avgNudge);
            Duration gridPos = sit->_mouseDownSnapShot._pos - sit->_mouseDownSnapShot._nudge;
            SequenceDrummer::SequenceEntry e = sit->_mouseDownSnapShot;
            e._nudge += delta;
            e._nudge = jlimit(InternalParam::minNudge*Durations::TICK, InternalParam::maxNudge*Durations::TICK, e._nudge);
            e._pos = gridPos + e._nudge;
            _sd.moveSelectedNote(sit, e);
        }
        getParentComponent()->repaint();
    }
    
    void onNudgeGestureChanged(bool isStarting){
        _sd.fixSelection();
    }
    
    void paint (Graphics& g) override {
        g.setColour(Colours::black);
        g.fillRect(getLocalBounds());
    }
    
    void resized() override {
        Rectangle<int> lb = getLocalBounds();

        int left_end = 0;
        int right_start = lb.getWidth();
        
        {
            Rectangle<int> bb = lb;
            bb.setX(0);
            bb.setWidth(150);
            _leftBox->setBounds(bb);
            left_end += 150;
        }

        // Right
        {
            Rectangle<int> bb = lb;
            bb.setX(lb.getWidth() - lb.getHeight() - 350);
            bb.setWidth(150);
            _gridBox->setBounds(bb);
            right_start = bb.getX();
        }
        {
            Rectangle<int> bb = lb;
            bb.setX(lb.getWidth() - lb.getHeight() - 200);
            bb.setWidth(100);
            _vzoomNob->setBounds(bb);
        }
        {
            Rectangle<int> bb = lb;
            bb.setX(lb.getWidth() - lb.getHeight() - 100);
            bb.setWidth(100);
            _hzoomNob->setBounds(bb);
        }
        {
            Rectangle<int> bb = lb;
            bb.setLeft(lb.getWidth() - lb.getHeight());
            _dndArea->setBounds(bb);
        }
        
        // Center
        {
            Rectangle<int> bb = lb;
            bb.setWidth(150);
            bb.setCentre( (left_end + right_start)/2, lb.getCentreY() );
            _playCont->setBounds(bb);
        }

        
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UpperBar)
};

class MainContentView : public VScrollingView
{
    Drummer& _drummer;
    SafePointer<PianoRollHeaderContainerView> _headerView;
    SafePointer<PianoRollContainerView> _containerView;
    
    ViewConverter* _conv;
    
    ScrollBar& _hScrollBar; // This is not in the base class
    
public:
    MainContentView(ParameterManager& pm, Drummer& drummer, ScrollBar& hScroll, ScrollBar& vScroll, ViewConverter* conv) : VScrollingView(vScroll), _drummer(drummer), _conv(conv), _hScrollBar(hScroll) {
        setName("MainContentView");
        setComponentID("MainContentView");
        
        if(typeid(drummer) == typeid(SequenceDrummer))
        {
            getScolloingContainer()->addAndMakeVisible(_headerView = new PianoRollHeaderContainerView(pm, drummer, _conv));
            _containerView = new PianoRollContainerView(*this, pm, drummer, hScroll, _conv);
            _containerView->setComponentID("PianoRollContainerView"); // This can be used to search...
            getScolloingContainer()->addAndMakeVisible(_containerView);
        }
    }
    
    virtual ~MainContentView(){
        removeAllChildren();
        _containerView.deleteAndZero();
        _headerView.deleteAndZero();
    }
    
    Rectangle<int> getContentSize(){
        return _headerView->getContentSize();
    }
    
    void resized () override {
        Rectangle<int> lb = getLocalBounds();
        Rectangle<int> headerContSize = _headerView->getContentSize();
        //Rectangle<int> bodyContSize = _containerView->getContentSize();


        // Assume here that containerView and haderView has the same height
        _vScrollBar.setRangeLimits(0, headerContSize.getHeight());
        _vScrollBar.setCurrentRange(_vScrollBar.getCurrentRangeStart(), jmin(headerContSize.getHeight(), lb.getHeight()));
        {
            setBoundsForceResized(_headerView, applyHeightIfTaller(headerContSize, lb));
        }
        {
            Rectangle<int> bb;
            bb.setWidth( lb.getWidth()-headerContSize.getWidth() );
            bb.setHeight( jmax( headerContSize.getHeight(), lb.getHeight() ));
            bb.setX(headerContSize.getWidth());
            setBoundsForceResized(_containerView, bb);
        }
        {
            // TO enable vertical scroll at this level, longer height, but clip the width
            Rectangle<int> bb;
            bb.setWidth(lb.getWidth());
            bb.setHeight(jmax( headerContSize.getHeight(), lb.getHeight() ));
            setBoundsForceResized(getScolloingContainer(), bb);
        }
    }
    
    virtual void mouseWheelMove(
        const MouseEvent &event,
        const MouseWheelDetails &wheel) override
    {
        //LOG("WHEEL", event.x, event.y, wheel.deltaX, wheel.deltaY, _scrollingContainer->getX(), _scrollingContainer->getY());
        
        float speed = 100.0; // deltaX, deltaY seems to not reflecting the actual positions, it is up to the program how to apply it to the coordinates. x1 does not work as normally deltaX, deltaY is enough smaller than 1.0 then it 0 when casted to int
        _vScrollBar.setCurrentRangeStart( _vScrollBar.getCurrentRangeStart() - std::ceil(speed * wheel.deltaY));
        
        // H scroll, we do not respond to header region. Scroll only apply in container region.
        if(event.x > _headerView->getWidth()){
            _hScrollBar.setCurrentRangeStart( _hScrollBar.getCurrentRangeStart() - std::ceil(speed * wheel.deltaX));
        }
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentView)
};

class MainView : public Component
{
    Drummer& _drummer;
    ParameterManager& _pm;
    SafePointer<MainContentView> _containerView;
    SafePointer<PianoRollTimeRuler> _timeRuler;
    
    SafePointer<ScrollBar> _vScrollBar;
    SafePointer<ScrollBar> _hScrollBar;
    std::unique_ptr<ViewConverter> _conv;
    Rectangle<int> _refRegionForConv;
    
public:
    MainView(ParameterManager& pm, Drummer& drummer) : _drummer(drummer), _pm(pm){
        setName("MainView");
        
        setReferenceRegion(1.0, 1.0);
        
        _conv.reset( new ViewConverter(_refRegionForConv) );
        _conv->xrange(0, Durations::BEAT4);
        _conv->yrange(0, 128);
        
        if(typeid(drummer) == typeid(SequenceDrummer))
        {
            _vScrollBar = new ScrollBar(true);
            _vScrollBar->setAutoHide(true);
            _vScrollBar->setRangeLimits(0.0, 1.0);
            _vScrollBar->setCurrentRange(0.0, 1.0);
            
            _hScrollBar = new ScrollBar(false); // Configuration is done in SequenceContainerView.
            
            _containerView = new MainContentView(pm, drummer, *_hScrollBar.getComponent(), *_vScrollBar.getComponent(), _conv.get());
            
            addAndMakeVisible(_containerView);
            addAndMakeVisible(_timeRuler = new PianoRollTimeRuler( *_hScrollBar.getComponent(), _drummer, _conv.get()));
            // To make scroll bar at the top, added finally.
            addAndMakeVisible(_vScrollBar);
            addAndMakeVisible(_hScrollBar);

        }
        
        MessageChannel::getInstance().addCallback(MessageChannel::ON_LOOP_END_CHANGE, std::bind(&MainView::onLoopEndChange, this));
        pm.addCallback(pm.VZOOM_PARAM, std::bind(&MainView::onVzoomSliderChanged, this, std::placeholders::_1));
        pm.addCallback(pm.HZOOM_PARAM, std::bind(&MainView::onHzoomSliderChanged, this, std::placeholders::_1));

    }
    
    void setReferenceRegion(float vzoom, float hzoom){
        _refRegionForConv = Rectangle<int>(InternalParam::_hoffset, 0, InternalParam::_widthForBEAT4 * hzoom, InternalParam::pianoRollRowHeight*128 * vzoom);
    }
    virtual ~MainView(){
        removeAllChildren();
        _hScrollBar.deleteAndZero();
        _vScrollBar.deleteAndZero();
        _timeRuler.deleteAndZero();
        _containerView.deleteAndZero();
    }
    
    void onVzoomSliderChanged(float vzoom)
    {
        setReferenceRegion(NonDB(vzoom), NonDB(_pm.getFloat(ParameterManager::HZOOM_PARAM)));
        resized();
        repaint();
    }
    
    void onHzoomSliderChanged(float hzoom)
    {
        setReferenceRegion(NonDB(_pm.getFloat(ParameterManager::VZOOM_PARAM)), NonDB(hzoom));
        resized();
        repaint();
    }
    
    void onLoopEndChange(){
        _timeRuler->resized();
        _timeRuler->repaint();
    }
    
    void resized () override {
        Rectangle<int> lb = getLocalBounds();
        Rectangle<int> timeRulerSize = _timeRuler->getContentSize();


        // Assume here that containerView and haderView has the same height
        _vScrollBar->setRangeLimits(0, _containerView->getContentSize().getHeight());
        _vScrollBar->setCurrentRange(_vScrollBar->getCurrentRangeStart(), jmin(_containerView->getContentSize().getHeight(), lb.getHeight()));

        {
            Rectangle<int> bb;
            bb.setWidth( lb.getWidth()-InternalParam::pianoRollHeaderWidth );
            bb.setHeight( timeRulerSize.getHeight() );
            bb.setX(InternalParam::pianoRollHeaderWidth);
            setBoundsForceResized(_timeRuler, bb);
        }
        {
            Rectangle<int> bb;
            bb.setWidth( lb.getWidth());
            bb.setHeight( lb.getHeight() - timeRulerSize.getHeight() );
            bb.setX(0);
            bb.setY(timeRulerSize.getHeight());
            setBoundsForceResized(_containerView, bb);

        }
        
        _vScrollBar->setBounds(
            lb.withLeft(lb.getWidth() - InternalParam::scrollBarWidth).withTop(timeRulerSize.getHeight()));
        _hScrollBar->setBounds(lb.withTop(lb.getHeight() - InternalParam::scrollBarWidth).withLeft(InternalParam::pianoRollHeaderWidth));
        
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainView)
};

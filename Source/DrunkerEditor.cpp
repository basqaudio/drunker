/*
  ==============================================================================

    DrunkenEditor.cpp
    Created: 9 Mar 2021 7:20:31pm
    Author:  Hiroyuki Baba

  ==============================================================================
*/

#include "DrunkerEditor.h"
#include "Drunker.h"
#include "PatternView.h"
#include "MainView.h"

DrunkerEditor::DrunkerEditor (AudioProcessor& p) : AudioProcessorEditor(p){
    setName("DrunkerEditor");
    DrunkerProcessor& dp = dynamic_cast<DrunkerProcessor&>(p);

    std::pair<int, int> windowSize = std::make_pair(800, 600);

    // Clear context information which shoudl be reset for new GUI instance
    MessageChannel::getInstance().clear();
    dp.getParameterManager().clearCallbacks();
    dp.getDrummer().clearContextInfo();
    
    
    // This should be added first due to common parameter availability is needed for the subsequent codes.
    addAndMakeVisible(_upperBar = new UpperBar(dp.getParameterManager(), dynamic_cast<SequenceDrummer&>(dp.getDrummer())));
    _mainView = new MainView(dp.getParameterManager(), dp.getDrummer());
    addAndMakeVisible(_mainView);
    
    
    setResizable(true, true);
    setResizeLimits(windowSize.first/2, windowSize.second/2, windowSize.second*2, windowSize.second*2);
    // This is to invoke resized() callback.
    setSize(windowSize.first, windowSize.second);
}

DrunkerEditor::~DrunkerEditor()  {
    LOG("DrunkerEditor destructed");
    
    // Not sure removing is needed ?
    removeAllChildren();
    
    // With the iversed ordef of addition.
    _mainView.deleteAndZero();
    _upperBar.deleteAndZero();
}

//==============================================================================
void DrunkerEditor::paint (Graphics&)  {
    
}
void DrunkerEditor::resized()  {
    Rectangle<int> lb = getLocalBounds();
    _mainView->setBounds(lb.withTop(InternalParam::upperBarHeight));
    {
        Rectangle<int> bb = lb;
        bb.setHeight(InternalParam::upperBarHeight);
        _upperBar->setBounds(bb);
    }
}

void DrunkerEditor::changeListenerCallback(ChangeBroadcaster* source)
{
    repaint();
}


void PatternDnDComponent::mouseDown(const MouseEvent &event){

    _midiFilePath = _drummer.exportMidiFile().getFullPathName();
    
}

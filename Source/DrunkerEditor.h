/*
  ==============================================================================

    DrunkerEditor.h
    Created: 9 Mar 2021 6:59:24pm
    Author:  Hiroyuki Baba

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
//#include "Drunker.h"
#include "PatternView.h"
#include "MainView.h"

class DrunkerProcessor;

class DrunkerEditor  : public AudioProcessorEditor, public ChangeListener
{
public:
    //==============================================================================
    DrunkerEditor (AudioProcessor& p);
    
    ~DrunkerEditor() override;

    //==============================================================================
    void paint (Graphics&) override ;
    void resized() override;
    
    virtual void changeListenerCallback(ChangeBroadcaster* source) override;

private:
    Component::SafePointer<MainView> _mainView;
    Component::SafePointer<Component> _upperBar;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrunkerEditor)
};

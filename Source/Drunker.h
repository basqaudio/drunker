/*
  ==============================================================================

   This file is part of the JUCE examples.
   Copyright (c) 2020 - Raw Material Software Limited

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:                  Drunken
 version:               1.0.0
 vendor:                BASQ AUDIO
 website:               http://basqaudio.com
 description:           Arpeggiator audio plugin.

 dependencies:          juce_audio_basics, juce_audio_devices, juce_audio_formats,
                        juce_audio_plugin_client, juce_audio_processors,
                        juce_audio_utils, juce_core, juce_data_structures,
                        juce_events, juce_graphics, juce_gui_basics, juce_gui_extra
 exporters:             xcode_mac, vs2019

 moduleFlags:           JUCE_STRICT_REFCOUNTEDPOINTER=1

 type:                  AudioProcessor
 mainClass:             Arpeggiator

 useLocalCopy:          1

 pluginCharacteristics: pluginWantsMidiIn, pluginProducesMidiOut, pluginIsMidiEffectPlugin

 END_JUCE_PIP_METADATA

*******************************************************************************/

#pragma once
#include "Drummer.h"
#include "PatternView.h"
#include "DrunkerEditor.h"

//==============================================================================
class DrunkerProcessor  : public AudioProcessor
{
public:

    //==============================================================================
    DrunkerProcessor();
    virtual ~DrunkerProcessor();
    
    Drummer& getDrummer();
    ParameterManager& getParameterManager(){ return *_paramMan.get(); }
    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;

    void releaseResources() override;

    void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi) override;

    using AudioProcessor::processBlock;

    //==============================================================================
    bool isMidiEffect() const override                     { return true; }

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                        { return true; }

    //==============================================================================
    const String getName() const override                  { return "Drunker"; }

    bool acceptsMidi() const override                      { return true; }
    bool producesMidi() const override                     { return true; }
    double getTailLengthSeconds() const override           { return 0; }

    //==============================================================================
    int getNumPrograms() override                          { return 1; }
    int getCurrentProgram() override                       { return 0; }
    void setCurrentProgram (int) override                  {}
    const String getProgramName (int) override             { return {}; }
    void changeProgramName (int, const String&) override   {}

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;

    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    //==============================================================================
    //AudioParameterFloat* speed;
    std::unique_ptr<Drummer> _drummer;
    std::unique_ptr<ParameterManager> _paramMan;
    Logger* _defaultLogger;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrunkerProcessor)
};

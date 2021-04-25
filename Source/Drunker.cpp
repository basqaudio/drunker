/*
  ==============================================================================

    Drunker.cpp
    Created: 9 Mar 2021 7:14:06pm
    Author:  Hiroyuki Baba

  ==============================================================================
*/


#include "Drunker.h"
#include "Drummer.h"
#include "PatternView.h"
#include "DrunkerEditor.h"
#include "Common.h"

DrunkerProcessor::DrunkerProcessor()
    : AudioProcessor (BusesProperties()) // add no audio buses at all
{
    // Seems, logger object needs to be released manually, which is done in destructor.
    _defaultLogger = juce::FileLogger::createDefaultAppLogger("Drunker","drunker.log.txt","Welcome to drunker log");
    juce::Logger::setCurrentLogger(_defaultLogger);
    
    _paramMan.reset(new ParameterManager(*this));
    _drummer.reset(new SequenceDrummer(*this, *_paramMan.get()));
    
    // Currently, dynamic addigion of a parameter from other than processor constructor is not supported
    // Doing so will result in the inconsistency between the underlyigng AU wrapper component.
    // Hence, pre-adding all the parameters which needs to be registered to processor should be added here.
    {
        _paramMan->addParam(new AudioParameterBool("LockOffGrid","LockOffGridValue", true), ParameterManager::LOCK_OFF_GRID_PARAM, true);
    }
    {
        NormalisableRange<float> nr(1,16,1);
        _paramMan->addParam(new AudioParameterFloat("GRID","gridSel", nr, 2), ParameterManager::GLOBAL_GRID_PARAM, true);
    }
    {
        NormalisableRange<float> nr(0,127,1);
        _paramMan->addParam(new AudioParameterFloat("Velocity","velocity", nr, 80), ParameterManager::VELOCITY_PARAM, true);
    }
    {
        NormalisableRange<float> nr(InternalParam::minNudge,InternalParam::maxNudge,1); // TICK unit
        _paramMan->addParam(new AudioParameterFloat("Nudge","nudge", nr, 0), ParameterManager::NUDGE_PARAM, true);
    }
    {
        NormalisableRange<float> nr(-10,10,0.5);
        _paramMan->addParam(new AudioParameterFloat("VZoom","vzoom", nr, 0), ParameterManager::VZOOM_PARAM, true);
    }
    {
        NormalisableRange<float> nr(-10,10,0.5);
        _paramMan->addParam(new AudioParameterFloat("HZoom","hzoom", nr, 0), ParameterManager::HZOOM_PARAM, true);
    }
    {
        _paramMan->addParam(new AudioParameterBool("PlayStop","playStop", true), ParameterManager::PLAYSTOP_PARAM, true);
    }
    {
        NormalisableRange<float> nr(5,990,0.0001);
        _paramMan->addParam(new AudioParameterFloat("Tempo","tempo", nr, InternalParam::defaultTempo), ParameterManager::TEMPO_PARAM, true);
    }
}

DrunkerProcessor::~DrunkerProcessor()
{
    Logger::setCurrentLogger (nullptr);
    delete _defaultLogger;
}

Drummer& DrunkerProcessor::getDrummer() { return *_drummer.get(); }

//==============================================================================
void DrunkerProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    ignoreUnused (samplesPerBlock);
    
    _drummer->prepareToPlay(sampleRate);
}

void DrunkerProcessor::releaseResources() {}

void DrunkerProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    // the audio buffer in a midi effect will have zero channels!
    jassert (buffer.getNumChannels() == 0);
    
    // however we use the buffer to get timing information
    auto numSamples = buffer.getNumSamples();
    
    // Get position info and tempo infos
    AudioPlayHead* ph = this->getPlayHead();
    AudioPlayHead::CurrentPositionInfo cp;
    ph->getCurrentPosition(cp);

    midi.clear();
    _drummer->processBlock(numSamples, cp, midi);

}

AudioProcessorEditor* DrunkerProcessor::createEditor()
{
    //return new GenericAudioProcessorEditor (*this);
    return new DrunkerEditor(*this);
}


void DrunkerProcessor::getStateInformation (MemoryBlock& destData)
{
    //MemoryOutputStream (destData, true).writeFloat (*speed);
    
    MemoryOutputStream  outputStream(destData, false); // Clear existing memory
    outputStream.writeInt64(InternalParam::stateInfoFormatVersion);
    _drummer->serialize(outputStream);
    //_paramMan->serialize(outputStream);
}

void DrunkerProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    //speed->setValueNotifyingHost (MemoryInputStream (data, static_cast<size_t> (sizeInBytes), false).readFloat());
    // return; // Bypass loading
    
    MemoryInputStream  inputStream(data, static_cast<size_t> (sizeInBytes), false);
    int64 stateInfoFormatVersion = inputStream.readInt64();
    if(InternalParam::stateInfoFormatVersion != stateInfoFormatVersion){
        LOG("Non backward compatible state information detected : ", stateInfoFormatVersion, " vs ", InternalParam::stateInfoFormatVersion);
        AlertWindow::showMessageBox(AlertWindow::AlertIconType::WarningIcon,
            "Error",
            FMTS("Non backward compatible state information detected : ", stateInfoFormatVersion, " vs ", InternalParam::stateInfoFormatVersion));
    }else{
        _drummer->deserialize(inputStream);
        //_paramMan->deserialize(inputStream);
    }
}


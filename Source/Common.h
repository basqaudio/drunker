/*
  ==============================================================================

    Common.h
    Created: 17 Mar 2021 2:54:48pm
    Author:  Hiroyuki Baba

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

typedef int64 Duration; // Type for musical duration indepenent from tempo, fs etc...

namespace Durations {
    const static Duration BEAT4  = 2*3*4*5*7*9*11*13 * 16; // this represents up to 13 tuplets of 64th note with integer. 248 4th-notes can be represented with 32bit int, which will be practically enough.
    const static Duration ZERO   = 0;
    const static Duration BEAT1  = BEAT4*4;
    const static Duration BEAT2  = BEAT4*2;
    const static Duration BEAT8  = BEAT4/2;
    const static Duration BEAT16 = BEAT4/4;
    const static Duration BEAT32 = BEAT4/8;
    const static Duration BEAT64 = BEAT4/16;
    const static Duration BEAT128 = BEAT4/32;
    const static Duration TICK   = BEAT4/960; // integer : 18018, correspnding to 1 TICK when 960/quoter note is used as resolution. 480/240/120/60/30 TICKS corresponds to 8-th/16-th/32-th/64-th/128-th note
}


namespace InternalParam {
    const int64 stateInfoFormatVersion = 7; // Used to judge the validity of the state information stored/restored to/from the host. For non-backward compatible state information strucure change, increment it.
    const int _controlAreaWidth = 120;
    const int _hoffset = 12;
    const int _voffset = 8;
    const int _trackHeight = 60;
    const int _widthForBEAT4 = 100; // Screen width corresponding to BEAT4 duration

    const int upperBarHeight = 60;
    const int pianoRollTopTimeRulerHeight = 20;
    const int pianoRollRowHeight = 8; // basic height of piano roll one row
    const int pianoRollHeaderWidth = 30;

    const int scrollBarWidth = 8;
    const int baseUImargin = 4; // Basic margin applied for UIs
    const int loopEndIndWidth = 3;

    const Duration minNudge = -60; //*Durations::TICK; // Minus 64-th note
    const Duration maxNudge =  60; //*Durations::TICK; // Plus 64-th note

    const int nobTitleFontSize = 11;

    const double defaultTempo = 100.0;
};

namespace ColourParam {
    const Colour seqViewBG         = Colours::grey.darker().darker().darker();
    const Colour seqViewGridLine   = Colours::grey.darker().darker();
    const Colour seqViewWhiteKeyBG = Colours::grey.darker().darker().darker(0.2);
    const Colour seqViewBlackKeyBG = Colours::grey.darker().darker().darker();
    const Colour seqHeaderBG       = Colours::grey.darker();
    const Colour timeRulerBG       = Colours::grey.darker().darker();
    const Colour timeRulerGridLine = Colours::grey;
    const Colour loopEndIndColor   = Colours::grey.darker().darker().darker().darker();
    const Colour labelColour       = Colours::lightgrey;
    const Colour upViewBoundColour = Colours::grey.darker().darker();
};


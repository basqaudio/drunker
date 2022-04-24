/*
  ==============================================================================

    Drummer.h
    Created: 9 Mar 2021 1:36:44pm
    Author:  Hiroyuki Baba

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Common.h"
#include "Helper.h"
#include <set>
#include <shared_mutex>

class DrunkerProcessor; // Do not include Drunker.h

class Drummer : public Serializable{
protected:
    float _fs;
    std::atomic_int64_t _time;
    
public:
    
    virtual Duration getLocalTimeInDuration() const = 0;
    
    struct DrumMap{
        int bs;
        int snare;
        int HH_close;
        int HH_open;
        int crash;
        int ride;
        int ride_bell;
    } _map;
    
    struct Pattern : public Serializable {
        virtual ~Pattern(){};
    };
    
    Drummer()  : _fs(0), _time(0) {}
    virtual ~Drummer(){}
    
    virtual void prepareToPlay (double sampleRate){
        _fs = static_cast<float>(sampleRate);
    }
    
    virtual void processBlock(int blockSize, const AudioPlayHead::CurrentPositionInfo& cp,
                              MidiBuffer& midi, bool& updateUI) = 0;

    inline int64 asSamples(Duration duration, double bpm) const {
        return static_cast<int64>(_fs / bpm * 60 * duration / Durations::BEAT4);
    }
    
    inline int asTicks(Duration duration, int ticksPerQuoaterNote){
        // Nomrlly,ticksPerQuoaterNote is 960 at the most in the typical MIDI file, which is much sumaller than resolution of Drummer::Duration.
        return std::round( (double)duration / Durations::BEAT4 * ticksPerQuoaterNote );
    }
    
    inline Duration asDuration(int64 samples, double bpm) const {
        return static_cast<Duration>(samples * bpm * Durations::BEAT4 / _fs / 60);
    }
    
    virtual File exportMidiFile() = 0;
    
    virtual void clearContextInfo() = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Drummer)
};

#define ENABLE_LOCK 1

class SequenceDrummer : public Drummer
{
public:


    struct SequenceEntry{
        int _note;
        Duration _pos;
        Duration _nudge; // Slight timing variance less than 64-th note. _pos shall always include this value(hence it represetns the actual timing of the note), while _nudge is the separated info to judge the on-grid / off-grid property. _pos - _nudge is the value used for on-grid/off-grid property.
        Duration _duration;
        uint8 _vel;
        
        bool operator<(const SequenceEntry& rh) const {
            return _pos < rh._pos;
        }
    };
    
    typedef std::multimap<Duration, SequenceEntry> SeqStorage;
    typedef std::multimap<Duration, SequenceEntry>::iterator SeqStorageItr;
    typedef std::multimap<Duration, SequenceEntry>::const_iterator SeqStorageCItr;
    typedef std::pair<Duration, SequenceEntry> SeqStoragePair;
    
    struct Sequence : public Pattern{
    private:
        SeqStorage _seq;
        std::atomic<Duration> _length; // Now is lock - free
        std::shared_timed_mutex _seqmtx;
    public:
        void readLock(){
#if ENABLE_LOCK
            _seqmtx.lock_shared();
#endif
        }
        void readUnlock(){
#if ENABLE_LOCK
            _seqmtx.unlock_shared();
#endif
        }

        // Write operations which requires write lock(heavy)

        SeqStorageItr insert(const SeqStoragePair& c){
#if ENABLE_LOCK
            std::lock_guard<std::shared_timed_mutex> lg(_seqmtx);
#endif
            return _seq.insert(c);
        }
        void erase(SeqStorageCItr cit){
#if ENABLE_LOCK
            std::lock_guard<std::shared_timed_mutex> lg(_seqmtx);
#endif
            _seq.erase(cit);
        }
        void update(SeqStorageCItr cit, const SequenceEntry& c){
#if ENABLE_LOCK
            std::lock_guard<std::shared_timed_mutex> lg(_seqmtx);
#endif
            SeqStorageItr it = _seq.erase(cit, cit); // This is a trick !. Just convert const it -> normal it.
            it->second = c;
        }
        void updateDuration(SeqStorageCItr cit, Duration d){
#if ENABLE_LOCK
            std::lock_guard<std::shared_timed_mutex> lg(_seqmtx);
#endif
            SeqStorageItr it = _seq.erase(cit, cit); // This is a trick !. Just convert const it -> normal it.
            it->second._duration = d;
        }
        void updateVelocity(SeqStorageCItr cit, uint8 v){
#if ENABLE_LOCK
            std::lock_guard<std::shared_timed_mutex> lg(_seqmtx);
#endif
            SeqStorageItr it = _seq.erase(cit, cit); // This is a trick !. Just convert const it -> normal it.
            it->second._vel = v;
        }
        void swapStorage(SeqStorage& other){
#if ENABLE_LOCK
            std::lock_guard<std::shared_timed_mutex> lg(_seqmtx);
#endif
            _seq.swap(other); // Iterators are remain valid with this.
        }
        
        // Lock free write operation
        void setLength(Duration l){
            _length.store(l);
        }
        
        // Lock (external) need operation ( if only it is used outside the thread provide write operation )
        const SeqStorage& getStorage() const { return _seq; } // Should use read lock and read unlock
        
        // Lock-free read
        Duration getLength() const { return _length.load(); }
        
        // This is not a pure core data, but required for the GUI.
        Duration _gridIntervalDuration;
        
        Sequence():_length(0), _gridIntervalDuration(Durations::BEAT8) {}
        virtual void serialize(MemoryOutputStream& outputStream) override {
            outputStream.writeInt64(_length);
            outputStream.writeInt((int)_seq.size());
            for(auto it = _seq.begin(); it != _seq.end(); ++it){
                outputStream.writeInt(it->second._note);
                outputStream.writeInt64(it->second._pos);
                outputStream.writeInt64(it->second._nudge);
                outputStream.writeInt64(it->second._duration);
                outputStream.writeByte(it->second._vel);
            }
            
            outputStream.writeInt64(_gridIntervalDuration);
        }
        virtual void deserialize(MemoryInputStream& inputStream) override {
            _length = inputStream.readInt64();
            int nSeqEntry = inputStream.readInt();
            _seq.clear();
            for(int i = 0; i < nSeqEntry; ++i){
                int note = inputStream.readInt();
                Duration pos = inputStream.readInt64();
                Duration nudge = inputStream.readInt64();
                Duration duration = inputStream.readInt64();
                uint8 vel = (uint8)inputStream.readByte();
                _seq.insert(SeqStoragePair(pos, {note, pos, nudge, duration, vel}));
            }
            _gridIntervalDuration = inputStream.readInt64();
        }
    };
    
protected:
    
    // Non-serialize target data
    std::atomic<double> _bpm;
    // Normal sequencer drummer
    DrunkerProcessor& _dp;
    ParameterManager& _pm;
    
    Sequence _seq;
    
    struct NoteOff{
        int64 _scheduleTime; // This is "absolute" time, rather than localtime in the loop.
        int note;
        bool operator<(const NoteOff& rh) const {
            return _scheduleTime < rh._scheduleTime;
        }
    };
    
    std::multiset<NoteOff> _noteOffs; // shall be multiset as multiple off will be scheuled at the same timing
    
    // Current on information during recording
    struct ScheduleTime{
        int64 _scheduleTime;
        uint8 _onVel;
        bool _valid;
    };
    ScheduleTime _noteOnsForRec[128];
    
    
    // Serialize target GUI data
    bool _lockOffGrid = true;
    
    //

public:
    Sequence& getSequence(){ return _seq; }
    const bool getLockOffGrid(){ return _lockOffGrid; }
    void setLockOffGrid(bool v){ _lockOffGrid = v; }
    
    SequenceDrummer(DrunkerProcessor& dp, ParameterManager& pm) : _bpm(0.0), _dp(dp), _pm(pm) {
        // https://hirasho.github.io/page/sound/gm-drums.html
        _map = {36, 40, 42, 46, 49, 51, 53}; // Seems YAMAHA style number is used ?
        
        _seq.setLength(Durations::BEAT1*2);
        _seq.insert(SeqStoragePair(Durations::BEAT4*0, {_map.bs, Durations::BEAT4*0, 0, Durations::BEAT16, 127}));
        /*
        _seq._seq.insert({_map.bs, Durations::BEAT4*1, 0, Durations::BEAT16, 127});
        _seq._seq.insert({_map.snare, Durations::BEAT4*1, 0, Durations::BEAT16, 127});
        _seq._seq.insert({_map.bs, Durations::BEAT4*2, 0, Durations::BEAT16, 127});
        _seq._seq.insert({_map.bs, Durations::BEAT4*3, 0, Durations::BEAT16, 127});
        _seq._seq.insert({_map.snare, Durations::BEAT4*3, 0, Durations::BEAT16, 127});
         */
        
        for(int i = 0; i < 128; ++i) _noteOnsForRec[i]._valid=false;
    }
    
    virtual ~SequenceDrummer(){
        
    }
    
    virtual Duration getLocalTimeInDuration() const override {
        Duration timeInDuration = asDuration(_time, _bpm);
        Duration commonLocalTimeSamples = timeInDuration % _seq.getLength();
        return commonLocalTimeSamples;
    }
    
    virtual void processBlock(int blockSize, const AudioPlayHead::CurrentPositionInfo& cp,
                      MidiBuffer& midi, bool& updateUI) override
    {
        if(cp.isPlaying){
            _bpm = cp.bpm;
            *_pm.getFloatParam(ParameterManager::TEMPO_PARAM) = _bpm; // Trigger update GUI if bpm changed(operator=)
        }else{
            _bpm = _pm.getFloat(ParameterManager::TEMPO_PARAM);
        }
                
        if(cp.isPlaying){
            _time = cp.timeInSamples;
        }
        
        
        if(!_pm.getBool(ParameterManager::PLAYSTOP_PARAM)){
            _time = 0;
            return;
        }
        
        // Use the copied value
        int64 time_now = _time;
        double bpm_now = _bpm;
    
    
        {
            Sequence* seq = &_seq;
            
            /// Prorcess Input MIDI messages
            updateUI = false;
            for (const MidiMessageMetadata metadata : midi){
                auto m = metadata.getMessage();
                if(m.isNoteOn()){
                    _noteOnsForRec[m.getNoteNumber()] = {metadata.samplePosition + time_now, m.getVelocity(), true}; // samplePosition in metadata is the offset from the start of this block. See help for getTimeStampgetTimeStamp of the MidiMessage class.
                }else if(m.isNoteOff()){
                    // Search corresponding note ON
                    int note = m.getNoteNumber();
                    if( _noteOnsForRec[note]._valid ){
                        int64 durationSamples = (metadata.samplePosition + time_now) - _noteOnsForRec[note]._scheduleTime;
                        SequenceDrummer::SequenceEntry newEntry;
                        newEntry._note = note;
                        newEntry._pos = mod( asDuration(_noteOnsForRec[note]._scheduleTime, bpm_now), seq->getLength() );
                        newEntry._nudge = 0;
                        newEntry._vel = _noteOnsForRec[note]._onVel;
                        newEntry._duration = asDuration(durationSamples, bpm_now);
                        seq->insert(SequenceDrummer::SeqStoragePair(newEntry._pos,newEntry));
                        _noteOnsForRec[note]._valid = false;
                        updateUI = true;
                    }else{
                        // Invalid states
                    }
                }
            }
            
            /// --- Lock Region ----
            seq->readLock(); // Lock :(
            
            int64 seqLengthSamples = asSamples(seq->getLength(), bpm_now);
            
            // If start time is within this block, then schedule
            //int64 time = time_now; // can be nagative !
            int localTimeSamples = static_cast<int>(mod(time_now, seqLengthSamples)); // assume int64 is no longer needed. pattern local time.
            
            Duration timeDuration = asDuration(time_now, bpm_now);
            Duration localStartTimeDurations = mod(timeDuration, seq->getLength());
            Duration blockSizeDurations = asDuration(blockSize, bpm_now);
            Duration localEndTimeDurations = mod(localStartTimeDurations + blockSizeDurations, seq->getLength()); // can be in the head of next loop
            
            // Firstly, sendNote Off
            
            std::set<NoteOff>::iterator offit = _noteOffs.begin();
            while(offit != _noteOffs.end()){
                int64 offset = offit->_scheduleTime - time_now;
                if(offset < blockSize){ // incl. negative(i.e. passed one...)
                    midi.addEvent (MidiMessage::noteOff  (1, offit->note), jmax(0, static_cast<int>(offset)));
                    offit = _noteOffs.erase(offit);
                }else{
                    break;
                }
            }
            
            SeqStorageCItr its[2][2];
            its[0][0] = seq->getStorage().lower_bound(localStartTimeDurations);
            its[0][1] = localEndTimeDurations > localStartTimeDurations ?
                seq->getStorage().upper_bound(localEndTimeDurations) :
                seq->getStorage().lower_bound(seq->getLength()); // As we should not use the note with pos = seq->_length, use lower limit on purpose.
            if(localEndTimeDurations > localStartTimeDurations){
                its[1][0] = its[1][1] = seq->getStorage().end(); // No need to consider
            }else{
                its[1][0] = seq->getStorage().begin();
                its[1][1] = seq->getStorage().upper_bound(localEndTimeDurations);
            }
            
            for(int s = 0; s < 2; ++s){
                auto it = its[s][0];
                while(it != its[s][1]){
                    const SequenceEntry& e = it->second;
                    Duration d = e._pos;
                    int64 offset = (asSamples(d, bpm_now) - localTimeSamples + seqLengthSamples) % seqLengthSamples;
                    if(offset < blockSize){
                        // In this block !
                        midi.addEvent (MidiMessage::noteOn   (1, e._note, e._vel), static_cast<int>(offset));
                        
                        int64 noteDurationSamples = asSamples(e._duration, bpm_now);
                        if(offset + noteDurationSamples < blockSize){
                            // Send immediately
                            midi.addEvent (MidiMessage::noteOff  (1, e._note), static_cast<int>(offset + noteDurationSamples));
                        }else{
                            // remember it in the NoteOff Buffer
                            _noteOffs.insert({time_now + offset + noteDurationSamples, e._note});
                        }
                    }
                    ++it;
                }
            }
            
            seq->readUnlock();
        }
        

        
        if(!cp.isPlaying){
            // Enable drum beat while not playing.
            _time.fetch_add(blockSize);
        }
    }
    
    virtual File exportMidiFile() override{
        // Generate MIDI file
        File outf = File::getSpecialLocation( File::SpecialLocationType::tempDirectory ).getChildFile( "file.mid" );

        MidiFile            mf;
        MidiMessageSequence ms;

        // http://quelque.sakura.ne.jp/midi_meta.html -> 3 is trakc name
        ms.addEvent( MidiMessage::textMetaEvent( 3, "Drunker Pattern" ) );
        
        {
            Sequence* seq = &_seq; //&_seqlist[pi];
            seq->readLock();
            
            // Pre search the minimum position. If minimum position is negative, then shift all the notes so that they becomes positive time
            Duration minPos = std::numeric_limits<Duration>::max();
            for(auto it = seq->getStorage().begin(); it != seq->getStorage().end(); ++it){
                const SequenceEntry& e = it->second;
                minPos = jmin(e._pos, minPos);
                if(e._pos > seq->getLength()) break;
            }
            
            Duration exportShift = 0;
            if(minPos < 0){
                exportShift = int(std::ceil(-minPos/(double)Durations::BEAT4)) * Durations::BEAT4;
            }
            
            for(auto it = seq->getStorage().begin(); it != seq->getStorage().end(); ++it){
                const SequenceEntry& e = it->second;
                if(e._pos > seq->getLength()) break;
                ms.addEvent( MidiMessage::noteOn(1, e._note, e._vel).withTimeStamp(asTicks(e._pos + exportShift, 960)));
                ms.addEvent( MidiMessage::noteOff(1, e._note).withTimeStamp(asTicks(e._pos + e._duration + exportShift, 960)));
            }
            
            seq->readUnlock();
        }
        
        
        
        mf.setTicksPerQuarterNote( 960 );
        mf.addTrack( ms );

        if ( std::unique_ptr<FileOutputStream> p_os = outf.createOutputStream() ) {
            p_os->setPosition(0); // by default output stream file is open as append mode of existing file.
            p_os->truncate();
            mf.writeTo( *p_os, 0 );
            p_os->flush();
        }
        
        return outf;
    }
    
    virtual void serialize(MemoryOutputStream& outputStream) override {
        _seq.serialize(outputStream);
        outputStream.writeBool(_lockOffGrid);
    }
    
    virtual void deserialize(MemoryInputStream& inputStream) override {
        _seq.deserialize(inputStream);
        _lockOffGrid = inputStream.readBool();
    }
    
public:
    /* --------------------------------- */
    // Utility functions for selections. Not a scope for serialization.
    // Will be reset when GUI is re-constructed
    // TODO : More appropirate data structure design ?
    
    struct Selection{
        //SequenceDrummer::SequenceEntry* _selSeq;
        SeqStorageCItr _selSeqItr; // This is valid only when
        SequenceDrummer::SequenceEntry _mouseDownSnapShot; // snap shot when mouse is down.
        
        bool operator<(const Selection& rh) const{
            return (*_selSeqItr) < (*rh._selSeqItr);
        }
    };
    
    typedef std::list<Selection> Selections;
    typedef std::list<Selection>::iterator SelectionItr;
    
    
    // Utility function for manupirating multiset
    static size_t EraseSelection(Selections& sels, SeqStorageCItr seqItr){
        sels.remove_if([seqItr](const Selection& s){ return s._selSeqItr == seqItr; });
        return 1;
    }
    
    // Utility function for manupirating multiset
    static bool FindSelection(const Selections& sels, SeqStorageCItr seqItr){
        for(const Selection& s : sels){
            if(s._selSeqItr == seqItr) return true;
        }
        return false;
    }
private:
    Selections _sellist;
    std::list< std::function<void(void)> > _cbs;
    
    // This is used for the tempral object while use gesture for e.g. Alt + mouse move.
    SeqStorage _stashedStorage;
    Selections _stashedSellist;
    bool _onFront;
    
    void doCallback(){
        for(auto cb : _cbs){
            cb();
        }
    }
    
    enum NotificationType {
        NoNotification = 0,
        NotifySync
    };
    
public:
    
    /*
     * Clear callbacks and selections
     */
    virtual void clearContextInfo() override {
        _sellist.clear();
        _cbs.clear();
    }
    
    const Selections& getSelection() const {
        return _sellist;
    }
    
    // Make sure to call fixSelection after finalizing the change.
    Selections& getSelection() {
        return _sellist;
    }
    
    void setSelection(const Selections& other, NotificationType notify = NotifySync)
    {
        _sellist = other;
        if(notify == NotifySync) doCallback();
    }
    
    void addCallback(std::function<void(void)> cb)
    {
        _cbs.push_back(cb);
    }
    
    void removeSelected(NotificationType notify = NotifySync){
        for(SelectionItr sit = _sellist.begin(); sit != _sellist.end(); ++sit )
        {
            _seq.erase(sit->_selSeqItr);
        }
        _sellist.clear();
        if(notify == NotifySync) doCallback();
    }
    
    void clearSelection(NotificationType notify = NotifySync){
        _sellist.clear();
        if(notify == NotifySync) doCallback();
    }
    
    void deleteSelection(SeqStorageCItr it, NotificationType notify = NotifySync){
        //_sellist.erase({it}); // This is not a valid way. If the same _pos notes are inserted, all of them will be removed as they are equivalent in terms of _pos.
        EraseSelection(_sellist, it);
        if(notify == NotifySync) doCallback();
    }
    
    void addSelection(SeqStorageCItr it, NotificationType notify = NotifySync){
        _sellist.push_back({it, it->second});
        if(notify == NotifySync) doCallback();
    }
    
    void moveSelectedNote(SelectionItr sit, const SequenceEntry& newEntry, bool keepStash = false, NotificationType notify = NotifySync){
        _seq.erase(sit->_selSeqItr);
        SeqStorageItr newIt = _seq.insert(SeqStoragePair(newEntry._pos, newEntry));
        
        sit->_selSeqItr = newIt;
        if(!keepStash) sit->_mouseDownSnapShot = newIt->second;
    }
    
    void notifySelectionUpdate(NotificationType notify = NotifySync){
        if(notify == NotifySync) doCallback();
    }
    
    bool isSelected(SeqStorageCItr it){
        return FindSelection(_sellist, it);
    }
    
    /**
     * This can be time consumeing if the number of notes is huge. Normally, no worries for that, though.
     * NOTE : Should not be called outside main-thread as we do not read lock the sequence data here.
     */
    void selectIf(std::function<bool(const SequenceEntry& e)> pred, NotificationType notify = NotifySync){

        for(SeqStorageCItr it = _seq.getStorage().begin(); it != _seq.getStorage().end(); ++it)
        {
            if(pred(it->second)){
                addSelection(it, NoNotification);
            }
        }

        if(notify == NotifySync) doCallback();
    }
    
    void unSelectIf(std::function<bool(const SequenceEntry& e)> pred, NotificationType notify = NotifySync){
        _sellist.remove_if([pred](const Selection& s){ return pred(s._selSeqItr->second); });
        if(notify == NotifySync) doCallback();
    }
    
    // Kind of provide parallel 2 verions of sequence and selection data, used for e.g. copy-dragging gesture etc.
    
    /**
     * Make another set (back buffer) of sequence and selection
     * NOTE : Should not be called outside main-thread as we do not read lock the sequence data here.
     */
    void stash(){
        _onFront = true; // Once stash is called current active data is treated as front data. 
        
        _stashedStorage.clear();
        _stashedSellist.clear();
        
        for(SeqStorageCItr it = _seq.getStorage().begin(); it != _seq.getStorage().end(); ++it){
            SequenceDrummer::SequenceEntry dupEntry = it->second;
            SeqStorageItr dupIt = _stashedStorage.insert(SeqStoragePair(dupEntry._pos, dupEntry));
            if( FindSelection(_sellist, it) ){
                _stashedSellist.push_back({dupIt, dupIt->second});
            }
        }
    }
    
    void labelCurrentAs(bool front){
        _onFront = front;
    }
    
    /**
     * Swap the back buffer and front buffer
     */
    void selectFrontBack(bool selectFront){
        if(selectFront == _onFront){
            // true,true || false,false
        }else{
            _seq.swapStorage(_stashedStorage);
            _sellist.swap(_stashedSellist);
            _onFront = !_onFront;
        }
    }
    
    /**
     * Duplicate the selected notes. Select the new duplicated notes.
     */
    void duplicateSelection(){
        Selections newSel;
        for(Selections::iterator sit = _sellist.begin(); sit != _sellist.end(); ++sit){
            SequenceDrummer::SequenceEntry dupEntry = sit->_selSeqItr->second;
            SeqStorageCItr dupIt = _seq.insert(SeqStoragePair(dupEntry._pos, dupEntry));
            newSel.push_back({dupIt, dupIt->second});
        }
        _sellist = std::move(newSel);
    }
    
    /**
     * This needs to be called when mouse drag ends.
     */
    void fixSelection(){
        if(_sellist.size()>0){
            // TODO : eliminate unnsessary update process
            // Update the mouseDown snap shot
            for(Selection& s : _sellist){
                s._mouseDownSnapShot = s._selSeqItr->second;
            }

        }
    }
    
    
    /**
     * This function does not update the  copy of SeuqneceEntry data which is stored internally as the original data when drag started.
     * Make sure to call fixSelection to update the copy of SequenceEntry data  to the latest ones.
     */
    bool dragSelected(float deltaX, int deltaNote){
        if(_sellist.size()>0){
            //Selections newSet;
            for(Selections::iterator sit = _sellist.begin(); sit != _sellist.end(); ++sit){
                float x = sit->_mouseDownSnapShot._pos - sit->_mouseDownSnapShot._nudge + deltaX;
                int note = sit->_mouseDownSnapShot._note + deltaNote;
                int gridId = ::round(x / _seq._gridIntervalDuration); // TODO more intuitive

                Duration gridPos = _seq._gridIntervalDuration * gridId;
                Duration actPos  = gridPos + sit->_mouseDownSnapShot._nudge;
                                
                // No longer need to care about insertion position as it is done automatically be multiset
                {
                    SequenceDrummer::SequenceEntry newEntry = sit->_selSeqItr->second;
                    newEntry._pos = actPos;
                    newEntry._nudge = sit->_mouseDownSnapShot._nudge; // same nudge value
                    newEntry._note = note;
                    
                    // Renew the sequence
                    _seq.erase(sit->_selSeqItr);
                    sit->_selSeqItr = _seq.insert(SeqStoragePair(newEntry._pos, newEntry)); // New iterator
                }
            }
            
            return true;
        }
        return false;
    }
    
    /**
     * This function does not update the  copy of SeuqneceEntry data which is stored internally as the original data when drag started.
     * Make sure to call fixSelection to update the copy of SequenceEntry data  to the latest ones.
     */
    bool copyAndDragSelected(float deltaX, int deltaNote){

        Selections newSel;
        for(Selections::iterator sit = _sellist.begin(); sit != _sellist.end(); ++sit){
            SequenceDrummer::SequenceEntry dupEntry = sit->_selSeqItr->second; // This is lock free as const iterator and iterator validity holds due to carefull mechanism.
            SeqStorageItr dupIt = _seq.insert(SeqStoragePair(dupEntry._pos, dupEntry));
            newSel.push_back({dupIt, dupIt->second});
        }
            
        return true;
    }
    
    /**
     * This function does not update the  copy of SeuqneceEntry data which is stored internally as the original data when drag started.
     * Make sure to call fixSelection to update the copy of SequenceEntry data  to the latest ones.
     */
    bool changeDurationSelected(Duration deltaDuration){
        if(_sellist.size()>0){
            // No need to update sellist as duration change does not affect the order of SequenceEntry.
            for(SelectionItr sit = _sellist.begin(); sit != _sellist.end(); ++sit ){
                _seq.updateDuration(sit->_selSeqItr, jmax(Durations::TICK, sit->_mouseDownSnapShot._duration + deltaDuration));
            }
            
            return true;
        }
        return false;
    }
};


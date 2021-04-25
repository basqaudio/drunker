/*
  ==============================================================================

    Helper.h
    Created: 9 Mar 2021 1:51:27pm
    Author:  Hiroyuki Baba

  ==============================================================================
*/

#pragma once

inline void FMT(String& s) {}

template <class Head, class... Tail>
void FMT(String& s,Head&& head, Tail&&... tail)
{
    s << head << ",";
    FMT(s, std::forward<Tail>(tail)...);
}

// Generalize format function returns String object.
template <class Head, class... Tail>
String FMTS(Head&& head, Tail&&... tail)
{
    String s;
    s << head << ",";
    FMT(s, std::forward<Tail>(tail)...);
    return s;
}

// パラメータパックが空になったら終了
inline void LOG() {}

// ひとつ以上のパラメータを受け取るようにし、
// 可変引数を先頭とそれ以外に分割する
template <class Head, class... Tail>
void LOG(Head&& head, Tail&&... tail)
{
    String s;
    // パラメータパックtailをさらにheadとtailに分割する
    FMT(s, head, std::forward<Tail>(tail)...);
    juce::Logger::getCurrentLogger()->writeToLog(s);
}

class Serializable
{
public:
    Serializable(){};
    virtual ~Serializable(){};
    virtual void serialize(MemoryOutputStream& outputStream) = 0;
    virtual void deserialize(MemoryInputStream& inputStream) = 0;
};

//========= Utility

// Modular with considering of v1 as negative value. v2 shall be positive.
template <class T>
T mod(T v1, T v2){
    // As per C++11 and after, the sign of modular is defined as same as v1.
    // To make is posive, add v2 in that case.
    T m = v1 % v2;
    if(m < T{}) return m+v2;
    return m;
}

//==============================================================================
/*
*/
class ViewConverter
{
    Rectangle<int>& viewScreenBox;
public:
    ViewConverter(Rectangle<int>& refRect) :
    viewScreenBox(refRect), _min_x(-1), _max_x(1), _min_y(-1), _max_y(1)
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.

    }
    
    std::pair<float,float> convToScreen(float x, float y) const {
        std::pair<float, float> ret;
        ret.first = convToScreenX(x);
        ret.second = convToScreenY(y);
        return ret;
    }
    
    float convToScreenX(float x) const {
        
        float ax = viewScreenBox.getWidth() / (_max_x - _min_x);
        float bx = viewScreenBox.getX() - ax * _min_x;
        
        return ax * x + bx;;
    }
    
    float convToScreenY(float y) const {
                
        float ay = viewScreenBox.getHeight() / (_min_y - _max_y); // y-axis is inverted
        float by = viewScreenBox.getY() - ay * _max_y;
        return ay * y + by;
    }
    
    float convToScreenWidth(float w) const {
        float ax = viewScreenBox.getWidth() / (_max_x - _min_x);
        return ax * w;
    }
    
    float convToScreenHeight(float h) const {
        float ay = viewScreenBox.getHeight() / (_min_y - _max_y); // y-axis is inverted
        return ay * h;
    }
    
    std::pair<float,float> convFromScreen(float x, float y) const {
        std::pair<float, float> ret;
        ret.first = convFromScreenX(x);
        ret.second = convFromScreenY(y);
        return ret;
    }
    
    float convFromScreenX(float x) const {

        float ax = viewScreenBox.getWidth() / (_max_x - _min_x);
        float bx = viewScreenBox.getX() - ax * _min_x;

        return (x - bx)/ax;
    }
    
    float convFromScreenY(float y) const {
                
        float ay = viewScreenBox.getHeight() / (_min_y - _max_y); // y-axis is inverted
        float by = viewScreenBox.getY() - ay * _max_y;
        return (y - by)/ay;
    }
    
    float convFromScreenWidth(float w) const {

        float ax = viewScreenBox.getWidth() / (_max_x - _min_x);
        //float bx = viewScreenBox.getX() - ax * _min_x;

        return w/ax;
    }
    
    float convFromScreenHeight(float h) const {
                
        float ay = viewScreenBox.getHeight() / (_min_y - _max_y); // y-axis is inverted
        //float by = viewScreenBox.getY() - ay * _max_y;
        return h/ay;
    }


    void xrange(float min_x, float max_x){
        _min_x = min_x;
        _max_x = max_x;
    }
    
    void yrange(float min_y, float max_y){
        _min_y = min_y;
        _max_y = max_y;
    }
    
    const Rectangle<int>& getRefRect() const {
        return viewScreenBox;
    }
    
private:
    
    float _min_x, _max_x;
    float _min_y, _max_y;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ViewConverter)
};

class ParameterManager
{
    class Callback : public AudioProcessorParameter::Listener {
    public:
        RangedAudioParameter& _p;
        std::function<void(float,bool)> _cb;
        std::function<void(bool)> _cb_gesture;
        bool _isInUserGesture;
        Callback(RangedAudioParameter& p, std::function<void(float,bool)> cb, std::function<void(bool)> cb_gesture=nullptr) : _p(p), _cb(cb), _cb_gesture(cb_gesture), _isInUserGesture(false){
            p.addListener(this);
        }
        virtual ~Callback(){
            _p.removeListener(this);
        }
        virtual void parameterValueChanged (int parameterIndex, float newValue) {
            _cb(_p.convertFrom0to1(newValue), _isInUserGesture);
        }
        virtual void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) {
            if(!_isInUserGesture){
                if(gestureIsStarting) _isInUserGesture = true;
                else{ /* Irregular state */ }
            }else{
                if(gestureIsStarting) { /* Irregular state */ }
                else{ _isInUserGesture = false; }
            }
            if(_cb_gesture) _cb_gesture(gestureIsStarting);
        }
    };
    
    struct WrappedParameter{
        RangedAudioParameter* p;
        bool registeredToAudioProcessor;
        int uniqueId;
    };
    
    Array< Callback* > _callbacks;
    
    //======= Helper feature for managing parameters
    AudioProcessor& _p;
public:
    ParameterManager(AudioProcessor& p) : _p(p){
        for(int i = 0; i < MAX_PARAMS; ++i)
            _wrappedParamList[i] = nullptr;
    }
    ~ParameterManager(){
        releaseParams();
        clearCallbacks();
    }
    
    static const int MAX_PARAMS = 1000;
    // Add parameter unique IDs here, if it needs to be reffered from external
    static const int LOCK_OFF_GRID_PARAM = 0;
    static const int GLOBAL_GRID_PARAM = 1;
    static const int VELOCITY_PARAM = 2;
    static const int NUDGE_PARAM = 3;
    static const int VZOOM_PARAM = 4;
    static const int HZOOM_PARAM = 5;
    static const int PLAYSTOP_PARAM = 6;
    static const int TEMPO_PARAM = 7;
    

    int SEQ_TRACK_PARAM(int trackId, int paramId){ return trackId * 5 + paramId + 100; }
    
    // uniqueId >= 0 < MAX_PARAMS can be used to refer it with O(1) later.
    RangedAudioParameter* addParam(RangedAudioParameter* param, int uniqueId, bool registerToAudioProcessor = false){
        _wrappedParamList[uniqueId] = new WrappedParameter{param, registerToAudioProcessor, uniqueId};
        if(registerToAudioProcessor)
            _p.addParameter(param);
        return param;
    };
    // Validity not chcked to prioritize the speed
    RangedAudioParameter* getParam(int uniqueId){
#ifdef DEBUG
        if(uniqueId < 0 || uniqueId >= MAX_PARAMS || _wrappedParamList[uniqueId] == nullptr ){
            LOG("Error invalid paramter unique ID specified");
            AlertWindow::showMessageBox(AlertWindow::AlertIconType::WarningIcon,
                "Error",
                "Error invalid paramter unique ID specified");
        }
        
#endif
        return _wrappedParamList[uniqueId]->p;
    };
    
    void setNotifyingHost(int uniqueId, float value){
        float newValue01 = _wrappedParamList[uniqueId]->p->convertTo0to1(value);
        _wrappedParamList[uniqueId]->p->setValueNotifyingHost(newValue01);
    }
    
    AudioParameterBool* getBoolParam(int uniqueId){
        return dynamic_cast<AudioParameterBool*>(_wrappedParamList[uniqueId]->p);
    };
    AudioParameterFloat* getFloatParam(int uniqueId){
        return dynamic_cast<AudioParameterFloat*>(_wrappedParamList[uniqueId]->p);
    };
    
    float getFloat(int uniqueId){
        return *getFloatParam(uniqueId);
    }
    bool getBool(int uniqueId){
        return *getBoolParam(uniqueId);
    }
    
    void addCallback(int uniqueId, std::function<void(float,bool)> cb, std::function<void(float)> cb_gesture=nullptr){
        RangedAudioParameter* p = getParam(uniqueId);
        _callbacks.add( new Callback(*p, cb, cb_gesture) );
    };
    
    void clearCallbacks(){
        for(int i = 0; i < _callbacks.size(); ++i){
            delete _callbacks[i];
        }
        _callbacks.clear();
    }
private:
    std::array<WrappedParameter*, MAX_PARAMS> _wrappedParamList;
    void releaseParams(){
        for(int i = 0; i < _wrappedParamList.size(); ++i){
            if(_wrappedParamList[i] != nullptr && (!_wrappedParamList[i]->registeredToAudioProcessor)){
                delete _wrappedParamList[i]->p;
            }
            delete _wrappedParamList[i];
            _wrappedParamList[i] = nullptr;
        }
    }
};



inline Rectangle<int> applyHeightIfTaller(const Rectangle<int>& base, const Rectangle<int>& comp)
{
    Rectangle<int> r = base;
    if(base.getHeight() < comp.getHeight()) r.setHeight(comp.getHeight());
    return r;
}

inline Rectangle<int> applyWidthIfWider(const Rectangle<int>& base, const Rectangle<int>& comp)
{
    Rectangle<int> r = base;
    if(base.getWidth() < comp.getWidth()) r.setWidth(comp.getWidth());
    return r;
}

inline Rectangle<int> applySizeIfBigger(const Rectangle<int>& base, const Rectangle<int>& comp)
{
    Rectangle<int> r = base;
    if(base.getHeight() < comp.getHeight()) r.setHeight(comp.getHeight());
    if(base.getWidth() < comp.getWidth()) r.setWidth(comp.getWidth());
    return r;
}

inline Rectangle<int> removeFromRightRatio(Rectangle<int>& base, float ratio){
    return base.removeFromRight(base.getWidth()*ratio);
}
inline Rectangle<int> removeFromLeftRatio(Rectangle<int>& base, float ratio){
    return base.removeFromLeft(base.getWidth()*ratio);
}


class MessageChannel{
public:
    typedef std::function<void(void)> Func;
    std::list< std::pair<int, Func> > _cbs;
    static MessageChannel& getInstance(){
        static MessageChannel instance;
        return instance;
    }
    static const int ON_LOOP_END_CHANGE = 0;
    void addCallback(int channel, Func cb){
        std::lock_guard<std::mutex> lock(_mtx);
        _cbs.push_back( std::make_pair(channel, cb));
    }
    void notify(int channel){
        std::lock_guard<std::mutex> lock(_mtx);
        for(auto it = _cbs.begin(); it != _cbs.end(); ++it){
            if(it->first == channel){
                it->second();
            }
        }
    }
    void clear(){
        // Clear all the callbacks
        std::lock_guard<std::mutex> lock(_mtx);
        _cbs.clear();
    }
private:
    std::mutex _mtx;
    MessageChannel(){}
    ~MessageChannel(){}
    MessageChannel(const MessageChannel&)= delete;
    MessageChannel& operator=(const MessageChannel&)= delete;

};

/**
 * Utiltity class which realize a mouse dragging with infinite bounds for upper and lower distance.
 * When the mouse goes below or above the reference component's area, then mouse is virtually moves go further with the velocity proporational to the distance of the mouse point and the edge of the reference component, and virtual drag event will be issued periodically.
 */
class InfiniteDragger : public Timer, public MouseListener
{
public:
    class DragKeeperListener{
    public:
        virtual ~DragKeeperListener(){}
        virtual void onPeriodicDragStart() {}
        virtual void onPeriodicDrag(int dragDistanceX, int dragDistanceY) {}
        virtual void onPeriodicDragEnd(int dragDistaneX, int dragDistanceY) {}
    };
    
protected:
    bool _outsideX = false;
    bool _outsideY = false;
    int _scrollStrengthX = 0;
    int _scrollStrengthY = 0;
    int _dragStartInRefX = 0;
    int _dragStartInRefY = 0;
    
    double _culmativeAutoScrollX = 0;
    double _culmativeAutoScrollY = 0;
    
    // Used for normal scrolling inside the reference component
    int _lastDragDistanceX;
    int _lastDragDistanceY;
    
    Array<DragKeeperListener*> _listeners;
    const Component* _refCompX;
    const Component* _refCompY;
    
    const int _timerPeriodMS = 100;
    
    // Base verlocity is "distance between mouse position and reference area edge" per 1 second.
    // Followign value indicates the magnification for the base velocity.
    const double _dragSpeedCoeff = 5.0;
public:
    InfiniteDragger(const Component* refCompX, const Component* refCompY) : _refCompX(refCompX), _refCompY(refCompY){
        startTimer(_timerPeriodMS);
    }
    
    ~InfiniteDragger(){
        stopTimer();
    }
    
    void addListener(DragKeeperListener* l){
        _listeners.add(l);
    }
    
    void removeListener(DragKeeperListener* l){
        _listeners.removeAllInstancesOf(l);
    }
    
    virtual void mouseDown(const MouseEvent &event) override {
        _dragStartInRefX = _refCompX->getLocalPoint(event.originalComponent, event.getMouseDownPosition()).toInt().x;
        _dragStartInRefY = _refCompY->getLocalPoint(event.originalComponent, event.getMouseDownPosition()).toInt().y;
        
        for(int i = 0; i < _listeners.size(); ++i)
            _listeners[i]->onPeriodicDragStart();
    }
    virtual void mouseUp(const MouseEvent &event) override {
        int dragX = _lastDragDistanceX + _culmativeAutoScrollX;
        int dragY = _lastDragDistanceY + _culmativeAutoScrollY;
        _outsideX = false;
        _outsideY = false;
        _scrollStrengthX = 0;
        _scrollStrengthY = 0;
        _culmativeAutoScrollX = 0;
        _culmativeAutoScrollY = 0;
        for(int i = 0; i < _listeners.size(); ++i)
            _listeners[i]->onPeriodicDragEnd(dragX, dragY);
    }
    virtual void mouseDrag(const MouseEvent &event) override{
        
        Point<int> pRefX = _refCompX->getLocalPoint(event.originalComponent, event.position.toInt()).toInt();
        Rectangle<int> rlbX = _refCompX->getLocalBounds();
        
        Point<int> pRefY = _refCompY->getLocalPoint(event.originalComponent, event.position.toInt()).toInt();
        Rectangle<int> rlbY = _refCompY->getLocalBounds();
        
        _lastDragDistanceX = event.getDistanceFromDragStartX();
        _lastDragDistanceY = event.getDistanceFromDragStartY();
        
        
        if(rlbX.getX() <= pRefX.x && pRefX.x <= rlbX.getRight()){
            // Inside
            _outsideX = false;
            _scrollStrengthX = 0;
        }else{
            _outsideX = true;
            _scrollStrengthX = rlbX.getX() > pRefX.x ?
                pRefX.x - rlbX.getX() : // < 0
                pRefX.x - rlbX.getRight(); // > 0
        }
        
        if(rlbY.getY() <= pRefY.y && pRefY.y <= rlbY.getBottom()){
            // Inside
            _outsideY = false;
            _scrollStrengthY = 0;
        }else{
            _outsideY = true;
            _scrollStrengthY = rlbY.getY() > pRefY.y ?
                pRefY.y - rlbY.getY() : // < 0
                pRefY.y - rlbY.getBottom(); // > 0
        
        }
        
        publishDragEvent();
    }

    virtual void timerCallback() override {
        double alpha = _timerPeriodMS / 1000.0 * _dragSpeedCoeff;
        _culmativeAutoScrollX += _scrollStrengthX * alpha;
        _culmativeAutoScrollY += _scrollStrengthY * alpha;
        
        if(_outsideX || _outsideY) publishDragEvent();
    }
    
    void publishDragEvent(){
        int dragX = _lastDragDistanceX + _culmativeAutoScrollX;
        int dragY = _lastDragDistanceY + _culmativeAutoScrollY;
        
        for(int i = 0; i < _listeners.size(); ++i)
            _listeners[i]->onPeriodicDrag(dragX, dragY);
    }
};

inline float InDB(float v){
    return 10.0 * log10(v);
}

inline float NonDB(float v){
    return pow(10.0, v/10.0);
}

inline void setBoundsForceResized(Component* c, const Rectangle<int>& newRect){
    Rectangle<int> oldRect = c->getBounds();
    c->setBounds(newRect);
    if(oldRect == newRect){
        c->resized(); // Invoke manually
    }
}

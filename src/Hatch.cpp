#include "Hatch.h"
#include <lajopindefs.h> 
#include <lajotimedefs.h>

#define _DEBUG_
#define _VERBOSE_
//#define _TEST_POS_ERR_
#define USING_DC_PERCENT      true

//#define _TEST_

Hatch::Hatch(){}

void Hatch::init(){
    pinMode(retractpin, OUTPUT);
    pinMode(extendpin, OUTPUT);
    pinMode(hatchspeedpin, OUTPUT);
    pinMode(outerlimitpin, INPUT_PULLUP);
    pinMode(innerlimitpin, INPUT_PULLUP);
    pinMode(hall1pin, INPUT);
    pinMode(hall2pin, INPUT);

    digitalWrite(extendpin, LOW);
    digitalWrite(retractpin, LOW);
    digitalWrite(hatchspeedpin, LOW);

	ESP32Encoder::useInternalWeakPullResistors = puType::NONE;
	// use pin 19 and 18 for the first encoder
    _hallEncoder = ESP32Encoder();
	_hallEncoder.attachHalfQuad(hall1pin, hall2pin);
    _hallEncoder.clearCount();
    _targetCount = 0;

    _armPosZeroed = false;

    isHatchFullyClosed();
    isHatchFullyOpen();
    #ifdef _VERBOSE_
        Serial.println("Setup completed. Closed: " + String(_isClosedArmOut) + " Open: " + String(_isOpenArmIn));
    #endif

}

Hatch::~Hatch()
{
}

void Hatch::setSlowSpeed(boolean slowSpeed){
    #ifdef _DEBUG_
        Serial.println("setSlowSpeed: " + String(slowSpeed));
    #endif
    slowSpeed ? digitalWrite(hatchspeedpin, LOW) : digitalWrite(hatchspeedpin, HIGH);
}
int64_t Hatch::getTargetCount(){
       return _targetCount;
}
Hatch::HatchCommand Hatch::setTargetCount(int64_t targetCount){
    #ifdef _DEBUG_
        Serial.println("setTargetCount: " + String(targetCount) + " was: " + String(_targetCount));
    #endif
    HatchCommand retval;
    if(targetCount<0 || targetCount > hatch_hall_count_when_hatch_open || (!isMoving() && targetCount == getOutCount())) {
        #ifdef _VERBOSE_
            Serial.println("setTargetCount: isMoving " + String(isMoving()) + " outCount: " + String(getOutCount()));
        #endif
        retval = HatchCommand::NO_OP;  
    } else {
        if(targetCount==0 || getOutCount() > targetCount){
            retval = HatchCommand::CLOSE;
            #ifdef _VERBOSE_
                Serial.println("setTargetCount: dir closing");
            #endif
        } else {
            retval = HatchCommand::OPEN;
            #ifdef _VERBOSE_
                Serial.println("setTargetCount: dir opening");
            #endif
        }
        _targetCount = targetCount;
        #ifdef _DEBUG_
            Serial.println("setTargetCount at the end: " + String(_targetCount));
        #endif
    }
    return retval;
}
Hatch::HatchCommand Hatch::setTargetMm(double targetMm){
    return setTargetCount(targetMm/hatch_mm_per_count);
}
Hatch::HatchCommand Hatch::changeTargetSteps(int steps){
    //int64_t before = getOutCount();     // Change from prev target or outCount?
    int64_t before = _targetCount;
    int64_t delta = steps*steplengthpulses;
    int64_t newTarget = before + delta;
    #ifdef _DEBUG_
        Serial.println("changeTargetSteps: " + String(steps) + "count changing by: " + String(delta) + 
        " from: " + String(before));
    #endif

    if(newTarget < 0){
        newTarget = 0;
        #ifdef _DEBUG_
            Serial.println("changeTargetSteps: Targetting 0.");
        #endif
    } else if(newTarget > hatch_hall_count_when_hatch_open){
        newTarget = hatch_hall_count_when_hatch_open;
        #ifdef _DEBUG_
            Serial.println("changeTargetSteps: Targetting fully open.");
        #endif
    }

    return setTargetCount(newTarget); 
}
Hatch::HatchCommand Hatch::changeTargetMm(double mm){
    //int64_t before = getOutCount();     // Change from prev target or outCount?
    int64_t before = _targetCount;
    int64_t delta = mm/hatch_mm_per_count;
    int64_t newTarget = before + delta;

    #ifdef _DEBUG_
        Serial.println("changeTargetMm: " + String(mm) + "count changing by: " + String(delta) + 
        " from: " + String(before));
    #endif

    if(newTarget < 0){
        newTarget = 0;
        #ifdef _DEBUG_
            Serial.println("changeTargetMm: Targetting 0.");
        #endif
    } else if(newTarget > hatch_hall_count_when_hatch_open){
        newTarget = hatch_hall_count_when_hatch_open;
        #ifdef _DEBUG_
            Serial.println("changeTargetMm: Targetting fully open.");
        #endif
    }

    return setTargetCount(newTarget); 
}

boolean Hatch::isTargetReached(){
    boolean retval = false;
    int64_t outCountNow = getOutCount();
    if(isClosing()){
        
        if((outCountNow <= _targetCount && _targetCount > hatch_hall_count_when_hatch_closed)  // Request is not to close fully
                || isHatchFullyClosed()){   // Request is to close fully
            retval = true;
        } else if (isTargetApproaching()){ // Not in target yet but close -> slow down
            setSlowSpeed(true);
            #ifdef _DEBUG_
                Serial.println("isTargetReached() closing approaching.");    
            #endif
        }
    }
    if(isOpening()){
        if((outCountNow >= _targetCount && _targetCount < hatch_hall_count_when_hatch_open) // Request is not to open fully
        || isHatchFullyOpen()) { // Request is to open fully
            retval = true;
        } else if (isTargetApproaching()){ // Not in target yet but close -> slow down
            setSlowSpeed(true);
            #ifdef _DEBUG_
                Serial.println("isTargetReached() opening approaching.");    
            #endif
        }
    }
    #ifdef _VERBOSE_
        Serial.println("isTargetReached() returning: " + String(retval) +" outCountNow: " + String(outCountNow) + " _targetCount: " + String(_targetCount));    
    #endif

    return retval;
}


boolean Hatch::startClosing(boolean slowSpeed) // Close hatch, ie. extend arm
{
    #ifdef _VERBOSE_
        Serial.println("startClosing. Already in: " + String(_isClosedArmOut));
    #endif

    if (!_isClosedArmOut) {
        if(!isClosing()){

            setSlowSpeed(slowSpeed);

            #ifdef _VERBOSE_
                int64_t outCountBefore = _hallEncoder.getCount();
                Serial.print("Start moving out i.e. extending i.e. closing hatch. Slow speed: ");
                Serial.print(isSlowSpeedOn());
                Serial.print(" targetCount: ");
                Serial.print(_targetCount);
                Serial.print(" outCount before: ");
                Serial.println(outCountBefore);
            #endif

            digitalWrite(retractpin, LOW); // make sure retracting is not on!
            digitalWrite(extendpin, HIGH); // make sure retracting is not on!

       }
        return true;
    } else {
        #ifdef _VERBOSE_
            Serial.println("Already closed (arm fully out), Count: " + String(_hallEncoder.getCount()));
        #endif

        return false;
    }
}
boolean Hatch::startOpening(boolean slowSpeed) // Close hatch, ie. extend arm
{
    #ifdef _VERBOSE_
        Serial.println("startOpening. Already in: " + String(_isOpenArmIn));
    #endif

    if (!_isOpenArmIn) {
        if(!isOpening()){

            setSlowSpeed(slowSpeed);

            #ifdef _VERBOSE_
                int64_t outCountBefore = _hallEncoder.getCount();
                Serial.print("Start moving in i.e. retracting i.e. opening hatch. Slow speed: ");
                Serial.print(isSlowSpeedOn());
                Serial.print(" targetCount: ");
                Serial.print(_targetCount);
                Serial.print(" outCount before: ");
                Serial.println(outCountBefore);
            #endif

            digitalWrite(extendpin, LOW); // make sure retracting is not on!
            digitalWrite(retractpin, HIGH); // make sure retracting is not on!

        }
        return true;
    } else {
        #ifdef _VERBOSE_
            Serial.println("Already closed (arm fully out), stepcount: " + String(_hallEncoder.getCount()));
        #endif

        return false;
    }
}

void Hatch::stop(boolean resetTarget){     // Stops hatch movement and sets _targetCount to that of stopping time actual
        int64_t currentOutCount = _hallEncoder.getCount();
        #ifdef _DEBUG_
            Serial.println("stop(). TargetCount: " + String(_targetCount) + " Count now: " + String(currentOutCount) +
            " reset after: " + String(resetTarget));
        #endif

        digitalWrite(extendpin, LOW); // make sure retracting is not on!
        digitalWrite(retractpin, LOW); // make sure retracting is not on!

        if(resetTarget){
            _targetCount = currentOutCount;
        }

        #ifdef _TEST_POS_ERR_
            if(_testPosErr == 0){
                _testPosErr = 1;
                _hallEncoder.setCount(currentOutCount + testposerr);
                Serial.println("TEST POS ERR +.");
            } else if(_testPosErr == 1){
                Serial.println("TEST POS ERR 1 => do nothing but next time -.");
                _testPosErr = -1;
            } else if(_testPosErr == 2){
                Serial.println("TEST POS ERR 2 => do nothing but next time +.");
                _testPosErr = 0;
            } else if(_testPosErr == -1){
                _testPosErr = 2;
                _hallEncoder.setCount(currentOutCount - testposerr);
                Serial.println("TEST POS ERR -.");
            } else {
                Serial.println("TEST POS ERR WTF!");
            }
        #endif
}

boolean Hatch::isHatchFullyOpen()
{
    _isOpenArmIn = !digitalRead(innerlimitpin); // true, when on motor inner limit

    if(_isOpenArmIn && (isOpening() || !isMoving())){
        _armPosZeroed = true;
        _hallEncoder.setCount(hatch_hall_count_when_hatch_open);
        _targetCount = hatch_hall_count_when_hatch_open;
        #ifdef _VERBOSE_
            Serial.println("isHatchFullyOpen() returning:" + String(_isOpenArmIn));    
        #endif

    }

    return _isOpenArmIn;
}

boolean Hatch::isHatchFullyClosed()
{
    _isClosedArmOut = !digitalRead(outerlimitpin); // true, when on motor outer limit

    if(_isClosedArmOut && (isClosing() || !isMoving())){    // zero if closing or not moving at all. Do not zero when opening
        _armPosZeroed = true;
        _hallEncoder.setCount(hatch_hall_count_when_hatch_closed);
        _targetCount = hatch_hall_count_when_hatch_closed;
    }
    //   #ifdef _VERBOSE_
    //     Serial.println("isHatchFullyClosed() returning:" + String(_isClosedArmOut));    
    //   #endif
    return _isClosedArmOut;
}

int64_t Hatch::getOutCount(){
    return _hallEncoder.getCount();
}

float Hatch::getOpeningInMm(){
    return _hallEncoder.getCount()*hatch_mm_per_count;
}


String Hatch::getStatusString()
{

    boolean fullyClosed = isHatchFullyClosed();
    boolean fullyOpen = isHatchFullyOpen();
    boolean closing = isClosing();
    boolean opening = isOpening();
    return "{\"outcount\":" + String(getOutCount()) + 
                ",\"target\":" + String(_targetCount) +    
                ",\"posmm\":" + String(getOpeningInMm(),0) +    
                ",\"closed\":" + int(fullyClosed) +
                ",\"open\":" + int(fullyOpen) +
                ",\"zeroed\":" + int(_armPosZeroed) +
                ",\"opening\":" + int(opening) +
                ",\"closing\":" + int(closing) +
                // ",\"fault\":" + int(getHatchFault()) +
                ",\"lspd\":" + String(isSlowSpeedOn()) +
            "}";
}

void Hatch::update()
{
    //updateAverageRPM(millis()-_lastUpdateTime);
    _lastUpdateTime = millis();
}

String Hatch::getName(){
    return "hatch";
}

boolean Hatch::isMoving(){
    return isClosing() || isOpening();
}
boolean Hatch::isClosing(){ 
    return digitalRead(extendpin);
}
boolean Hatch::isOpening(){
    return digitalRead(retractpin);
}

void Hatch::testingOutcountManipulate(){
    if(isClosing()){
        _hallEncoder.setCount(_hallEncoder.getCount()-1);
    } else if(isOpening()){
        _hallEncoder.setCount(_hallEncoder.getCount()+1);
    }
}

boolean Hatch::repostionHatchToTarget(){
    int64_t currentOutCount = _hallEncoder.getCount();
    boolean retval = false;
    #ifdef _DEBUG_
        Serial.println("repostionHatchToTarget. TargetCount: " + String(_targetCount) + " Count now: " +String(currentOutCount) );
    #endif
    if(currentOutCount >= _targetCount - hatchrepositiontolerance && currentOutCount <= _targetCount + hatchrepositiontolerance){
        #ifdef _DEBUG_
            Serial.println("repostionHatchToTarget within tolerance, done!");
        #endif
        stop(false);
        retval = true;
    } else if(_targetCount>currentOutCount){
        #ifdef _DEBUG_
            Serial.println("repostionHatchToTarget. opening.");
        #endif
        startOpening(true);
    } else {
        #ifdef _DEBUG_
            Serial.println("repostionHatchToTarget. closing.");
        #endif
        startClosing(true);
    }

    return retval;
}

boolean Hatch::getAlarm(){return false;}
boolean Hatch::isSlowSpeedOn(){return !digitalRead(hatchspeedpin);}

boolean Hatch::isTargetApproaching(){
    boolean retval = false;
    if(_armPosZeroed){
        int64_t outCountNow = getOutCount();   
        if ( (outCountNow > (_targetCount-hatchslowdownadvance))   && (outCountNow < (_targetCount+hatchslowdownadvance)) ){
            retval = true;
            #ifdef _VERBOSE_
                Serial.println("isTargetApproaching. Yes it is.");
            #endif
        }
    }
    return retval;
}

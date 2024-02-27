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
    pinMode(retractpwmpin, OUTPUT);
    pinMode(extendpwmpin, OUTPUT);
    // pinMode(hatchpwmpin, OUTPUT);
    pinMode(hatchspeedpin, OUTPUT);
    pinMode(outerlimitpin, INPUT_PULLUP);
    pinMode(innerlimitpin, INPUT_PULLUP);
    pinMode(hall1pin, INPUT);
    pinMode(hall2pin, INPUT);

    _em241pwmExtend = new ESP32_FAST_PWM(extendpwmpin, em241pwmfreq, 0.0f, hatchextendpwmchannel, hatchpwmresolution);
    _em241pwmRetract = new ESP32_FAST_PWM(retractpwmpin, em241pwmfreq, 0.0f, hatchretractpwmchannel, hatchpwmresolution);

    if (_em241pwmExtend){
        boolean setSuccess = _em241pwmExtend->setPWM(extendpwmpin, em241pwmfreq, 0.0f);
        #ifdef _VERBOSE_
            Serial.println("Setup: _em241pwmExtend exists, setPWM fine: " + String(setSuccess));
        #endif
    }
    if (_em241pwmRetract){
        boolean setSuccess = _em241pwmRetract->setPWM(retractpwmpin, em241pwmfreq, 0.0f);
        #ifdef _VERBOSE_
            Serial.println("Setup: _em241pwmRetract exists, setPWM fine: " + String(setSuccess));
        #endif
    }

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

void Hatch::setSpeed(float speed)
{
    if(speed>=0.0f && speed <=100.0f){
        _speed = speed;
    } else {
        Serial.println("Invalid speed setting: " + String(speed));  
    }
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
    return setTargetCount(_targetCount + steps*steplengthpulses); 
}
Hatch::HatchCommand Hatch::changeTargetMm(double mm){
    #ifdef _DEBUG_
        Serial.println("changeTargetMm: " + String(mm) + "count changing by: " + String((mm/hatch_mm_per_count)));
    #endif
    return setTargetCount(_targetCount + mm/hatch_mm_per_count); 
}

boolean Hatch::isTargetReached(){
    boolean retval = false;
    int64_t outCountNow = getOutCount();
    if(isClosing()){
        
        if((outCountNow <= _targetCount && _targetCount > hatch_hall_count_when_hatch_closed)  // Request is not to close fully
                || isHatchFullyClosed()){   // Request is to close fully
            retval = true;
        } else if (_armPosZeroed && outCountNow < (_targetCount+hatchslowdownadvance)){ // Not in target yet but close -> slow down
            //_em241pwmExtend->setPWM_DCPercentage_manual(extendpwmpin, hatchapproachspeed);
            #ifdef _DEBUG_
                Serial.println("isTargetReached() closing approaching.");    
            #endif
        }
    }
    if(isOpening()){
        if((outCountNow >= _targetCount && _targetCount < hatch_hall_count_when_hatch_open) // Request is not to open fully
        || isHatchFullyOpen()) { // Request is to open fully
            retval = true;
        } else if (_armPosZeroed && outCountNow > (_targetCount-hatchslowdownadvance)){ // Not in target yet but close -> slow down
            //_em241pwmRetract->setPWM_DCPercentage_manual(retractpwmpin, hatchapproachspeed);
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


boolean Hatch::startClosing() // Close hatch, ie. extend arm
{
    #ifdef _VERBOSE_
        Serial.println("startClosing. Already in: " + String(_isClosedArmOut));
    #endif

    if (!_isClosedArmOut) {
        if(!isClosing()){
            // _targetCount = outCountBefore + steps;
            #ifdef _VERBOSE_
                int64_t outCountBefore = _hallEncoder.getCount();
                Serial.print("Start moving out i.e. extending i.e. closing hatch. Speed: ");
                Serial.print(_speed);
                Serial.print(" targetCount: ");
                Serial.print(_targetCount);
                Serial.print(" outCount before: ");
                Serial.println(outCountBefore);
            #endif

            boolean setSuccess = _em241pwmRetract->setPWM_DCPercentage_manual(retractpwmpin,0.0f);           // make sure retracting is not on!
            // #ifdef _VERBOSE_
            //     Serial.println("_em241pwmRetract->setPWM_DCPercentage_manual returned: " + String(setSuccess));
            // #endif

            setSuccess = _em241pwmExtend->setPWM_DCPercentage_manual(extendpwmpin,_speed);
            // #ifdef _VERBOSE_
            //     Serial.println("_em241pwmExtend->setPWM_DCPercentage_manual returned: " + String(setSuccess));
            // #endif
       }
        return true;
    } else {
        #ifdef _VERBOSE_
            Serial.println("Already closed (arm fully out), Count: " + String(_hallEncoder.getCount()));
        #endif

        return false;
    }
}
boolean Hatch::startOpening() // Close hatch, ie. extend arm
{
    #ifdef _VERBOSE_
        Serial.println("startOpening. Already in: " + String(_isOpenArmIn));
    #endif

    if (!_isOpenArmIn) {
        if(!isOpening()){
            // _targetCount = outCountBefore + steps;
            #ifdef _VERBOSE_
                int64_t outCountBefore = _hallEncoder.getCount();
                Serial.print("Start moving in i.e. retracting i.e. opening hatch. Speed: ");
                Serial.print(_speed);
                Serial.print(" targetCount: ");
                Serial.print(_targetCount);
                Serial.print(" outCount before: ");
                Serial.println(outCountBefore);
            #endif

            boolean setSuccess = _em241pwmExtend->setPWM_DCPercentage_manual(extendpwmpin,0.0f);           // make sure extending is not on!
            //  #ifdef _VERBOSE_
            //     Serial.println("_em241pwmExtend->setPWM_DCPercentage_manual returned: " + String(setSuccess));
            // #endif
           setSuccess = _em241pwmRetract->setPWM_DCPercentage_manual(retractpwmpin,_speed);
            // #ifdef _VERBOSE_
            //     Serial.println("_em241pwmRetract->setPWM_DCPercentage_manual returned: " + String(setSuccess));
            // #endif
        }
        return true;
    } else {
        #ifdef _VERBOSE_
            Serial.println("Already closed (arm fully out), stepcount: " + String(_hallEncoder.getCount()));
        #endif

        return false;
    }
}

void Hatch::stop(){     // Stops hatch movement and sets _targetCount to that of stopping time actual
        int64_t currentOutCount = _hallEncoder.getCount();
        #ifdef _DEBUG_
            Serial.println("stop(). TargetCount: " + String(_targetCount) + " Count now: " + String(currentOutCount) );
        #endif
        _em241pwmRetract->setPWM_DCPercentage_manual(retractpwmpin,0.0f);
        _em241pwmExtend->setPWM_DCPercentage_manual(extendpwmpin,0.0f);

        _targetCount = currentOutCount;

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
    return "{\"outcount\":" + String(getOutCount()) + 
                ",\"target\":" + String(_targetCount) +    
                ",\"posmm\":" + String(getOpeningInMm(),0) +    
                ",\"closed\":" + int(isHatchFullyClosed()) +
                ",\"open\":" + int(isHatchFullyOpen()) +
                ",\"zeroed\":" + int(_armPosZeroed) +
                ",\"opening\":" + int(isOpening()) +
                ",\"closing\":" + int(isClosing()) +
                // ",\"fault\":" + int(getHatchFault()) +
                ",\"spd\":" + String(_speed) +
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
    // #ifdef _VERBOSE_
    //     Serial.println("---------------------- isClosing ------------------------------");
    //     printPWMInfo(_em241pwmExtend);
    // #endif
    return _em241pwmExtend->getActualDutyCycle() > 0.0f ? true : false;
}
boolean Hatch::isOpening(){
    // #ifdef _VERBOSE_
    //     Serial.println("---------------------- isOpening ------------------------------");
    //     printPWMInfo(_em241pwmRetract);
    // #endif
    return _em241pwmRetract->getActualDutyCycle() > 0.0f ? true : false;
}

void Hatch::printPWMInfo(ESP32_FAST_PWM* PWM_Instance)
{
  Serial.print("Actual data: pin = ");
  Serial.print(PWM_Instance->getPin());
  Serial.print(", PWM DC = ");
  Serial.print(PWM_Instance->getActualDutyCycle());
  Serial.print(", PWMPeriod = ");
  Serial.print(PWM_Instance->getPWMPeriod());
  Serial.print(", PWM Freq (Hz) = ");
  Serial.println(PWM_Instance->getActualFreq(), 4);
  Serial.println("----------------------------------------------------");
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
        stop();
        retval = true;
    } else if(_targetCount>currentOutCount){
        #ifdef _DEBUG_
            Serial.println("repostionHatchToTarget. opening.");
        #endif
        setSpeed(hatchrepositionspeed);
        startOpening();
    } else {
        #ifdef _DEBUG_
            Serial.println("repostionHatchToTarget. closing.");
        #endif
        setSpeed(hatchrepositionspeed);
        startClosing();
    }

    return retval;
}

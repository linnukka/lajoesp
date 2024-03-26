#ifndef HATCH_H
#define HATCH_H

#pragma once

#include <Arduino.h>
#include <SerialPrinter.h>
//#include "ESP32_FastPWM.h"
#include <ESP32Encoder.h>
#include <StatusReportingObject.h>


class Hatch: public StatusReportingObject
{
public:
    enum class HatchCommand : byte {
        NO_OP,
        OPEN,
        CLOSE,
        OP_STEPS,
        OP_MM,
        CL_STEPS,
        CL_MM,
        SET_POS,
        SHAKE,
        SHUTDOWN,        
        CANCEL_SHUTDOWN,        
    };


    Hatch();
    ~Hatch();
    void init();
    // void close(int64_t steps);
    // void close(int64_t steps, boolean pollSwitch);
    // void open(int64_t steps);
    // void open(int64_t steps, boolean pollSwitch);
    boolean startClosing(boolean slowSpeed); // return true if started, false if already closed
    boolean startOpening(boolean slowSpeed); // return true if started, false if already open
    void stop(boolean resetTarget);    
    // void openFully(boolean pollSwitch);
    int64_t getOutCount();
    float getOpeningInMm();

    boolean isHatchFullyOpen();
    boolean isHatchFullyClosed();
    void setSlowSpeed(boolean slowSpeed);
    String getStatusString();
    String getName();
    void update();
    int64_t getTargetCount();
    HatchCommand setTargetCount(int64_t targetCount);
    HatchCommand setTargetMm(double targetMm);
    HatchCommand changeTargetSteps(int steps);
    HatchCommand changeTargetMm(double mm);
    boolean isMoving();
    boolean isClosing();
    boolean isOpening();
    boolean isTargetReached();
    //void outputPwmForTesting(int speed);
    void testingOutcountManipulate();
    boolean repostionHatchToTarget();
    boolean getAlarm();
    boolean isSlowSpeedOn();
    boolean isTargetApproaching();
    //boolean getHatchFault();

private:
    int _stepInLen;
    int _stepOutLen;
    int _testPosErr = 0;
    //int64_t _outCount;
    // float _dutyCycle;
    boolean _isOpenArmIn;
    boolean _isClosedArmOut;
    boolean _armPosZeroed = false;
    int64_t _targetCount;

    ESP32Encoder _hallEncoder;
};

#endif
#ifndef HATCH_H
#define HATCH_H

#pragma once

#include <Arduino.h>
#include <SerialPrinter.h>
#include "ESP32_FastPWM.h"
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
    };


    Hatch();
    ~Hatch();
    void init();
    // void close(int64_t steps);
    // void close(int64_t steps, boolean pollSwitch);
    // void open(int64_t steps);
    // void open(int64_t steps, boolean pollSwitch);
    boolean startClosing(); // return true if started, false if already closed
    boolean startOpening(); // return true if started, false if already open
    void stop();    
    // void openFully(boolean pollSwitch);
    int64_t getOutCount();
    float getOpeningInMm();

    boolean isHatchFullyOpen();
    boolean isHatchFullyClosed();
    void setSpeed(float speed);
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

    float _speed = 100.0f;
    ESP32_FAST_PWM* _em241pwmRetract;
    ESP32_FAST_PWM* _em241pwmExtend;
    ESP32Encoder _hallEncoder;
    void printPWMInfo(ESP32_FAST_PWM* PWM_Instance);

};

#endif
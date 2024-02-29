#ifndef ROTATOR_H
#define ROTATOR_H

#pragma once

#include <Arduino.h>
#include <StatusReportingObject.h>
#include <FunctionalInterrupt.h>

class Rotator: public StatusReportingObject
{
public:
    Rotator();
    Rotator(int pin, int pinMode, int minSpeed, String name);
    Rotator(int pin, int pinMode, int minSpeed, int pulsesPerRev, String name);
    void init(int pin, int pinMode, int minSpeed, String name);
    void init(int pin, int pinMode, int minSpeed, int pulsesPerRev, String name);
    ~Rotator();
    void rotatorIsr(void);
    void updateAverageRPM(unsigned long averagingTimeInMS);
    double getCurrentRPM();
    double getAverageRPM();
    boolean isInGoodSpeed();
    boolean isRunning();
    boolean getAlarm();
    String getStatusString();
    String getName();
    void update();
    void setMinSpeed(int minspeed);
    int getMinSpeed();

private:
    unsigned int _pinNumber;
    boolean _running;
    unsigned int _pulsesPerRev;
    unsigned int _minSpeed;
    unsigned int _proxCount;

    unsigned int _lastProxCount;
    unsigned long _lastMillisBetweenISRs = 0;
    unsigned long _lastPulseTime = 0;
    double _averageRPM = 0;
    String _name = "";

    void _init(int pin, int pinModeToUse, int minSpeed, String name);
};

#endif
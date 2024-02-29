#ifndef ELEVATOR_H
#define ELEVATOR_H

#pragma once

#include <Rotator.h>

class Elevator: public StatusReportingObject
{
public:
    Elevator();
    Elevator(int beltAProxPin,
                int beltBProxPin,
                int pinMode,
                int elevatorminSpeed,
                int runningEnableRelayPin,
                String name);
    ~Elevator();
    boolean isRunning();
    boolean isInGoodSpeed();
    void updateAverageRPM(unsigned long averagingTimeInMS);
    double getAverageRPM();
    String getStatusString();
    String getName();
    void update();
    void setMinSpeed(int minspeed);
    int getMinSpeed();
    boolean getAlarm();

private:
    Rotator* _beltA;
    Rotator* _beltB;
    String _name;
    unsigned int _minSpeed;
    int _runEnableRelPin;
};

#endif
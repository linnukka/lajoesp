#include "Elevator.h"
#include <Arduino.h>

Elevator::Elevator()
{
}

Elevator::Elevator(int beltAProxPin,
                int beltBProxPin,
                int pinMode,
                int elevatorminSpeed,
                int runningEnableRelayPin,
                String name)
{
    _beltA = new Rotator(beltAProxPin, pinMode, elevatorminSpeed, name + ".A");
    _beltB = new Rotator(beltBProxPin, pinMode, elevatorminSpeed, name + ".B");
    _name = name;
    _minSpeed = elevatorminSpeed;
    _runEnableRelPin = runningEnableRelayPin;
}

Elevator::~Elevator(){}

boolean Elevator::isInGoodSpeed(){

    if(_beltA->isInGoodSpeed() && _beltB->isInGoodSpeed()){
        return true;
    } else {
        return false;
    }
}

boolean Elevator::isRunning(){
    return _beltA->isRunning() && _beltB->isRunning();
}

void Elevator::updateAverageRPM(unsigned long averagingTimeInMS){
    _beltA->updateAverageRPM(averagingTimeInMS);
    _beltB->updateAverageRPM(averagingTimeInMS);
}

double Elevator::getAverageRPM(){
    return (_beltA->getAverageRPM()+_beltB->getAverageRPM())/2;
}

String Elevator::getStatusString()
{
    return "{\"n\":\"" + String(_name) + "\"" +
                ",\"rpm\":" + String(getAverageRPM(),0) +
                ",\"spdok\":" + int(isInGoodSpeed()) +
                ",\"run\":" + int(isRunning()) +
                ",\"lim\":" + String(_minSpeed) +
                ",\"" + _beltA->getName() + "\":" + _beltA->getStatusString() +
                ",\"" + _beltB->getName() + "\":" + _beltB->getStatusString() +
            "}";
}

void Elevator::update()
{
    updateAverageRPM(millis()-_lastUpdateTime);
    _lastUpdateTime = millis();
}

String Elevator::getName(){
    return _name;
}

void Elevator::setMinSpeed(int minSpeed){
    if(minSpeed>0){
        _minSpeed = minSpeed;
    }
}

int Elevator::getMinSpeed(){
    return _minSpeed;
}

boolean Elevator::getAlarm(){
    return _beltA->getAlarm() || _beltB->getAlarm();
}
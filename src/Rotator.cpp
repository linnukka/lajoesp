#include <Rotator.h>


Rotator::Rotator() {}
Rotator::Rotator(int pin, int pinMode, int minSpeed, String name) {
    init(pin, pinMode, minSpeed, name);
}
Rotator::Rotator(int pin, int pinMode, int minSpeed, int pulsesPerRev, String name) {
    init(pin, pinMode, minSpeed, pulsesPerRev, name);
}


void Rotator::init(int pin, int pinMode, int minSpeed, String name)
{
    _pulsesPerRev = 1;
    _init(pin, pinMode, minSpeed, name);
}

void Rotator::init(int pin, int pinMode, int minSpeed, int pulsesPerRev, String name){
    _pulsesPerRev = pulsesPerRev;
    if(pulsesPerRev>0){
        _pulsesPerRev = pulsesPerRev;
    } else {
        _pulsesPerRev = 1;
    }
    _init(pin, pinMode, minSpeed, name);
}

Rotator::~Rotator() {
    detachInterrupt(_pinNumber);
}

void Rotator::_init(int pin, int pinModeToUse, int minSpeed, String name)
{
    _minSpeed = minSpeed;
    _pinNumber = pin;
    _name = name;
    pinMode(_pinNumber, pinModeToUse);    
    _proxCount = 0;

    // attach interrupt
    attachInterrupt(_pinNumber, std::bind(&Rotator::rotatorIsr,this), RISING);
}

void IRAM_ATTR ARDUINO_ISR_ATTR Rotator::rotatorIsr()
{
    if (_lastPulseTime > 0)
    {
        _lastMillisBetweenISRs = millis() - _lastPulseTime;
        _running = true; // sets running status from second pulse
    }

    _lastPulseTime = millis(); // Store pulsetime
    _proxCount++;   // Simply add one to pulse counter on each interrupt for averaging
}

double Rotator::getCurrentRPM(){

    double retval = 0;
    //int revs = _lastProxCount/_pulsesPerRev;
    if(_running && _lastMillisBetweenISRs > 0){
        retval = ((60000)/_lastMillisBetweenISRs)/_pulsesPerRev;
    }
    return retval;
}

void Rotator::updateAverageRPM(unsigned long averagingTimeInMS){

    int previousAvg = _averageRPM;
    int revs = _proxCount/_pulsesPerRev;
    double avgtimeinsecs =  averagingTimeInMS / (double)1000;
    double avgtimeinmins = averagingTimeInMS / (double)60000.0; 



    // _averageRPM = revs / avgtimeinsecs;
    _averageRPM = revs / avgtimeinmins;

    //Serial.println("UpdateAvg: " + _name + " _proxCount: " + String(_proxCount) + " revs: " + String(revs) + " time ms: "+ String(averagingTimeInMS) +" = " + String(avgtimeinsecs,4) + " secs = " + String(avgtimeinmins,4) + " mins. Average: " + String(_averageRPM,4));

    _proxCount = 0;
    if(_averageRPM == 0 && previousAvg == 0){
        _running = false; // Average zero twice => not running
        _lastPulseTime = 0; // Reset last pulse time as well, used for running detection
    }
}

double Rotator::getAverageRPM (){
    return _averageRPM;
}

boolean Rotator::isInGoodSpeed(){

    if(_averageRPM > (double)_minSpeed){
        return true;
    } else {
        return false;
    }
}

boolean Rotator::isRunning(){
    return _running;
}

boolean Rotator::getAlarm(){
    boolean retval = true;
    if(isRunning() && isInGoodSpeed()){
        retval = false;
    }
    return retval;
}

String Rotator::getStatusString()
{
    return "{\"rpm\":" + String(getAverageRPM(),0) +
                ",\"spdok\":" + int(isInGoodSpeed()) +
                ",\"run\":" + int(isRunning()) +
                ",\"lim\":" + String(_minSpeed) +
            "}";
}

void Rotator::update()
{
    updateAverageRPM(millis()-_lastUpdateTime);
    _lastUpdateTime = millis();
}

void Rotator::setMinSpeed(int minSpeed){
    if(minSpeed>0){
        _minSpeed = minSpeed;
    }
}

int Rotator::getMinSpeed(){
    return _minSpeed;
}

String Rotator::getName(){
    return _name;
}
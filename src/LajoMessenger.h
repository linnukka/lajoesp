#ifndef LAJOMESSENGER_H
#define LAJOMESSENGER_H

#pragma once

#include <Arduino.h>
#include <StatusReportingObject.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <lajomqttdefs.h>
#include <lajotimedefs.h>
//#include <TaskScheduler.h>
#include <TaskSchedulerDeclarations.h>
#include <Hatch.h>
#include <ArduinoJson.h>

#define opencmd     "open"
#define closecmd    "close"
#define poscmd      "pos"
#define shakecmd    "shake"
#define shutdowncmd "shutdown"
#define valuestep   "st"
#define valuemm     "mm"

class LajoMessenger
{
public:
    // static std::shared_ptr<LajoMessenger> ptrToSelf;

    LajoMessenger(PubSubClient *pubSubClient, Scheduler *scheduler);
    //LajoMessenger(PubSubClient *pubSubClient);
    void println(String line);
    void setDebugToMqtt(boolean debugToMq);
    void setDebugToSerial(boolean debugToSer);
    //
    void sendStatus();
    void prepareStatusString(StatusReportingObject *sros[], int len);
    void init(std::function<void (char*, uint8_t*, unsigned int)> MQTTcallback);
    ~LajoMessenger();
    boolean reconnect();

    static Hatch::HatchCommand processCommandFromInputString(uint8_t *payload, unsigned int length, Hatch *hatch);
    static Hatch::HatchCommand lajoCallback(char *topic, uint8_t *payload, unsigned int length, Hatch *hatch);
    void setSorterStarted(boolean started);
    void setTrioriAlarm();
    void setBrushAlarm();
    void setElevator1Alarm();
    void setElevator2Alarm();
    void setLevelAlarm();
    void setExtAlarm();
    void setSkipAlarm(boolean skip);

    void clearTrioriAlarm();
    void clearBrushAlarm();
    void clearElevator1Alarm();
    void clearElevator2Alarm();
    void clearLevelAlarm();
    void clearExtAlarm();

private:
    static const int _cmdCapacity = JSON_OBJECT_SIZE(2);
    String _statusMessage;
    boolean _trioriAlarm = false;
    boolean _brushAlarm = false;
    boolean _elevator1Alarm = false;
    boolean _elevator2Alarm = false;
    boolean _levelAlarm = false;
    boolean _extAlarm = false;
    boolean _sorterStarted = false;
    boolean _skipAlarm = false;
    
    //String _name = "";

    String getStatusString(StatusReportingObject *sros[], int len);

    PubSubClient *_pubSubClient;
    Scheduler *_ts;
};

#endif
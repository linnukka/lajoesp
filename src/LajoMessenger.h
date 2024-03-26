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
#define cancelshutdowncmd "cancelshutdown"
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
    void setTrioriAlarm(boolean alarm);
    void setBrushAlarm(boolean alarm);
    void setElevator1Alarm(boolean alarm);
    void setElevator2Alarm(boolean alarm);
    void setLevelAlarm(boolean alarm);
    void setExtAlarm(boolean alarm);
    void setSkipAlarm(boolean skip);
    void setShuttingDown(boolean shutting);
    void setShutReason(String reason);

    void clearTrioriAlarm();
    void clearBrushAlarm();
    void clearElevator1Alarm();
    void clearElevator2Alarm();
    void clearLevelAlarm();
    void clearExtAlarm();
    void setAlarmCount(int count);

private:
    static const int _cmdCapacity = JSON_OBJECT_SIZE(2);
    String _statusMessage;
    String _shutReason;
    boolean _trioriAlarm = false;
    boolean _brushAlarm = false;
    boolean _elevator1Alarm = false;
    boolean _elevator2Alarm = false;
    boolean _levelAlarm = false;
    boolean _extAlarm = false;
    boolean _sorterStarted = false;
    boolean _skipAlarm = false;
    boolean _shuttingDown = false;
    int _alarmCount = 0;
    unsigned long _shutdownAtMillis = 0;
    
    //String _name = "";

    String getStatusString(StatusReportingObject *sros[], int len);

    PubSubClient *_pubSubClient;
    Scheduler *_ts;
};

#endif
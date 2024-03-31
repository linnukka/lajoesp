#include <LajoMessenger.h>
#include <Hatch.h>


#define _DEBUG_
#define _STATUS_TO_SER_
#define _VERBOSE_

LajoMessenger::LajoMessenger(PubSubClient *pubSubClient, Scheduler *scheduler) {
//LajoMessenger::LajoMessenger(PubSubClient *pubSubClient) {
    _pubSubClient = pubSubClient;
    _ts = scheduler;
}

LajoMessenger::~LajoMessenger() {}

Hatch::HatchCommand LajoMessenger::processCommandFromInputString(uint8_t *payload, unsigned int length, Hatch *hatch){
    Hatch::HatchCommand retval = Hatch::HatchCommand::NO_OP;

    if(length>0){
        DynamicJsonDocument jsonDoc(_cmdCapacity);
        DeserializationError error = deserializeJson(jsonDoc, payload);

        // Test if parsing succeeds.
        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            return retval;
        }

        String cmd = jsonDoc["cmd"];
        double mm = jsonDoc["mm"];
        int st = jsonDoc["st"];
        long pos = jsonDoc["pos"];

        #ifdef _DEBUG_
            Serial.println("Parsed. Cmd: " + String(cmd) + " mm: " + String(mm) + " st: " + String(st)+ " pos: " + pos);
        #endif    


        if(cmd.equals(opencmd)){
            if(mm==0.0f && st==0){
                hatch->setTargetCount(hatch_hall_count_when_hatch_open);
                retval = Hatch::HatchCommand::OPEN; // No step value => open fully
                #ifdef _VERBOSE_
                    Serial.println("No step value => open fully");
                #endif
            } else if(mm>0.0f && st == 0){
                hatch->changeTargetMm(mm);
                retval = Hatch::HatchCommand::OP_MM;
                #ifdef _VERBOSE_
                    Serial.println("Open mm: " + String(mm));
                #endif
            } else if (st>0 && mm==0.0f){
                hatch->changeTargetSteps(st);
                retval = Hatch::HatchCommand::OP_STEPS;
                #ifdef _VERBOSE_
                    Serial.println("Open st: " + String(st));
                #endif
            } else {
                // command = NO_OP; // both steps given or negative value, do nothing
            }
        } else if(cmd.equals(closecmd)){
            if(mm==0.0f && st==0){
                hatch->setTargetCount(hatch_hall_count_when_hatch_closed);
                retval = Hatch::HatchCommand::CLOSE; // No step value => close fully
                #ifdef _VERBOSE_
                    Serial.println("No step value => close fully");
                #endif
            } else if(mm>0.0f && st == 0){
                hatch->changeTargetMm(-mm);
                retval = Hatch::HatchCommand::CL_MM;
                #ifdef _VERBOSE_
                    Serial.println("Close mm: " + String(mm));
                #endif
            } else if (st>0 && mm==0.0f){
                hatch->changeTargetSteps(-st);
                retval = Hatch::HatchCommand::CL_STEPS;
                #ifdef _VERBOSE_
                    Serial.println("Close st: " + String(st));
                #endif
            } else {
                // command = NO_OP; // both steps given or negative value, do nothing
                #ifdef _VERBOSE_
                    Serial.println("both steps given or negative value, do nothing");
                #endif
            }
        } else if(cmd.equals(poscmd)){
            Hatch::HatchCommand dir;
            if(mm>0.0f && pos == 0){
                retval = hatch->setTargetMm(mm);
                #ifdef _VERBOSE_
                    Serial.println("Positioning to mm: " + String(mm));
                #endif
            } else if (pos>0 && mm==0.0f){
                retval = hatch->setTargetCount(pos);
                #ifdef _VERBOSE_
                    Serial.println("Positioning to count: " + String(pos));
                #endif
            }

        } else if(cmd.equals(shakecmd)){
            retval = Hatch::HatchCommand::SHAKE;
            #ifdef _VERBOSE_
                Serial.println("Shake it baby, shake!");
            #endif
        } else if(cmd.equals(shutdowncmd)){
            retval = Hatch::HatchCommand::SHUTDOWN;
            #ifdef _VERBOSE_
                Serial.println("Shutdown time!");
            #endif
        } else if(cmd.equals(cancelshutdowncmd)){
            retval = Hatch::HatchCommand::CANCEL_SHUTDOWN;
            #ifdef _VERBOSE_
                Serial.println("Cancel Shutdown!");
            #endif
        }
    }

    return retval;
}

Hatch::HatchCommand LajoMessenger::lajoCallback(char* topic, uint8_t* payload, unsigned int length, Hatch *hatch){
    Hatch::HatchCommand retval = Hatch::HatchCommand::NO_OP;
    #ifdef _VERBOSE_
        Serial.print("Message arrived [");
        Serial.print(topic);
        Serial.print("] ");
        for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
        }
        Serial.println();
    #endif    

    if(String(mqttcommandtopic).equals(topic) && length>0){
        retval = processCommandFromInputString(payload, length, hatch);
    }
    return retval;
}

void LajoMessenger::init(std::function<void (char*, uint8_t*, unsigned int)> MQTTcallback){
    
    WiFi.begin(wifissidTo, wifipassTo);

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        #ifdef _DEBUG_
          Serial.print(".");
        #endif      
    }

    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(wifissidTo);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    _pubSubClient->setServer(mqttserverip, 1883);
    _pubSubClient->setCallback(MQTTcallback);

    delay(1000);

    while(!reconnect()){
      delay(mqttreconnectdelay);
    }

    _pubSubClient->subscribe(mqttcommandtopic);
    String msg = "Lajo messenger connected and subscribed. IP:" + WiFi.localIP().toString();
    Serial.println("MQTT Connected.");
    _pubSubClient->publish(mqttdebugtopic, msg.c_str());
}


void LajoMessenger::println(String line){

}

void LajoMessenger::sendStatus(){
    #ifdef _STATUS_TO_SER_
        Serial.println(_statusMessage);
    #endif
    #ifdef _DEBUG_
        Serial.println();
    #endif
    if(_pubSubClient->connected()){
        // See https://pubsubclient.knolleary.net/api#beginPublish
        int len = _statusMessage.length();
        _pubSubClient->beginPublish(mqttstatustopic, len, true);
        _pubSubClient->write((byte*)_statusMessage.c_str(), len);
        bool sentOk = _pubSubClient->endPublish();
        // #ifdef _VERBOSE_
        //     Serial.println("Publish ok: " + String(sentOk));
        // #endif
        //_pubSubClient->publish(mqttstatustopic, statusStr.substring(0,210).c_str());    
    }
}

String LajoMessenger::getStatusString(StatusReportingObject *sros[], int len){

    String shuttingDownIn = "";
    if(_shuttingDown){
        shuttingDownIn = String((int)(_shutdownAtMillis - millis())/1000);
    }


    // First start status and alarms:
    String retval = "{\"started\":" ;
    retval = retval + String((int)_sorterStarted) +
                    ", \"shutting\":" + String((int)_shuttingDown) +
                    ", \"sh_in\":\"" + shuttingDownIn +
                    "\", \"reason\":\"" + _shutReason + 
                    "\", \"alarms\": {\"master\":" + String(_alarmCount) +
                    ", \"t\":" + String((int)_trioriAlarm) +
                    ", \"b\":" + String((int)_brushAlarm) +
                    ", \"e1\":" + String((int)_elevator1Alarm) + 
                    ", \"e2\":" + String((int)_elevator2Alarm) +
                    ", \"l\":" + String((int)_levelAlarm) +
                    ", \"ext\":" + String((int)_extAlarm) +
                    ", \"skip\":" + String((int)_skipAlarm) +
                    "}, "
                    ;

    // loop sros
    for(int i=0;i<len;i++){
        retval = retval + "\"" + sros[i]->getName() + "\": " + sros[i]->getStatusString() +", ";
    }
    
    retval.remove(retval.lastIndexOf(","));

    return retval + "}";
}

void LajoMessenger::prepareStatusString(StatusReportingObject *sros[], int len){
    _statusMessage = getStatusString(sros, len);
}

boolean LajoMessenger::reconnect() {
  boolean retval = _pubSubClient->connected();
  if(!retval){
    // not connected => try to
    #ifdef _DEBUG_
      Serial.print(millis());
      Serial.println(F(": Not connected to MQTT, trying to connect"));
    #endif      

    if (_pubSubClient->connect(mqttclientname, mqttusername, mqttpasswd)) {
      _pubSubClient->subscribe(mqttcommandtopic);
      retval = true;
      #ifdef _DEBUG_
        Serial.print(millis());
        Serial.println(F(": Connected and subscribed!"));
      #endif      
    } else {
      // connection failed, retry happens from task scheduling if necessary
      #ifdef _DEBUG_
        Serial.print(millis());
        Serial.println(F(": MQTT reconnect failed"));
      #endif      
    }
  }
  return retval;
}

void LajoMessenger::setSorterStarted(boolean started){
    _sorterStarted = started;
}
void LajoMessenger::setTrioriAlarm(boolean alarm){_trioriAlarm = alarm;}
void LajoMessenger::setBrushAlarm(boolean alarm){_brushAlarm = alarm;}
void LajoMessenger::setElevator1Alarm(boolean alarm){_elevator1Alarm = alarm;}
void LajoMessenger::setElevator2Alarm(boolean alarm){_elevator2Alarm = alarm;}
void LajoMessenger::setLevelAlarm(boolean alarm){_levelAlarm = alarm;}
void LajoMessenger::setExtAlarm(boolean alarm){_extAlarm = alarm;}
void LajoMessenger::setSkipAlarm(boolean skip){_skipAlarm = skip;}
void LajoMessenger::setShuttingDown(boolean shutting){
    _shuttingDown = shutting;
    if(shutting){
        //if(_shutdownAtMillis == 0){
        _shutdownAtMillis = millis() + poweroffdelayfromhatchcloseS*1000;  // from hatch closed: Motor poweroff delay
        //}
    } else {
        _shutdownAtMillis = 0;
        _shutReason = "";
    }
}
void LajoMessenger::clearTrioriAlarm(){_trioriAlarm = false;}
void LajoMessenger::clearBrushAlarm(){_brushAlarm = false;}
void LajoMessenger::clearElevator1Alarm(){_elevator1Alarm = false;}
void LajoMessenger::clearElevator2Alarm(){_elevator2Alarm = false;}
void LajoMessenger::clearLevelAlarm(){_levelAlarm = false;}
void LajoMessenger::clearExtAlarm(){_extAlarm = false;}
void LajoMessenger::setAlarmCount(int count){_alarmCount = count;}
void LajoMessenger::setShutReason(String reason){_shutReason = reason;}
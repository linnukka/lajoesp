#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>


#include <lajopindefs.h> // conf file that has most of define -statements
#include <lajotimedefs.h>

#include <Rotator.h>
#include <Hatch.h>
#include <Elevator.h>
//#include <ButSwitch.h>
#include <LajoMessenger.h>
//#include <StatusUpdater.h>
#include <TaskScheduler.h>
#include <FunctionalInterrupt.h>

#define _DEBUG_
#define _VERBOSE_
#define _CHECK_BRUSH_
#define _CHECK_ELEVATOR1_
#define _CHECK_ELEVATOR2_
//#define _CHECK_EXT_ALARM_
//#define _CHECK_LEVELS_
//#define _HATCH_SIMULATOR_
//#define _USERTCMILLIS_

WiFiClient wifiClient;
PubSubClient pubSubClient(wifiClient);

Elevator* inElevator1;
Elevator* inElevator2;
Rotator* triori;
Rotator* brush;
Hatch* hatch;
unsigned long lastAverageMillis;

const int srolen = 5;
StatusReportingObject *sros[srolen];
LajoMessenger *messenger;
// StatusUpdater updater;
boolean openOn=false;
boolean closeOn=false;
boolean sorterStarted=false;
unsigned long openPressedTime = 0;
unsigned long closePressedTime = 0;
int lastUpdPercentage = 0;
int64_t outCountOnIsr = 0;
int alarmCount = 0;
//unsigned long openClickCounter = 0;
//unsigned long closeClickCounter = 0;

// --------------------- Task scheduler definitions -----------------------

// Forward definition of all callback methods is now required by v1.6.6
Scheduler ts;
String serialInputBuffer;

void pubsubLoopCallback();
void prepareStatusMsgCallback();  // don
void sendStatusCallback();  // done
void mqttReconnectCallback(); // done
void updateStatusCallback();  // done
void updateStatusForOneCallback(); // done
void statusUpdateCompletedCallback(); // done
void openSwDebounceCallback();
void openSwLongPressCallback();
void openingPollCallback();
void closeSwDebounceCallback();
void closeSwLongPressCallback();
void closingPollCallback();
void shutdownCallback();
void hatchRepositionCallback();
void serialPollingCallback();
void otaHandleCallback();
void motorPoweroffCallback();
void elevatorPoweroffCallback();

#ifdef _HATCH_SIMULATOR_
  void hatchHallSimulatorCallback();
#endif


Task tMqttReconnect(mqttreconnectdelay * TASK_MILLISECOND, TASK_FOREVER, &mqttReconnectCallback, &ts, false);
Task tUpdateStatus(updateStatusIntervalS * TASK_SECOND, TASK_FOREVER ,&updateStatusCallback, &ts, false);
Task tUpdateStatusForOne(TASK_IMMEDIATE,srolen,&updateStatusForOneCallback, &ts, false, NULL, &statusUpdateCompletedCallback);
Task tPrepareStatusMsg(TASK_IMMEDIATE, 1, &prepareStatusMsgCallback, &ts, false);
Task tSendStatus(TASK_IMMEDIATE, 1, &sendStatusCallback, &ts, false);
Task tPubsub(pubsubloopintervalms * TASK_MILLISECOND, TASK_FOREVER, &pubsubLoopCallback, &ts, false);
Task tSerialPoll(serialpollingintervalms * TASK_MILLISECOND, TASK_FOREVER, &serialPollingCallback, &ts, false);
Task tOtaPoll(otahandleintervalms * TASK_MILLISECOND, TASK_FOREVER, &otaHandleCallback, &ts, false);

Task tOpenSwDebounceCheck(buttondebouncetimems * TASK_MILLISECOND, 1, openSwDebounceCallback, &ts, false);
Task tCloseSwDebounceCheck(buttondebouncetimems * TASK_MILLISECOND, 1, closeSwDebounceCallback, &ts, false);
Task tOpenSwitchLongPressCheck((buttonclicktimelimitms+buttondebouncetimems) * TASK_MILLISECOND, 1, &openSwLongPressCallback, &ts, false);
Task tCloseSwitchLongPressCheck((buttonclicktimelimitms+buttondebouncetimems) * TASK_MILLISECOND, 1, &closeSwLongPressCallback, &ts, false);
Task tOpeningPoll(hatchpollingintervalms * TASK_MILLISECOND, TASK_FOREVER, &openingPollCallback, &ts, false);
Task tClosingPoll(hatchpollingintervalms * TASK_MILLISECOND, TASK_FOREVER, &closingPollCallback, &ts, false);
Task tRepositioningPoll(hatchstoprepositiondelayms * TASK_MILLISECOND, TASK_FOREVER, &hatchRepositionCallback, &ts, false);
Task tShutdown(TASK_IMMEDIATE, 1, &shutdownCallback, &ts, false);
Task tSorterPoweroff(poweroffdelayfromhatchcloseS * TASK_SECOND, 1, &motorPoweroffCallback, &ts, false);

#ifdef _HATCH_SIMULATOR_
  Task tHatchSimulator(hatchsimulatorintervalms * TASK_MILLISECOND, TASK_FOREVER, &hatchHallSimulatorCallback, &ts, false);
#endif


// --------------------- Task scheduler definitions end -----------------------

void openSwIsr(void);
//void openReleasedIsr(void);
void closeSwIsr(void);
//void closeReleasedIsr(void);

void enableMotor(){digitalWrite(motorenablerelay, HIGH);}
void disableMotor(){digitalWrite(motorenablerelay, LOW);}
void enableElevator1(){digitalWrite(elevator1relaypin, HIGH);}
void disableElevator1(){digitalWrite(elevator1relaypin, LOW);}
void enableElevator2(){digitalWrite(elevator2relaypin, HIGH);}
void disableElevator2(){digitalWrite(elevator2relaypin, LOW);}
void alarmOn(){
  digitalWrite(alarmrelpin, LOW);
  alarmCount++;
  messenger->setAlarmCount(alarmCount);
  #ifdef _DEBUG_
    Serial.print(millis());
    Serial.println(F(": AlarmOn."));
  #endif      
}
void alarmOff(){
  #ifdef _VERBOSE_
    Serial.print(millis());
    Serial.println(F(": AlarmOff."));
  #endif      
  digitalWrite(alarmrelpin, HIGH);
  alarmCount = 0;
  messenger->setAlarmCount(alarmCount);
}

void startOpeningHatch(boolean slowSpeed){
    if(hatch->startOpening(slowSpeed)){
      // returned true, so really moving
      tOpeningPoll.enableDelayed();
  } else {
    // hatch already fully open, so nothing to do
  }
}
void startClosingHatch(boolean slowSpeed){
      if(hatch->startClosing(slowSpeed)){
        // returned true, so really moving
        tClosingPoll.enableDelayed();
      } else {
        // hatch already fully open, so nothing to do
      }
}

void processHatchCmd(Hatch::HatchCommand cmd){
  switch (cmd){
    case Hatch::HatchCommand::OPEN:
        #ifdef _DEBUG_
          Serial.print(millis());
          Serial.println(F(": processHatchCmd cmd OPEN"));
        #endif    
        startOpeningHatch(false);  // Command processing already has updated target to run into
      break;    
    case Hatch::HatchCommand::OP_STEPS:
    case Hatch::HatchCommand::OP_MM:
        #ifdef _DEBUG_
          Serial.print(millis());
          Serial.println(F(": processHatchCmd cmd OP_STEPS or OP_MM"));
        #endif    
        startOpeningHatch(hatch->isTargetApproaching());  // Command processing already has updated target to run into
      break;    
    case Hatch::HatchCommand::CLOSE:
        #ifdef _DEBUG_
          Serial.print(millis());
          Serial.println(F(": processHatchCmd cmd CLOSE"));
        #endif    
        startClosingHatch(false); // Command processing already has updated target to run into
      break;    
    case Hatch::HatchCommand::CL_STEPS:
    case Hatch::HatchCommand::CL_MM:
        #ifdef _DEBUG_
          Serial.print(millis());
          Serial.println(F(": processHatchCmd cmd CLOSE or CL_STEPS or CL_MM"));
        #endif    
        startClosingHatch(hatch->isTargetApproaching()); // Command processing already has updated target to run into
      break;    
    case Hatch::HatchCommand::SHAKE:
        #ifdef _DEBUG_
          Serial.print(millis());
          Serial.println(F(": processHatchCmd cmd SHAKE ---------------- TODO --------------"));
        #endif    
      break;    
    case Hatch::HatchCommand::SHUTDOWN:
        #ifdef _DEBUG_
          Serial.print(millis());
          Serial.println(F(": processHatchCmd cmd SHUTDOWN"));
        #endif    
        messenger->setShutReason("cmd");
        tShutdown.setIterations(1);
        tShutdown.enable();
        tShutdown.forceNextIteration();
      break;    
    default:
        #ifdef _DEBUG_
          Serial.print(millis());
          Serial.println(F(": processHatchCmd UNKNOWN cmd"));
        #endif    
      break;
  }
}

static void mqttCallback(char* topic, byte* payload, unsigned int length){
  processHatchCmd(messenger->lajoCallback(topic, payload, length, hatch));
}

boolean isSkipAlarmOn(){
  boolean skip = !digitalRead(skipalarmswpin);
  messenger->setSkipAlarm(skip); 
  return skip;
}
boolean isExtAlarmOn(){
  boolean ext = !digitalRead(extalarmpin);
  messenger->setExtAlarm(ext); 
  return ext;
}
boolean isLevelAlarmOn(){
  boolean lev = !digitalRead(levelalarmproxpin);
  messenger->setLevelAlarm(lev); 
  return lev;
}

boolean isOpenNowOn(){return !digitalRead(openswpin);}
boolean isCloseNowOn(){return !digitalRead(closeswpin);}
unsigned long getOpenOnTime(){
    if(openOn && openPressedTime>0){ 
      return millis()-openPressedTime;
    } else {
        return 0;
    }
}
unsigned long getCloseOnTime(){
    if(closeOn && closePressedTime>0){ 
      return millis()- closePressedTime;
    } else {
        return 0;
    }
}

void setup() {
  // Serial.begin(usbSerialBaudrate, hwSerialBaudrate);
  Serial.begin(usbSerialBaudrate);

  pinMode(openswpin, INPUT_PULLUP); 
  pinMode(closeswpin, INPUT_PULLUP); 
  pinMode(skipalarmswpin, INPUT_PULLUP); 
  pinMode(alarmrelpin, OUTPUT);
  pinMode(motorenablerelay, OUTPUT);
  pinMode(levelalarmproxpin, INPUT_PULLUP);
  pinMode(elevator2relaypin, OUTPUT);
  pinMode(elevator1relaypin, OUTPUT);
  pinMode(extalarmpin, INPUT_PULLUP);

  delay(10);
  #ifdef _DEBUG_
    delay(2000);
  #endif
  
  Serial.println("Serial start.");
  messenger = new LajoMessenger(&pubSubClient, &ts);
  //messenger = new LajoMessenger(&pubSubClient);
  messenger->init(mqttCallback);
  delay(100);
  tMqttReconnect.enableDelayed();

  #ifdef _DEBUG_
    Serial.print(millis());
    Serial.println(F(": Initing OTA"));
  #endif      
  ArduinoOTA.setHostname(hostname);
  // ArduinoOTA.setPassword("admin");
  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      lastUpdPercentage = 0;
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {

      int percentage = (progress / (total / 100)); 
      if(percentage % 10 == 0 && percentage>lastUpdPercentage){
        lastUpdPercentage = percentage;
        Serial.printf("%u%%\r ", percentage);
      }
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  #ifdef _DEBUG_
    Serial.print(millis());
    Serial.println(F(": OTA Inited. Creating SROs"));
  #endif      

  // TODO move elevator monitor to own class
  inElevator1 = new Elevator(elevator1Aproxpin, elevator1Bproxpin, INPUT_PULLUP, elevator1minspeeddefault, elevator1relaypin, "inElev1");
  inElevator2 = new Elevator(elevator2Aproxpin, elevator2Bproxpin, INPUT_PULLUP, elevator2minspeeddefault, elevator2relaypin, "inElev2");
  triori = new Rotator(trioriproxpin, INPUT_PULLUP, trioriminspeeddefault, "triori");
  brush = new Rotator(brushproxpin, INPUT_PULLUP, brushminspeeddefault, "brush");
  hatch = new Hatch();
  hatch->init();

  
  attachInterrupt(openswpin, openSwIsr, CHANGE);
  attachInterrupt(closeswpin, closeSwIsr, CHANGE);

  enableMotor();
  enableElevator1();
  enableElevator2();

  sros[0] = hatch;
  sros[1] = triori;
  sros[2] = brush;
  sros[3] = inElevator1;
  sros[4] = inElevator2;

  #ifdef _DEBUG_
    Serial.print(millis());
    Serial.println(F(": Enabling tPubsub"));
  #endif      

  tPubsub.enableDelayed();
  //delay(5000);
  //updater.init(sros);

  #ifdef _DEBUG_
    Serial.print(millis());
    Serial.println(F(": Enabling tUpdateStatus and Serial polling"));

    alarmOn();
    delay(1000);
  #endif      

  tUpdateStatus.enableDelayed();
  tSerialPoll.enableDelayed();
  tOtaPoll.enableDelayed();

  #ifdef _HATCH_SIMULATOR_
    tHatchSimulator.enable();
  #endif

  alarmOff();

  #ifdef _DEBUG_
    Serial.print(millis());
    Serial.println(F(": Setup complete"));
  #endif      


}

void IRAM_ATTR ARDUINO_ISR_ATTR openSwIsr()
{
    if(!tOpenSwDebounceCheck.isEnabled()){
        // Debounce timer for either edge not running, so start now
        tOpenSwDebounceCheck.setIterations(1);
        tOpenSwDebounceCheck.enableDelayed();
        outCountOnIsr = hatch->getOutCount();
    } else {
        // First glitch already recorded and debounce timer running => do nothing
    }
}

void IRAM_ATTR ARDUINO_ISR_ATTR closeSwIsr()
{
    if(!tCloseSwDebounceCheck.isEnabled()){
        // Debounce timer for either edge not running, so start now
        tCloseSwDebounceCheck.setIterations(1);
        tCloseSwDebounceCheck.enableDelayed();
        outCountOnIsr = hatch->getOutCount();
    } else {
        // First glitch already recorded and debounce timer running => do nothing
    }
}
void openSwDebounceCallback(){
    #ifdef _VERBOSE_
      Serial.print(millis());
      Serial.println(F(": openSwDebounceCallback"));
    #endif      
    int64_t ontime = getOpenOnTime();      
    if(isOpenNowOn()){    // On-edge
        openOn = true;
        openPressedTime = millis();
        tOpenSwitchLongPressCheck.setIterations(1);
        tOpenSwitchLongPressCheck.enableDelayed();
        #ifdef _VERBOSE_
          Serial.print(millis());
          Serial.println(F(": opensw now on"));
        #endif      
    } else {  // Off-edge
      openOn = false;
      if(ontime > 0){      
        if(ontime<buttonclicktimelimitms){
          // Click, so add one to counter and enable counter processing
          //openClickCounter++;
          hatch->changeTargetSteps(1);
          startOpeningHatch(true);

        } else {
          // Long press end or switch off, so hatch must have been in continuous move => stop it
          #ifdef _VERBOSE_
            Serial.println("openSwDebounceCallback stopping hatch.");    
          #endif
          hatch->stop(true);
          tOpeningPoll.disable();
          tRepositioningPoll.enableDelayed();
          // hatch->setTargetCount(hatch->getOutCount());
        }
      }
      #ifdef _VERBOSE_
        Serial.print(millis());
        Serial.print(F(": opensw now off. Was on for: "));
        Serial.print(ontime);
        Serial.print(F(" Hatch target: "));
        Serial.println(String(hatch->getTargetCount()));
      #endif 
      openPressedTime = 0;
    }
}
void openSwLongPressCallback(){
    #ifdef _VERBOSE_
      Serial.print(millis());
      Serial.println(F(": openSwLongPressCallback"));
    #endif      
    if(getOpenOnTime()>buttonclicktimelimitms){
      hatch->setTargetCount(hatch_hall_count_when_hatch_open);
      startOpeningHatch(false);
    } else {
        #ifdef _VERBOSE_
          Serial.print(millis());
          Serial.println(F(": Click, not long press."));
        #endif      
    }
}

void openingPollCallback(){
    #ifdef _VERBOSE_
      Serial.print(millis());
      Serial.println(F(": openingPollCallback"));
    #endif      

    if(hatch->isHatchFullyOpen() || hatch->isTargetReached()){
      #ifdef _VERBOSE_
        Serial.println("openingPollCallback stopping hatch.");    
      #endif
      hatch->stop(false);
      tOpeningPoll.disable();
      tRepositioningPoll.enableDelayed();
      // hatch->setTargetCount(hatch->getOutCount());
      #ifdef _VERBOSE_
        Serial.print(millis());
        Serial.println(F("Hatch fully open or target reached, stopped and polling disabled. Click counter zeroed."));
      #endif      
    }
}

void closeSwDebounceCallback(){
    #ifdef _VERBOSE_
      Serial.print(millis());
      Serial.println(F(": closeSwDebounceCallback"));
    #endif
    int64_t ontime = getCloseOnTime();      
    if(isCloseNowOn()){ // On-edge
        closeOn = true;
        closePressedTime = millis();
        tCloseSwitchLongPressCheck.setIterations(1);
        tCloseSwitchLongPressCheck.enableDelayed();        
        #ifdef _VERBOSE_
          Serial.print(millis());
          Serial.println(F(": closesw now on"));
        #endif      
    } else { // Off-edge, if not glitch
      closeOn = false;
       if(ontime > 0){
        if(ontime < buttonclicktimelimitms){
          // Click, so add one to counter and enable counter processing
          hatch->changeTargetSteps(-1);
          startClosingHatch(true);
        } else {
          // Long press end or switch off, so hatch must have been in continuous move => stop it
          #ifdef _VERBOSE_
            Serial.print(millis());
            Serial.println(F(": closeSwDebounceCallback stopping hatch."));
          #endif      
          hatch->stop(true);
          tClosingPoll.disable();
          tRepositioningPoll.enableDelayed();
          // hatch->setTargetCount(hatch->getOutCount());
        }
      }   
      #ifdef _VERBOSE_
        Serial.print(millis());
        Serial.print(F(": closesw now off. Was on for: "));
        Serial.print(ontime);
        Serial.print(F(" Hatch target: "));
        Serial.println(String(hatch->getTargetCount()));
      #endif 
      closePressedTime = 0;
    }
}
void closeSwLongPressCallback(){
    #ifdef _VERBOSE_
      Serial.print(millis());
      Serial.println(F(": closeSwLongPressCallback"));
    #endif      
    if(getCloseOnTime()>buttonclicktimelimitms){
      hatch->setTargetCount(hatch_hall_count_when_hatch_closed);
      startClosingHatch(false);
    } else {
        #ifdef _VERBOSE_
          Serial.print(millis());
          Serial.println(F(": Click, not long press."));
        #endif      
    }
}

void closingPollCallback(){
    #ifdef _VERBOSE_
      Serial.print(millis());
      Serial.println(F(": closingPollCallback"));
    #endif      

    if(hatch->isHatchFullyClosed() || hatch->isTargetReached()){
      #ifdef _VERBOSE_
        Serial.println("closingPollCallback stopping hatch.");    
      #endif

      hatch->stop(false);
      tClosingPoll.disable();
      tRepositioningPoll.enableDelayed();
      // hatch->setTargetCount(hatch->getOutCount());
      #ifdef _VERBOSE_
        Serial.print(millis());
        Serial.println(F("Hatch fully closed or target reached, stopped and polling disabled. Click counter zeroed."));
      #endif      
    }
}
void shutdownCallback(){
    #ifdef _DEBUG_
      Serial.print(millis());
      Serial.println(F(": shutdownCallback starts"));
    #endif

    //messenger->setShuttingDown(true);

    if(hatch->isHatchFullyClosed()){
      #ifdef _DEBUG_
        Serial.print(millis());
        Serial.println(F(": shutdownCallback Hatch closed, enable poweroff after delay."));
      #endif

      hatch->stop(false);

      // Poweroff sorter, once hatch closed, with delay
      tSorterPoweroff.setIterations(1);
      tSorterPoweroff.setCallback(&motorPoweroffCallback);
      tSorterPoweroff.setInterval(poweroffdelayfromhatchcloseS * TASK_SECOND);
      tSorterPoweroff.enableDelayed();

      tShutdown.disable();
      tShutdown.setInterval(TASK_IMMEDIATE);
      tShutdown.disable();

    } else if(hatch->isClosing()){
      // Do nothing but wait again for closure
      tShutdown.setInterval(poweroffhatchclosedpollintms * TASK_MILLISECOND);
      tShutdown.setIterations(TASK_FOREVER);
      tShutdown.enableDelayed();

    } else { // Hatch is not closing, so close now
      #ifdef _DEBUG_
        Serial.print(millis());
        Serial.println(F(": shutdownCallback Hatch not closed, start closing."));
      #endif

      // Close hatch
      hatch->setTargetCount(hatch_hall_count_when_hatch_closed);
      hatch->startClosing(false);

      tShutdown.setInterval(poweroffhatchclosedpollintms * TASK_MILLISECOND);
      tShutdown.setIterations(TASK_FOREVER);
      tShutdown.enableDelayed();
    }
}

void motorPoweroffCallback(){
  #ifdef _DEBUG_
    Serial.print(millis());
    Serial.println(F(": motorPoweroffCallback begin"));
  #endif
    // Set sorterStarted to false and clear potential alarms;
    sorterStarted = false;
    messenger->setSorterStarted(false);
    //digitalWrite(motorenablerelay, HIGH);
    disableMotor();

    // Poweroff elevators, once sorter stopped
    tSorterPoweroff.setCallback(&elevatorPoweroffCallback);
    tSorterPoweroff.setInterval(elevatorpowerofffrommotoroffS * TASK_SECOND);
    tSorterPoweroff.setIterations(1);
    tSorterPoweroff.enableDelayed();
  
  #ifdef _DEBUG_
    Serial.print(millis());
    Serial.println(F(": motorPoweroffCallback end"));
  #endif

}

void elevatorPoweroffCallback(){
    // Set sorterStarted to false and clear potential alarms;
    #ifdef _DEBUG_
      Serial.print(millis());
      Serial.println(F(": elevatorPoweroffCallback begin"));
    #endif

    disableElevator1();
    disableElevator2();
    alarmOff();

    delay(poweroffdisablerelaysperiodms);

    enableMotor();
    enableElevator1();
    enableElevator2();

    messenger->setShuttingDown(false);

    tShutdown.disable();
    tSorterPoweroff.disable();

    #ifdef _DEBUG_
      Serial.print(millis());
      Serial.println(F(": elevatorPoweroffCallback end"));
    #endif
}

void hatchRepositionCallback(){
    #ifdef _DEBUG_
      Serial.print(millis());
      Serial.println(F(": hatchRepositionCallback "));
    #endif
    boolean positioned = hatch->repostionHatchToTarget();
    if(positioned){
      tRepositioningPoll.disable();
    }
}

void mqttReconnectCallback(){
  if(!pubSubClient.connected()){
    if(!messenger->reconnect()){
      #ifdef _DEBUG_
        Serial.print(millis());
        Serial.println(F(": MQTT reconnect failed"));
      #endif      
      // Reconnect failed, should disable messaging until reconnected
      // Reconnects are happening by task scheduler, no need to delay and manually retry
    }
  }
}

void updateStatusCallback(){ 
  // Starts status update iteration
  tUpdateStatusForOne.setIterations(srolen);
  tUpdateStatusForOne.enable();
}

void updateStatusForOneCallback(){
  // one step in looping status updates
  int i = tUpdateStatusForOne.getRunCounter() -1;

  if(i<srolen){
    sros[i]->update();
  } else {
    #ifdef _DEBUG_
      Serial.print(millis());
      Serial.print(F(": updateStatusForOneCallback RUNOUT! i="));
      Serial.println(i);
    #endif         
    tUpdateStatusForOne.disable();
  }
}

boolean isRunningFine(){
  boolean retval = true;  // Start optimistic and then set to false if anything wrong

    // Triori check
    if (!triori->isRunning()){
      Serial.print(millis());
      Serial.println(F(": Triori not running!"));
      //messenger->setTrioriAlarm();
      retval = false;
    } else if (!triori->isInGoodSpeed()){
      Serial.print(millis());
      Serial.println(F(": Triori speed low!"));    
      retval = false;
    }

    // Brush check
    #ifdef _CHECK_BRUSH_
      if (!brush->isRunning()){
        Serial.print(millis());
        Serial.println(F(": Brush not running!"));      
        retval = false;
        } else if (!brush->isInGoodSpeed()){
        Serial.print(millis());
        Serial.println(F(": Brush speed low!"));     
        retval = false;
      }
    #endif    

    // Set started condition if all good
    if(retval && !sorterStarted){
      sorterStarted = true;
      messenger->setSorterStarted(true);
    }

    // Clear started if skipalarm on and both triori and brush stopped
    if(isSkipAlarmOn() && !triori->isRunning() && !brush->isRunning()){
      sorterStarted = false;
      messenger->setSorterStarted(false);
    }

    // check both elevators
    #ifdef _CHECK_ELEVATOR1_
      if (!inElevator1->isRunning()){
        Serial.print(millis());
        Serial.println(F(": Elevator 1 not running!"));          
        retval = false;
      }  else if (!inElevator1->isInGoodSpeed()) {
        Serial.print(millis());
        Serial.println(F(": Elevator 1  speed low!"));       
        retval = false;
      }
    #endif    
    #ifdef _CHECK_ELEVATOR2_
      if (!inElevator2->isRunning()){
        Serial.print(millis());
        Serial.println(F(": Elevator 2 not running!")); 
        retval = false;
      }  else if (!inElevator2->isInGoodSpeed()) {
        Serial.print(millis());
        Serial.println(F(": Elevator 2  speed low!")); 
        retval = false;
      }
    #endif    

    // Level alarm check
    #ifdef _CHECK_LEVELS_
      if(isLevelAlarmOn()){
        Serial.print(millis());
        Serial.println(F(": Level alarm!")); 
        retval = false;
      }
    #endif    

    // External alarm check
    #ifdef _CHECK_EXT_ALARM_
      if(isExtAlarmOn()){
        Serial.print(millis());
        Serial.println(F(": Ext alarm!")); 
        retval = false;
      }
    #endif    

  return retval;
}

void statusUpdateCompletedCallback(){
  messenger->setBrushAlarm(brush->getAlarm());
  messenger->setTrioriAlarm(triori->getAlarm());
  messenger->setElevator1Alarm(inElevator1->getAlarm());
  messenger->setElevator2Alarm(inElevator2->getAlarm());
  messenger->setSorterStarted(sorterStarted);
  messenger->setSkipAlarm(isSkipAlarmOn());
  messenger->setExtAlarm(isExtAlarmOn());
  messenger->setLevelAlarm(isLevelAlarmOn());
  // Check running conditions

  boolean runningFine = isRunningFine();

  if(sorterStarted){
    #ifdef _VERBOSE_
      Serial.print(millis());
      Serial.println(F(": statusUpdateCompletedCallback sorter started"));
    #endif         
    if(runningFine){ 
      alarmOff();
    } else {
      if(!isSkipAlarmOn()){
        alarmOn();
        if(alarmCount>alarmcounttostartshutoff){
          messenger->setShuttingDown(true);
          messenger->setShutReason("alarm");
          tShutdown.setIterations(1);
          tShutdown.enable();
          #ifdef _DEBUG_
            Serial.print(millis());
            Serial.println(F(": But not running fine! Shutdown initiated!"));
          #endif         
        } else {
          #ifdef _DEBUG_
            Serial.print(millis());
            Serial.println(F(": But not running fine! Alarm on, shutdown not initiated yet!"));
          #endif         
        }
      } else {
        #ifdef _DEBUG_
          Serial.print(millis());
          Serial.println(F("Not running fine, but skip alarm on, so not shutting down!"));
        #endif         
      }
    }
  } else { 
    #ifdef _DEBUG_
      Serial.print(millis());
      Serial.println(F(": statusUpdateCompletedCallback sorter NOT started"));
    #endif         
    isSkipAlarmOn(); // just to update switch status
  } 
  // All status reporting objects looped => enable message prep and then sending
  tPrepareStatusMsg.setIterations(1);
  tPrepareStatusMsg.enable();
}
void prepareStatusMsgCallback(){
  // Prepare message and then enable sending
  messenger->prepareStatusString(sros, srolen);
  tSendStatus.setIterations(1);
  tSendStatus.enable();
}
void sendStatusCallback(){
  messenger->sendStatus();
}

void pubsubLoopCallback(){
  pubSubClient.loop();
}

void serialPollingCallback(){
  int avbytes = Serial.available(); 
  if(avbytes>0){
    #ifdef _DEBUG_
      Serial.print(millis());
      Serial.println(": serialPollingCallback input available: " + String(avbytes));
    #endif         

    serialInputBuffer += Serial.readString();

    if(serialInputBuffer.endsWith(serialterminator)){
      #ifdef _DEBUG_
        Serial.print(millis());
        Serial.println((": Line received: " + serialInputBuffer));
      #endif
      processHatchCmd(messenger->processCommandFromInputString((uint8_t*)serialInputBuffer.c_str(), serialInputBuffer.length(), hatch));
      serialInputBuffer = "";
    }
  }
}

void otaHandleCallback(){
  ArduinoOTA.handle();
}

void loop(){
   ts.execute();
}

#ifdef _HATCH_SIMULATOR_
void hatchHallSimulatorCallback(){
  hatch->testingOutcountManipulate();
}
#endif
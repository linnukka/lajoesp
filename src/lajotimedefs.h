#ifndef LAJOTIMEDEFS
#define LAJOTIMEDEFS

#define usbSerialBaudrate 115200 // Serial speed USB
#define updateintervaldefault 10000

// com test
//ledc: 0  => Group: 0, Channel: 0, Timer: 0
//#define hatchextendpwmchannel      0  // Not pin, but channel: For ESP32_S2, ESP32_S3, number of channels is 8, max 20-bit resolution
//ledc: 1  => Group: 0, Channel: 1, Timer: 0
//#define hatchretractpwmchannel      1  // Not pin, but channel: For ESP32_S2, ESP32_S3, number of channels is 8, max 20-bit resolution
//#define hatchspeedpwmchannel      2  // Not pin, but channel: For ESP32_S2, ESP32_S3, number of channels is 8, max 20-bit resolution

#define buttondebouncetimems            50   

// ----------------- Speed limits ------------------------
//                                         // SPEEDS FOR TESTING, 4,0V for fan too low
// #define trioriminspeeddefault           1700 // 8   // Minimum number of pulses before alarm
// #define brushminspeeddefault            900 // 8   // Minimum number of pulses before alarm
// #define elevator1minspeeddefault        1700      
// #define elevator2minspeeddefault        1700   

// ----------------- Speed limits ------------------------
                                        // SPEEDS FOR TESTING, 4,0V for fan too low
#define trioriminspeeddefault           650 // 740 idle running speed   // Minimum number of pulses before alarm
#define brushminspeeddefault            10 // 24 idle   // Minimum number of pulses before alarm
#define elevator1minspeeddefault        250 // 340 idle      
#define elevator2minspeeddefault        250    


// ----------------- TASK Scheduler ------------------------
#define updateStatusIntervalS           15    
#define pubsubloopintervalms            100
#define buttonclicktimelimitms          500
#define hatchpollingintervalms          500
#define serialpollingintervalms         500
#define otahandleintervalms             5000
#define poweroffhatchclosedpollintms    1000
//#define poweroffdelayfromhatchcloseS    10
#define poweroffdelayfromhatchcloseS    300
#define poweroffdisablerelaysperiodms   2000
#define elevatorpowerofffrommotoroffS   10  

// ----------------- Hatch movement ------------------------
#define steplengthms            50 // 2200 // 55
#define pauselengthms           500
//#define em241pwmfreq            2000.0f
//#define hatchpwmresolution      12  // Resolution  4096 (12-bit) for lower frequencies, OK @ 10K

#define hatch_hall_count_when_hatch_open    4540
#define hatch_hall_count_when_hatch_closed    0
#define hatch_maxtravel                   4543 // 4535  4544   
#define hatch_mm_per_count                0.045f    

#define steplengthpulses                111     // 111 pulses ~ 5 mm
#define hatchstoprepositiondelayms      500
// #define hatchrepositionspeed            100.0f
// #define hatchapproachspeed              100.0f
// #define hatchfullspeed                  100.0f
// #define hatchspeedoutput                100.0f
#define hatchrepositiontolerance        15
#define hatchslowdownadvance            150

// ---------- alarm --------------
#define alarmcounttostartshutoff        3   // 1 allows one status update of alarm before shutoff  

// ---------- testing ------------
#define hatchsimulatorintervalms        200
#define testposerr                      20


#endif
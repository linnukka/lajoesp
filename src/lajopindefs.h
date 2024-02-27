#ifndef LAJOPINDEFS
#define LAJOPINDEFS


// *  Reading ~14 VDC prox switch: Use divider: Switch_black 3k9 to pin to 2k2 to ground


// ------------ Right side pins from antenna down ---------------
                        // GND     
#define alarmrelpin         23    // blue, Screw 1 Alarm relay output 230V
#define uart1txpin       22 // ?????  EM241 pin 10    ------------------------------------ TODO free up to something else -------------------------------
                        // DO NOT USE  //#define extenddirpin         1  // EM241 pin 9
                        // DO NOT USE  //#define hatchpwmpin          3     // EM241 pin 12, 2 kHz
#define extendpwmpin         21  // EM241 pin 9
//#define retractpwmpin        21 // EM241 pin 10
                        // GND
#define outerlimitpin       19     // violet, R1 ?  
#define innerlimitpin       18     //white, R2 ? // green, screw 10, Proximity switch used to limit hatch opening movement, closing controlled by built-in limit in motor
#define closeswpin        5  // yellow, Screw 4
#define openswpin         17   // pink, Screw 3, note that switch is NormallyConnected

#define skipalarmswpin      16    // yellow, Screw 2

// 20240127 Wire buttons in parallel with Switch and check usage in code free up 2 pins as 1&3 are nogo
#define retractpwmpin        4     // EM241 pin 10, 2 kHz
#define hatchspeedpin       0  // EM241 pin 13

#define brushproxpin        2     // PIN 12 TO OUTPUT USE, prevents flashing!
#define trioriproxpin       15     // INT1, grey, screw 9

                        // DO NOT USE
                        // DO NOT USE
                        // DO NOT USE


// ------------ Left side pins from antenna down ---------------
                        // 3V3     
                        // RESET     
#define hall1pin            39    // PA-04-HS pin1 Hall sensor signal 1, leads extending          DIG input, no pullups
#define hall2pin            36    // PA-04-HS pin6 Hall sensor signal 2, leads retracting         DIG input, no pullups
#define elevator2Aproxpin   34 //     DIG input, no pullups
#define elevator2Bproxpin   35 //     DIG input, no pullups
#define elevator1Bproxpin 32 // 
#define elevator1Aproxpin 33 // 

#define uart1rxpin          25 // ???

#define motorenablerelay    26     // Allow motor to run, disconnect sorter main motor from power on running failure
#define elevator2relaypin   27    // 13     

#define levelalarmproxpin   14     // brown,Screw 5
#define elevator1relaypin   12     // PIN 12 TO OUTPUT USE, prevents flashing!
                        // GND
#define extalarmpin         13     // 27 // black, R3, 230VAC to R3 A1-A2 when external alarm to close down
                        // DO NOT USE
                        // DO NOT USE
                        // DO NOT USE
                        // 5V



// Use alarm relay pin for buzzer as well, free up pin for elevator relay
// #define buzzerpin           15     // blue, screw 11, Buzzer transistor base through 1k res, dedicated -DC blue (separate wire), +12VDC purple




#endif
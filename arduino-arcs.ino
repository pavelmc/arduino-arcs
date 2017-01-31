/*******************************************************************************
 *
 *            ARCS - Amateur Radio Control & Clock Solution
 *           -----------------------------------------------
 * A full QRP/Hombrew transceiver control with RF generation, the Cuban way.
 *
 * Copyright (C) 2016 Pavel Milanes (CO7WT) <pavelmc@gmail.com>
 *
 * This work is based on the previous work of these great people:
 *  * NT7S (http://nt7s.com)
 *  * SQ9NJE (http://sq9nje.pl)
 *  * AK2B (http://ak2b.blogspot.com)
 *  * QRL Labs team (http://qrp-labs.com)
 *  * WJ6C for the idea and hardware support.
 *  * Many other hams with code, ideas, critics and opinions
 *
 * Latest version is always found on the Github repository (URL below)
 * https://www.github.com/pavelmc/arduino-arcs/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/

/*******************************************************************************
 * Important information you need to know about this code & Si5351
 *
 * We use a Si5351a control code that presume this:
 *  * We use CLK0 & CLK1 ONLY
 *  * CLK0 use PLLA & CLK1 use PLLB
 *  * CLK0 is employed as VFO (to mix with the RF from/to the antenna)
 *  * CLK1 is employed as BFO (to mix with the IF to mod/demodulate the audio)
 *  * We use the full power in all outputs (8mA ~0dB)
 *
 *  * Please have in mind that this IC has a SQUARE wave output and you need to
 *    apply some kind of low-pass/band-pass filtering to smooth it and get rid
 *    of the nasty harmonics
 ******************************************************************************/

/****************************** FEATURES SEGMENTATION *************************
* Here we activate/deactivate some of the sketch features, it's just a matter
* of comment out the feature and it will keep out of compilation (smaller code)
*
* For example: one user requested a "headless" mode: no lcd, no buttons, just
* cat control via serial/usb from the PC. for that we have the headless mode.
*
* Uncomment the line that define the HEADLESS macro an you are done.
* (uncomment it is just to remove the two slashes before it)
*******************************************************************************/
#define CAT_CONTROL True
#define ABUT True
#define ROTARY True

// if you want a headless control unit just uncomment this line below and 
// you will get no LCD / buttons / rotary; only cat control
//#define HEADLESS True

#ifdef HEADLESS
    // no LCD 
    #define NOLCD True

    // no Analog Buttons
    #ifdef ABUT
        #undef ABUT
    #endif // ABUT

    // no rotary ot push buttons
    #ifdef ROTARY
        #undef ROTARY
    #endif // ROTARY

    // YES CAT_CONTROL
    #ifndef CAT_CONTROL
        #define CAT_CONTROL True
    #endif  // CAT_CONTROL
#endif  // headless

// default (non optional) libraries loading
#include <EEPROM.h>         // default
#include <Wire.h>           // default


// the fingerprint to know the EEPROM is initialized, we need to stamp something
// on it, as the 5th birthday anniversary of my daughter was the date I begin to
// work on this project, so be it: 2016 June 1st
#define EEPROMfingerprint "20160601"

// The eeprom & sketch version; if the eeprom version is lower than the one on
// the sketch we force an update (init) to make a consistent work on upgrades
#define EEP_VER     4
#define FMW_VER     12

// The index in the eeprom where to store the info
#define ECPP 0  // conf store up to 36 bytes so far.

// the limits of the VFO, for now just 40m for now; you can tweak it with the
// limits of your particular hardware, again this are LCD diplay frequencies.
#define F_MIN      6500000     // 6.500.000
#define F_MAX      7500000     // 7.500.000

// PTT IN/OUT pin
#define PTT     13              // PTT actuator, this will put the radio on TX
                                // this match the led on pin 13 with the PTT
#define inPTT   12              // PTT/CW KEY Line with pullup


#ifdef ROTARY
    // Enable weak pullups in the rotary lib before inclusion
    #define ENABLE_PULLUPS

    // include the libs
    #include <Rotary.h>         // https://github.com/mathertel/RotaryEncoder/
    #include <Bounce2.h>        // https://github.com/thomasfredericks/Bounce2/
    
    // define encoder pins
    #define ENC_A    3              // Encoder pin A
    #define ENC_B    2              // Encoder pin B
    #define btnPush 11              // Encoder Button

    // rotary encoder library setup
    Rotary encoder = Rotary(ENC_A, ENC_B);

    // the debounce instances
    #define debounceInterval  10    // in milliseconds
    Bounce dbBtnPush = Bounce();
    Bounce dbPTT = Bounce();
#endif  // rotary


#ifdef ABUT
    // define the max count for analog buttons in the BMux library
    #define BUTTONS_COUNT 4
    
    // define the sampling rate used
    #define BMUX_SAMPLING 10    // 10 samples per second
    
    // include the lib
    #include <BMux.h>           //  https://github.com/pavelmc/BMux/
    
    // define the analog pin to handle the buttons
    #define KEYS_PIN  2

    // instantiate it
    BMux abm;

    // Creating the analog buttons for the BMux lib; see the BMux doc for details
    // you may have to tweak this values a little for your hardware case
    Button bvfoab   = Button(510, &btnVFOABClick);      // 10k
    Button bmode    = Button(316, &btnModeClick);       // 4.7k
    Button brit     = Button(178, &btnRITClick);        // 2.2k
    Button bsplit   = Button(697, &btnSPLITClick);      // 22k
#endif  //abut


#ifdef CAT_CONTROL
    // library include
    #include <ft857d.h>         // https://github.com/pavelmc/ft857d/
    
    // instantiate it
    ft857d cat = ft857d();
#endif  // cat


#ifndef NOLCD
    // lib include
    #include <LiquidCrystal.h>  // default

    // lcd pins assuming a 1602 (16x2) at 4 bits
    // COLAB shield + Arduino Mini/UNO Board
    #define LCD_RS      5
    #define LCD_E       6
    #define LCD_D4      7
    #define LCD_D5      8
    #define LCD_D6      9
    #define LCD_D7      10

    // lcd library instantiate
    LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
#endif // nolcd


// run mode constants
#define MODE_LSB 0
#define MODE_USB 1
#define MODE_CW  2
#define MODE_MAX 2                // the mode count to cycle (-1)
#define MAX_RIT 99900             // RIT 9.99 Khz * 10

// config constants
#define CONFIG_IF       0
#define CONFIG_VFO_A    1
#define CONFIG_MODE_A   2
#define CONFIG_LSB      3
#define CONFIG_USB      4
#define CONFIG_CW       5
#define CONFIG_PPM      6
// --
#define CONFIG_MAX 6               // the amount of configure options

// Tick interval for the timed actions like the SMeter and the autosave
#define TICK_INTERVAL  250

// EERPOM saving interval (if some parameter has changed) in 1/4 seconds; var i
// word so max is 65535 in 1/4 secs is ~ 16383 sec ~ 273 min ~ 4h 33 min
#define SAVE_INTERVAL 2400      // 10 minutes = 60 sec * 10 * 4 ticks

// hardware pre configured values
// Pre configured values for a Single conversion radio using the FT-747GX
long lsb =          -1450;
long usb =           1450;
long cw =            0;
long ifreq =         10000000; // 8212980

// This value is not the real PPM value is just the freq correction for your
// particular xtal from the 27.00000 Mhz one, if you can measure it put it here
long si5351_ppm = 2256;

// Si5351a Xtal
#define XTAL   27000000;          // default FREQ of the XTAL for the Si5351a

// the variables
long XTAL_C = XTAL;               // corrected xtal with the ppm
long vfoa = 7110000;              // default starting VFO A freq
long vfob = 7125000;              // default starting VFO B freq
long tvfo = 0;                    // temporal VFO storage for RIT usage
long txSplitVfo = 0;              // temporal VFO storage for RIT usage when TX
byte step = 2;                    // default steps position index:
                                  // as 1*10E2 = 100 = 100hz; step position is
                                  // calculated to avoid to use a big array
                                  // see getStep() function
boolean update = true;            // lcd update flag in normal mode
byte encoderState = 0;            // encoder state (DIR_NONE)
byte config = 0;                  // holds the configuration item selected
boolean inSetup = false;          // the setup mode, just looking or modifying
#define STEP_SHOW_TIME  6         // the time the step must be shown in 1/4 secs
                                  // aprox 1.5 secs
byte showStepCounter = 0;         // the step timer counter
boolean runMode =      true;      // true: normal, false: setup
boolean activeVFO =    true;      // true: A, False: B
byte VFOAMode =        MODE_LSB;
byte VFOBMode =        MODE_LSB;
boolean ritActive =    false;     // true: rit active, false: rit disabled
boolean tx =           false;     // whether we are on TX mode or not
#ifndef NOLCD
    #define BARGRAPH_SAMPLES    6
    word pep[BARGRAPH_SAMPLES] = {};
                                        // s-meter readings storage
    boolean smeterOk = false;           // it's ok to show the bar graph
    word sMeter = 0;                    // hold the value of the Smeter readings
                                        // in both RX and TX modes
#endif

unsigned long lastMilis = 0;       // to track the last sampled time
boolean barReDraw =    true;       // bar needs to be redrawn from zero
boolean split =        false;      // this holds th split state
boolean catTX =        false;      // CAT command to go to PTT
word qcounter =        0;          // Timer to be incremented each 1/4 second
                                   // approximately, to trigger a EEPROM update
                                   // if needed

// temp vars (used in the loop function)
boolean tbool   = false;

// structured data: Main Configuration Parameters
struct mConf {
    char finger[9] =  EEPROMfingerprint;  // nine, all strings ends with a null
    byte version = EEP_VER;
    long ifreq;
    long vfoa;
    byte vfoaMode;
    long vfob;
    byte vfobMode;
    int lsb;
    int usb;
    int cw;
    int ppm;
};

// declaring the main configuration variable for mem storage
struct mConf conf;

// pointers to the actual values
long *ptrVFO;       // will hold the value of the selected VFO
byte *ptrMode;      // idem but for the mode of the *ptrVFO


/*********** MISCELLANEOUS FUNCTIONS RELATED TO CORE PROCEDURES *************/


// split check
void splitCheck() {
    if (split) {
        // revert back the VFO
        activeVFO = !activeVFO;
        updateAllFreq();
    }
}


// change the modes in a cycle
void changeMode() {
    // normal increment
    *ptrMode += 1;

    // checking for overflow
    if (*ptrMode > MODE_MAX) *ptrMode = 0;

    // Apply the changes
    updateAllFreq();
}


// change the steps
void changeStep() {
    // calculating the next step
    if (step < 7) {
        // simply increment
        step += 1;
    } else {
        // default start mode is 2 (100Hz)
        step = 2;
        // in setup mode and just specific modes it's allowed to go to 1 hz
        boolean am = false;
        am = am or (config == CONFIG_LSB) or (config == CONFIG_USB);
        am = am or (config == CONFIG_PPM);
        if (!runMode and am) step = 1;
    }

    // if in normal mode reset the counter to show the change in the LCD
    if (runMode) showStepCounter = STEP_SHOW_TIME;
}

// return the right step size to move
long getStep () {
    // we get the step from the global step var
    long ret = 1;
    for (byte i=0; i < step; i++, ret *= 10);
    return ret/10;
}


// hardware or software commands in to RX
void going2RX() {
    // PTT released, going to RX
    tx = false;
    digitalWrite(PTT, LOW);

    // make changes if tx goes active when RIT is active
    if (ritActive) {
        // get the TX vfo and store it as the reference for the RIT
        tvfo = *ptrVFO;
        // restore the rit VFO to the actual VFO
        *ptrVFO = txSplitVfo;
    }

    // make the split changes if needed
    splitCheck();
}


// hardware or software commands in to TX
void going2TX() {
    // PTT asserted, going into TX
    tx = true;
    digitalWrite(PTT, 1);

    // make changes if tx goes active when RIT is active
    if (ritActive) {
        // save the actual rit VFO
        txSplitVfo = *ptrVFO;
        // set the TX freq to the active VFO
        *ptrVFO = tvfo;
    }

    // make the split changes if needed
    splitCheck();
}


// swaps the VFOs
void swapVFO(byte force = 2) {
    // swap the VFOs if needed
    if (force == 2) {
        // just toggle it
        activeVFO = !activeVFO;
    } else {
        // set it as commanded
        activeVFO = bool(force);
    }

    // setting the VFO/mode pointers
    if (activeVFO) {
        ptrVFO = &vfoa;
        ptrMode = &VFOAMode;
    } else {
        ptrVFO = &vfob;
        ptrMode = &VFOBMode;
    }
}


/*****************************************************************************
 *
 *                          Where the other functions go?
 *
 * This sketch use the split in files feature of the Arduino IDE, look for tabs
 * in you editor.
 *
 * Tabs are organized by group of functions preceded with the letters "fx-"
 * where "x" is a sequence to preserve order in the files up on load
 *
 * The "setup" and "loop" are placed in the file "z-end" to get placed at the
 * end when the compiler does it's magic.
 *
 * ***************************************************************************/


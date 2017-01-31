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
* Uncomment the line that define the HEADLESS macro an you a re done.
*******************************************************************************/
#define CAT_CONTROL True
#define ABUT True
#define ROTARY True
#define SMETER True

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


#ifdef NOLCD
    // then we must unset the SMETER as we don't have a LCD to show smeter
    #ifdef SMETER
        #undef SMETER
    #endif  // smeter
#endif  // nolcd


#ifdef ABUT
    // define the max count for analog buttons in the BMux library
    #define BUTTONS_COUNT 4
#endif


// now the main include block
#include <EEPROM.h>         // default
#include <Wire.h>           // default
#ifndef NOLCD
    #include <LiquidCrystal.h>  // default
#endif  // nolcd


#ifdef ROTARY
    // Enable weak pullups in the rotary lib before inclusion
    #define ENABLE_PULLUPS
    
    #include <Rotary.h>         // https://github.com/mathertel/RotaryEncoder/
    #include <Bounce2.h>        // https://github.com/thomasfredericks/Bounce2/
#endif  // rotary


// optional features libraries
#ifdef CAT_CONTROL
    #include <ft857d.h>         // https://github.com/pavelmc/ft857d/
#endif // cat_control


#ifdef ABUT
    #define BMUX_SAMPLING 10    // 10 samples per second
    #include <BMux.h>           // https://github.com/pavelmc/BMux/
#endif // abut

// the fingerprint to know the EEPROM is initialized, we need to stamp something
// on it, as the 5th birthday anniversary of my daughter was the date I begin to
// work on this project, so be it: 2016 June 1st
#define EEPROMfingerprint "20160601"

/************************** USER BOARD SELECTION *******************************
 * if you have the any of the CO-LAB shields uncomment the following line.
 * (the sketch is configured by default for my particular hardware)
 ******************************************************************************/
#define COLAB

// The eeprom & sketch version; if the eeprom version is lower than the one on
// the sketch we force an update (init) to make a consistent work on upgrades
#define EEP_VER     4
#define FMW_VER     10

// The index in the eeprom where to store the info
#define ECPP 0  // conf store up to 36 bytes so far.

// the limits of the VFO, for now just 40m for now; you can tweak it with the
// limits of your particular hardware, again this are LCD diplay frequencies.
#define F_MIN      6500000     // 6.500.000
#define F_MAX      7500000     // 7.500.000

// PTT OUT pin
#define PTT     13              // PTT actuator, this will put the radio on TX
                                // this match the led on pin 13 with the PTT
#define inPTT   12              // PTT/CW KEY Line with pullup


#ifdef ROTARY
    // encoder pins
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
    // analog buttons library declaration (BMux)
    // define the analog pin to handle the buttons
    #define KEYS_PIN  2
    BMux abm;

    // Creating the analog buttons for the BMux lib; see the BMux doc for details
    // you may have to tweak this values a little for your hardware case
    Button bvfoab   = Button(510, &btnVFOABClick);      // 10k
    Button bmode    = Button(316, &btnModeClick);       // 4.7k
    Button brit     = Button(178, &btnRITClick);        // 2.2k
    Button bsplit   = Button(697, &btnSPLITClick);      // 22k
#endif  //abut


#ifdef CAT_CONTROL
    // the CAT radio lib
    ft857d cat = ft857d();
#endif


#ifndef NOLCD
    // lcd pins assuming a 1602 (16x2) at 4 bits
    // COLAB shield + Arduino Mini/UNO Board
    #define LCD_RS      5
    #define LCD_E       6
    #define LCD_D4      7
    #define LCD_D5      8
    #define LCD_D6      9
    #define LCD_D7      10
#endif // nolcd


#ifndef NOLCD
    // lcd library setup
    LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

    #ifdef SMETER
        // defining the chars for the Smeter
        byte bar[8] = {
          B11111,
          B11111,
          B11111,
          B10001,
          B11111,
          B11111,
          B11111
        };

        byte s1[8] = {
          B11111,
          B10011,
          B11011,
          B11011,
          B11011,
          B10001,
          B11111
        };

        byte s3[8] = {
          B11111,
          B10001,
          B11101,
          B10001,
          B11101,
          B10001,
          B11111
        };

        byte s5[8] = {
          B11111,
          B10001,
          B10111,
          B10001,
          B11101,
          B10001,
          B11111
        };

        byte s7[8] = {
          B11111,
          B10001,
          B11101,
          B11011,
          B11011,
          B11011,
          B11111
        };

        byte s9[8] = {
          B11111,
          B10001,
          B10101,
          B10001,
          B11101,
          B11101,
          B11111
        };
    #endif  // smeter
#endif  // nolcd


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
#ifdef SMETER
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


/******************************* MISCELLANEOUS ********************************/


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


#ifdef ABUT
    // RIT toggle
    void toggleRit() {
        if (!ritActive) {
            // going to activate it: store the active VFO
            tvfo = *ptrVFO;
            ritActive = true;
        } else {
            // going to deactivate: reset the stored VFO
            *ptrVFO = tvfo;
            ritActive = false;
            // flag to redraw the bar graph
            barReDraw = true;
        }
    }
#endif  // abut


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


// check some values don't go below zero
void belowZero(long *value) {
    // some values are not meant to be below zero, this check and fix that
    if (*value < 0) *value = 0;
}


/******************************** ENCODER *************************************/


#ifdef ROTARY
    // the encoder has moved
    void encoderMoved(int dir) {
        // check the run mode
        if (runMode) {
            // update freq
            updateFreq(dir);
            update = true;
        } else {
            #ifndef NOLCD   // no meaning if no lcd
                // update the values in the setup mode
                updateSetupValues(dir);
            #endif  // nolcd
        }
    }



    // update freq procedure
    void updateFreq(int dir) {
        long freq = *ptrVFO;

        if (ritActive) {
            // we fix the steps to 10 Hz in rit mode
            freq += 100 * dir;
            // check we don't exceed the MAX_RIT
            if (abs(tvfo - freq) > MAX_RIT) return;
        } else {
            // otherwise we use the default step on the environment
            freq += getStep() * dir;
            // check we don't exceed the limits
            if(freq > F_MAX) freq = F_MIN;
            if(freq < F_MIN) freq = F_MAX;
        }

        // apply the change
        *ptrVFO = freq;

        // update the output freq
        setFreqVFO();
    }
#endif  // rotary

/************************** LCD INTERFACE RELATED *****************************/

#ifndef NOLCD
    // update the setup values
    void updateSetupValues(int dir) {
        // we are in setup mode, showing or modifying?
        if (!inSetup) {
            // just showing, show the config on the LCD
            updateShowConfig(dir);
        } else {
            // change the VFO to A by default
            swapVFO(1);
            // I'm modifying, switch on the config item
            switch (config) {
                case CONFIG_IF:
                    // change the IF value
                    ifreq += getStep() * dir;
                    belowZero(&ifreq);
                    break;
                case CONFIG_VFO_A:
                    // change VFOa
                    *ptrVFO += getStep() * dir;
                    belowZero(ptrVFO);
                    break;
                case CONFIG_MODE_A:
                    // hot swap it
                    changeMode();
                    // set the default mode in the VFO A
                    showModeSetup(VFOAMode);
                    break;
                case CONFIG_USB:
                    // change the mode to USB
                    *ptrMode = MODE_USB;
                    // change the USB BFO
                    usb += getStep() * dir;
                    break;
                case CONFIG_LSB:
                    // change the mode to LSB
                    *ptrMode = MODE_LSB;
                    // change the LSB BFO
                    lsb += getStep() * dir;
                    break;
                case CONFIG_CW:
                    // change the mode to CW
                    *ptrMode = MODE_CW;
                    // change the CW BFO
                    cw += getStep() * dir;
                    break;
                case CONFIG_PPM:
                    // change the Si5351 PPM
                    si5351_ppm += getStep() * dir;
                    // instruct the lib to use the new ppm value
                    XTAL_C = XTAL + si5351_ppm;
                    break;
            }

            // for all cases update the freqs
            updateAllFreq();

            // update el LCD
            showModConfig();
        }
    }


    // update the configuration item before selecting it
    void updateShowConfig(int dir) {
        // move the config item, it's a byte, so we use a temp var here
        int tconfig = config;
        tconfig += dir;

        if (tconfig > CONFIG_MAX) tconfig = 0;
        if (tconfig < 0) tconfig = CONFIG_MAX;
        config = tconfig;

        // update the LCD
        showConfig();
    }


    // show the mode for the passed mode in setup mode
    void showModeSetup(byte mode) {
        // now I have to print it out
        lcd.setCursor(0, 1);
        spaces(11);
        showModeLcd(mode); // this one has a implicit extra space after it
    }


    // print a string of spaces, to save some flash/eeprom "space"
    void spaces(byte m) {
        // print m spaces in the LCD
        while (m) {
            lcd.print(" ");
            m--;
        }
    }


    // show the labels of the config
    void showConfigLabels() {
        switch (config) {
            case CONFIG_IF:
                lcd.print(F("  IF frequency  "));
                break;
            case CONFIG_VFO_A:
                lcd.print(F("   VFO A freq   "));
                break;
            case CONFIG_MODE_A:
                lcd.print(F("   VFO A mode   "));
                break;
            case CONFIG_USB:
                lcd.print(F(" BFO freq. USB  "));
                break;
            case CONFIG_LSB:
                lcd.print(F(" BFO freq. LSB  "));
                break;
            case CONFIG_CW:
                lcd.print(F(" BFO freq. CW   "));
                break;
            case CONFIG_PPM:
                lcd.print(F("Si5351 PPM error"));
                break;
        }
    }


    // show the setup main menu
    void showConfig() {
        // we have update the whole LCD screen
        lcd.setCursor(0, 0);
        lcd.print(F("#> SETUP MENU <#"));
        lcd.setCursor(0, 1);
        // show the specific item label
        showConfigLabels();
    }


    // print the sign of a passed parameter
    void showSign(long val) {
        // just print it
        if (val > 0) lcd.print("+");
        if (val < 0) lcd.print("-");
        if (val == 0) lcd.print(" ");
    }


    // show the ppm as a signed long
    void showConfigValue(long val) {
        lcd.print(F("Val:"));

        // Show the sign only on config and not VFO & IF
        boolean t;
        t = config == CONFIG_VFO_A or config == CONFIG_IF;
        if (!runMode and !t) showSign(val);

        // print it
        formatFreq(abs(val));

        // if on normal mode we show in 10 Hz
        if (runMode) lcd.print("0");
        lcd.print(F("hz"));
    }


    // update the specific setup item
    void showModConfig() {
        lcd.setCursor(0, 0);
        showConfigLabels();

        // show the specific values
        lcd.setCursor(0, 1);
        switch (config) {
            case CONFIG_IF:
                showConfigValue(ifreq);
                break;
            case CONFIG_VFO_A:
                showConfigValue(vfoa);
                break;
            case CONFIG_MODE_A:
                showModeSetup(VFOAMode);
            case CONFIG_USB:
                showConfigValue(usb);
                break;
            case CONFIG_LSB:
                showConfigValue(lsb);
                break;
            case CONFIG_CW:
                showConfigValue(cw);
                break;
            case CONFIG_PPM:
                showConfigValue(si5351_ppm);
                break;
        }
    }


    // format the freq to easy viewing
    void formatFreq(long freq) {
        // for easy viewing we format a freq like 7.110 to 7.110.00
        long t;

        // Mhz part
        t = freq / 1000000;
        if (t < 10) lcd.print(" ");
        if (t == 0) {
            spaces(2);
        } else {
            lcd.print(t);
            // first dot: optional
            lcd.print(".");
        }
        // Khz part
        t = (freq % 1000000);
        t /= 1000;
        if (t < 100) lcd.print("0");
        if (t < 10) lcd.print("0");
        lcd.print(t);
        // second dot: forced
        lcd.print(".");
        // hz part
        t = (freq % 1000);
        if (t < 100) lcd.print("0");
        // check if in config and show up to 1hz resolution
        if (!runMode) {
            if (t < 10) lcd.print("0");
            lcd.print(t);
        } else {
            lcd.print(t/10);
        }
    }


    // lcd update in normal mode
    void updateLcd() {
        // this is the designed normal mode LCD
        /******************************************************
         *   0123456789abcdef
         *  ------------------
         *  |A 14.280.25 lsb |
         *  |RX 0000000000000|
         *
         *  |RX +9.99 Khz    |
         *
         *  |RX 100hz        |
         *  ------------------
         ******************************************************/

        // first line
        lcd.setCursor(0, 0);
        // active a?
        if (activeVFO) {
            lcd.print("A");
        } else {
            lcd.print("B");
        }

        // split?
        if (split) {
            // ok, show the split status as a * sign
            lcd.print("*");
        } else {
            // print a separator.
            spaces(1);
        }

        // show VFO and mode
        formatFreq(*ptrVFO);
        spaces(1);
        showModeLcd(*ptrMode);

        // second line
        lcd.setCursor(0, 1);
        if (tx) {
            lcd.print(F("TX "));
        } else {
            lcd.print(F("RX "));
        }

        // if we have a RIT or steps we manage it here and the bar will hold
        if (ritActive) showRit();
    }


    // show rit in LCD
    void showRit() {
        /***************************************************************************
         * RIT show something like this on the line of the non active VFO
         *
         *   |0123456789abcdef|
         *   |----------------|
         *   |RX RIT -9.99 khz|
         *   |----------------|
         *
         *             WARNING !!!!!!!!!!!!!!!!!!!!1
         *  If the user change the VFO we need to *RESET* & disable the RIT ASAP.
         *
         **************************************************************************/

        // get the active VFO to calculate the deviation & scale it down
        long diff = (*ptrVFO - tvfo)/10;

        // we start on line 2, char 3 of the second line
        lcd.setCursor(3, 1);
        lcd.print(F("RIT "));

        // show the difference in Khz on the screen with sign
        // diff can overflow the input of showSign, so we scale it down
        showSign(diff);

        // print the freq now, we have a max of 10 Khz (9.990 Khz)
        diff = abs(diff);

        // Khz part (999)
        word t = diff / 100;
        lcd.print(t);
        lcd.print(".");
        // hz part
        t = diff % 100;
        if (t < 10) lcd.print("0");
        lcd.print(t);
        spaces(1);
        // unit
        lcd.print(F("kHz"));
    }


    // show the mode on the LCD
    void showModeLcd(byte mode) {
        // print it
        switch (mode) {
            case MODE_USB:
              lcd.print(F("USB "));
              break;
            case MODE_LSB:
              lcd.print(F("LSB "));
              break;
            case MODE_CW:
              lcd.print(F("CW  "));
              break;
        }
    }


    // show the vfo step
    void showStep() {
        // in nomal or setup mode?
        if (runMode) {
            // in normal mode is the second line, third char
            lcd.setCursor(3, 1);
        } else {
            // in setup mode is just in the begining of the second line
            lcd.setCursor(0, 1);
        }

        // show it
        if (step == 1) lcd.print(F("  1Hz"));
        if (step == 2) lcd.print(F(" 10Hz"));
        if (step == 3) lcd.print(F("100Hz"));
        if (step == 4) lcd.print(F(" 1kHz"));
        if (step == 5) lcd.print(F("10kHz"));
        if (step == 6) lcd.print(F(" 100k"));
        if (step == 7) lcd.print(F(" 1MHz"));
        spaces(11);
    }
#endif  // nolcd


/*********************************** Si5351 *********************************/


// Disable any output (0-7)
void si5351aDisableCLK(byte clk) {
    si5351ai2cWrite(16 + clk, 0x80);
}


// Frequency in Hz; must be within [7,810 kHz to ~220 MHz]
void si5351aSetFrequency(byte clk, unsigned long frequency) { 
    #define c 1048574;
    unsigned long fvco;
    unsigned long outdivider;
    byte R = 1;
    byte a;
    unsigned long b;
    float f;
    unsigned long MSx_P1;
    unsigned long MSNx_P1;
    unsigned long MSNx_P2;
    unsigned long MSNx_P3;
    byte shifts = 0;

    // With 900 MHz beeing the maximum internal PLL-Frequency
    outdivider = 900000000 / frequency;

    // If output divider out of range (>900) use additional Output divider
    while (outdivider > 900) {
        R = R * 2;
        outdivider = outdivider / 2;
    }

    // finds the even divider which delivers the intended Frequency
    if (outdivider % 2) outdivider--;

    // Calculate the PLL-Frequency (given the even divider)
    fvco = outdivider * R * frequency;

    // Convert the Output Divider to the bit-setting required in register 44
    switch (R) {
        case 1: R = 0; break;
        case 2: R = 16; break;
        case 4: R = 32; break;
        case 8: R = 48; break;
        case 16: R = 64; break;
        case 32: R = 80; break;
        case 64: R = 96; break;
        case 128: R = 112; break;
    }

    a = fvco / XTAL_C;
    f = fvco - a * XTAL_C;
    f = f * c;
    f = f / XTAL_C;
    b = f;

    MSx_P1 = 128 * outdivider - 512;
    f = 128 * b / c;
    MSNx_P1 = 128 * a + f - 512;
    MSNx_P2 = f;
    MSNx_P2 = 128 * b - MSNx_P2 * c; 
    MSNx_P3 = c;

    // select the clk to write and disable it's output
    si5351aDisableCLK(clk);

    if (clk > 0 ) shifts = 8;

    // plls
    si5351ai2cWrite(26 + shifts, (MSNx_P3 & 65280) >> 8);   // Bits [15:8] of MSNx_P3 in register 26
    if (clk == 0) {
        si5351ai2cWrite(27, MSNx_P3 & 255);
        si5351ai2cWrite(28, (MSNx_P1 & 196608) >> 16);
    } else {
        si5351ai2cWrite(35, MSNx_P1 & 255);
        si5351ai2cWrite(36, (MSNx_P2 & 0x00030000) >> 10);
    }
    si5351ai2cWrite(29 + shifts, (MSNx_P1 & 65280) >> 8);   // Bits [15:8]  of MSNx_P1 in register 29
    si5351ai2cWrite(30 + shifts, MSNx_P1 & 255);            // Bits [7:0]  of MSNx_P1 in register 30
    si5351ai2cWrite(31 + shifts, ((MSNx_P3 & 983040) >> 12) | ((MSNx_P2 & 983040) >> 16)); // Parts of MSNx_P3 and MSNx_P1
    si5351ai2cWrite(32 + shifts, (MSNx_P2 & 65280) >> 8);   // Bits [15:8]  of MSNx_P2 in register 32
    si5351ai2cWrite(33 + shifts, MSNx_P2 & 255);            // Bits [7:0]  of MSNx_P2 in register 33

    shifts = clk * 8;

    // multisynths
    si5351ai2cWrite(42 + shifts, 0);                        // Bits [15:8] of MS0_P3 (always 0) in register 42
    si5351ai2cWrite(43 + shifts, 1);                        // Bits [7:0]  of MS0_P3 (always 1) in register 43
    si5351ai2cWrite(44 + shifts, ((MSx_P1 & 196608) >> 16) | R);  // Bits [17:16] of MSx_P1 in bits [1:0] and R in [7:4]
    si5351ai2cWrite(45 + shifts, (MSx_P1 & 65280) >> 8);    // Bits [15:8]  of MSx_P1 in register 45
    si5351ai2cWrite(46 + shifts, MSx_P1 & 255);             // Bits [7:0]  of MSx_P1 in register 46
    si5351ai2cWrite(47 + shifts, 0);                        // Bits [19:16] of MS0_P2 and MS0_P3 are always 0
    si5351ai2cWrite(48 + shifts, 0);                        // Bits [15:8]  of MS0_P2 are always 0
    si5351ai2cWrite(49 + shifts, 0);                        // Bits [7:0]   of MS0_P2 are always 0
    if (outdivider == 4 and clk == 0) {
        si5351ai2cWrite(44, 12 | R);       // Special settings for R = 4 (see datasheet)
        si5351ai2cWrite(45, 0);                    // Bits [15:8]  of MSx_P1 must be 0
        si5351ai2cWrite(46, 0);                    // Bits [7:0]  of MSx_P1 must be 0
    }
    
    // PLL reset
    if (clk == 0) {
        // This soft-resets PLL A & and enable it's output
        si5351ai2cWrite(177, 32);
        si5351ai2cWrite(16, 79);
    } else {
        // This soft-resets PLL B & and enable it's output
        si5351ai2cWrite(177, 128);
        si5351ai2cWrite(17, 111);
    }
}

void si5351ai2cWrite(byte regist, byte value){
  Wire.beginTransmission(96);
  Wire.write(regist);
  Wire.write(value);
  Wire.endTransmission();
}


// set the calculated freq to the VFO
void setFreqVFO() {
    // temp var to hold the calculated value
    long freq = *ptrVFO + ifreq;

    // apply the ssb factor
    if (*ptrMode == MODE_USB) freq += usb;
    if (*ptrMode == MODE_LSB) freq += lsb;
    if (*ptrMode == MODE_CW)  freq += cw;

    // set the Si5351 up with the change
    si5351aSetFrequency(0, freq);
}


// Force freq update for all the environment vars
void updateAllFreq() {
    // VFO update
    setFreqVFO();

    // BFO update
    long freq = ifreq;

    // mod it by mode
    if (*ptrMode == MODE_USB) freq += usb;
    if (*ptrMode == MODE_LSB) freq += lsb;
    if (*ptrMode == MODE_CW)  freq += cw;

    // deactivate it if zero
    if (freq == 0) {
        // deactivate it
        si5351aDisableCLK(1);
    } else {
        // output it
        si5351aSetFrequency(1, freq);
    }
}


/********************************** EEPROM ************************************/


// check if the EEPROM is initialized
boolean checkInitEEPROM() {
    // read the eeprom config data
    EEPROM.get(ECPP, conf);

    // check for the initializer and version
    if (conf.version == EEP_VER) {
        // so far version is the same, but the fingerprint has a different trick
        for (int i=0; i<sizeof(conf.finger); i++) {
            if (conf.finger[i] != EEPROMfingerprint[i]) return false;
        }
        // if it reach this point is because it's the same
        return true;
    }

    // return false: default
    return false;
}


// initialize the EEPROM mem, also used to store the values in the setup mode
// this procedure has a protection for the EEPROM life using update semantics
// it actually only write a cell if it has changed
void saveEEPROM() {
    // load the parameters in the environment
    conf.vfoa       = vfoa;
    conf.vfoaMode   = VFOAMode;
    conf.vfob       = vfob;
    conf.vfobMode   = VFOBMode;
    conf.ifreq      = ifreq;
    conf.lsb        = lsb;
    conf.usb        = usb;
    conf.cw         = cw;
    conf.ppm        = si5351_ppm;
    conf.version    = EEP_VER;
    strcpy(conf.finger, EEPROMfingerprint);

    // write it
    EEPROM.put(ECPP, conf);
}


// load the eprom contents
void loadEEPROMConfig() {
    // write it
    EEPROM.get(ECPP, conf);

    // load the parameters to the environment
    vfoa        = conf.vfoa;
    VFOAMode    = conf.vfoaMode;
    vfob        = conf.vfob;
    VFOBMode    = conf.vfobMode;
    lsb         = conf.lsb;
    usb         = conf.usb;
    cw          = conf.cw;
    XTAL_C = XTAL + conf.ppm;
}


/**************************** SMETER / TX POWER *******************************/

#ifdef SMETER
    // show the bar graph for the RX or TX modes
    void showBarGraph() {
        // we are working on a 2x16 and we have 13 bars to show (0-12)
        unsigned long ave = 0, i;
        volatile static byte barMax = 0;

        // find the average
        for (i=0; i<BARGRAPH_SAMPLES; i++) ave += pep[i];
        ave /= BARGRAPH_SAMPLES;

        // set the smeter reading on the global scope for CAT readings
        sMeter = ave;

        // scale it down to 0-12 from word
        byte local = map(ave, 0, 1023, 0, 12);

        // printing only the needed part of the bar, if growing or shrinking
        // if the same no action is required, remember we have to minimize the
        // writes to the LCD to minimize QRM

        // if we get a barReDraw = true; then reset to redrawn the entire bar
        if (barReDraw) {
            barMax = 0;
            // forcing the write of one line
            if (local == 0) local = 1;
        }

        // growing bar: print the difference
        if (local > barMax) {
            // LCD position & print the bars
            lcd.setCursor(3 + barMax, 1);

            // write it
            for (i = barMax; i <= local; i++) {
                switch (i) {
                    case 0:
                        lcd.write(byte(1));
                        break;
                    case 2:
                        lcd.write(byte(2));
                        break;
                    case 4:
                        lcd.write(byte(3));
                        break;
                    case 6:
                        lcd.write(byte(4));
                        break;
                    case 8:
                        lcd.write(byte(5));
                        break;
                    default:
                        lcd.write(byte(0));
                        break;
                }
            }

            // second part of the erase, preparing for the blanking
            if (barReDraw) barMax = 12;
        }

        // shrinking bar: erase the old ones print spaces to erase just the diff
        if (barMax > local) {
            i = barMax;
            while (i > local) {
                lcd.setCursor(3 + i, 1);
                lcd.print(" ");
                i--;
            }
        }

        // put the var for the next iteration
        barMax = local;
        //reset the redraw flag
        barReDraw = false;
    }


    // take a sample an inject it on the array
    void takeSample() {
        // reference is 5v
        word val;

        if (!tx) {
            val = analogRead(1);
        } else {
            val = analogRead(0);
        }

        // push it in the array
        for (byte i = 0; i < BARGRAPH_SAMPLES - 1; i++) pep[i] = pep[i + 1];
        pep[BARGRAPH_SAMPLES - 1] = val;
    }


    // smeter reading, this take a sample of the smeter/txpower each time; an will
    // rise a flag when they have rotated the array of measurements 2/3 times to
    // have a moving average
    void smeter() {
        // static smeter array counter
        volatile static byte smeterCount = 0;

        // no matter what, I must keep taking samples
        takeSample();

        // it has rotated already?
        if (smeterCount > (BARGRAPH_SAMPLES * 2 / 3)) {
            // rise the flag about the need to show the bar graph and reset the count
            smeterOk = true;
            smeterCount = 0;
        } else {
            // just increment it
            smeterCount += 1;
        }
    }
#endif

/******************************* CAT ******************************************/

#ifdef CAT_CONTROL
    // instruct the sketch that must go in/out of TX
    void catGoPtt(boolean tx) {
        if (tx) {
            // going to TX
            going2TX();
        } else {
            // goint to RX
            going2RX();
        }

        // update the state
        update = true;
    }


    // set VFO toggles from CAT
    void catGoToggleVFOs() {
        activeVFO = !activeVFO;
        update = true;
    }


    // set freq from CAT
    void catSetFreq(long f) {
        // we use 1/10 hz so scale it
        f *= 10;

        // check for the freq boundaries
        if (f > F_MAX) return;
        if (f < F_MIN) return;

        // set the freq for the active VFO
        *ptrVFO =  f;

        // apply changes
        updateAllFreq();
        update = true;
    }


    // set mode from CAT
    void catSetMode(byte m) {
        // the mode can be any of the CAT ones, we have to restrict it to our modes
        if (m > 2) return;  // no change

        // by luck we use the same mode than the CAT lib so far
        *ptrMode = m;

        // Apply the changes
        updateAllFreq();
        update = true;
    }


    // get freq from CAT
    long catGetFreq() {
        // get the active VFO freq and pass it
        return *ptrVFO / 10;
    }


    // get mode from CAT
    byte catGetMode() {
        // get the active VFO mode and pass it
        return *ptrMode;
    }


    // get the s meter status to CAT
    byte catGetSMeter() {
        // returns a byte in wich the s-meter is scaled to 4 bits (15)
        // it's scaled already this our code
        #ifdef SMETER
            return sMeter >> 6 ;
        #else
            return 0;
        #endif
    }


    // get the TXstatus to CAT
    byte catGetTXStatus() {
        // prepare a byte like the one the CAT wants:

        /*
         * this must return a byte in wich the different bits means this:
         * 0b abcdefgh
         *  a = 0 = PTT off
         *  a = 1 = PTT on
         *  b = 0 = HI SWR off
         *  b = 1 = HI SWR on
         *  c = 0 = split on
         *  c = 1 = split off
         *  d = dummy data
         *  efgh = PO meter data
         */

        // build the byte to return
        #ifdef SMETER
            return tx<<7 + split<<5 + sMeter;
        #else
            return tx<<7 + split<<5;
        #endif
    }


    // delay with CAT check, this is for the welcome screen
    // see the note in the setup; default ~2 secs
    void delayCat(int del = 2000) {
        // delay in msecs to wait, 2000 ~2 seconds by default
        long delay = millis() + del;
        long m = 0;

        // loop to waste time
        while (m < delay) {
            cat.check();
            m = millis();
        }
    }
#endif

/******************************* BUTTONS *************************************/

#ifdef ABUT
    // VFO A/B button click >>>> (OK/SAVE click)
    void btnVFOABClick() {
        // normal mode
        if (runMode) {
            // we force to deactivate the RIT on VFO change, as it will confuse
            // the users and have a non logical use, only if activated and
            // BEFORE we change the active VFO
            if (ritActive) toggleRit();

            // now we swap the VFO.
            swapVFO();

            // update VFO/BFO and instruct to update the LCD
            updateAllFreq();

            // set the LCD update flag
            update = true;
        } else {
            // SETUP mode
            if (!inSetup) {
                // I'm going to setup a element
                inSetup = true;

                // change the mode to follow VFO A
                if (config == CONFIG_USB) VFOAMode = MODE_USB;
                if (config == CONFIG_LSB) VFOAMode = MODE_LSB;

                #ifndef NOLCD
                    // config update on the LCD
                    showModConfig();
                #endif  // nolcd
            } else {
                // get out of the setup change
                inSetup = false;

                // save to the eeprom
                saveEEPROM();

                #ifndef NOLCD
                    // lcd delay to show it properly (user feedback)
                    lcd.setCursor(0, 0);
                    lcd.print(F("##   SAVED    ##"));
                    delay(1000);

                    // show setup
                    showConfig();
                #endif  // nolcd

                // reset the minimum step if set (1hz > 10 hz)
                if (step == 1) step = 2;
            }
        }
    }


    // MODE button click >>>> (CANCEL click)
    void btnModeClick() {
        // normal mode
        if (runMode) {
            changeMode();
            update = true;
        } else if (inSetup) {
            // setup mode, just inside a value edit, then get out of here
            inSetup = false;

            #ifndef NOLCD
                // user feedback
                lcd.setCursor(0, 0);
                lcd.print(F(" #  Canceled  # "));
                delay(1000);

                // show it
                showConfig();
            #endif  // nolcd
        }
    }


    // RIT button click >>>> (RESET values click)
    void btnRITClick() {
        // normal mode
        if (runMode) {
            toggleRit();
            update = true;
        } else if (inSetup) {
            // SETUP mode just inside a value edit.

            // where we are to know what to reset?
            if (config == CONFIG_LSB) lsb   = 0;
            if (config == CONFIG_USB) usb   = 0;
            if (config == CONFIG_CW)  cw    = 0;
            if (config == CONFIG_IF)  ifreq = 0;
            if (config == CONFIG_PPM) {
                // reset, ppm
                si5351_ppm = 0;
            }

            // update the freqs for
            updateAllFreq();
            #ifndef NOLCD
                showModConfig();
            #endif  // nolcd
        }
    }


    // SPLIT button click  >>>> (Nothing yet)
    void btnSPLITClick() {
        // normal mode
        if (runMode) {
            split = !split;
            update = true;
        }

        // no function in SETUP yet.
    }
#endif

/************************* SETUP and LOOP *************************************/


void setup() {
    #ifdef CAT_CONTROL
        // CAT Library setup
        cat.addCATPtt(catGoPtt);
        cat.addCATAB(catGoToggleVFOs);
        cat.addCATFSet(catSetFreq);
        cat.addCATMSet(catSetMode);
        cat.addCATGetFreq(catGetFreq);
        cat.addCATGetMode(catGetMode);
        cat.addCATSMeter(catGetSMeter);
        cat.addCATTXStatus(catGetTXStatus);
        // now we activate the library
        cat.begin(57600, SERIAL_8N1);
    #endif

    #ifndef NOLCD
        #ifdef SMETER
            // LCD init, create the custom chars first
            lcd.createChar(0, bar);
            lcd.createChar(1, s1);
            lcd.createChar(2, s3);
            lcd.createChar(3, s5);
            lcd.createChar(4, s7);
            lcd.createChar(5, s9);
        #endif  // smeter
        // now load the library
        lcd.begin(16, 2);
        lcd.clear();
    #endif  // nolcd

    #ifdef ABUT
        // analog buttons setup
        abm.init(KEYS_PIN, 3, 20);
        abm.add(bvfoab);
        abm.add(bmode);
        abm.add(brit);
        abm.add(bsplit);
    #endif

    #ifdef ROTARY
        // buttons debounce & PTT
        pinMode(btnPush, INPUT_PULLUP);
        dbBtnPush.attach(btnPush);
        dbBtnPush.interval(debounceInterval);
        // pin mode of the PTT
        pinMode(inPTT, INPUT_PULLUP);
        dbPTT.attach(inPTT);
        dbPTT.interval(debounceInterval);
        // default awake mode is RX
    #endif  // ROTARY

    // set PTT at the start to LOW aka RX
    pinMode(PTT, OUTPUT);
    digitalWrite(PTT, 0);

    // I2C init
    Wire.begin();

    // disable all the possible outputs (set freq enable it by default)
    for (byte i=0; i < 8; i++) si5351aDisableCLK(i);

    // check the EEPROM to know if I need to initialize it
    if (checkInitEEPROM()) {
        // just if it's already ok
        loadEEPROMConfig();
    } else {
        #ifndef NOLCD
            // full init, LCD banner by 1 second
            lcd.setCursor(0, 0);
            lcd.print(F("Init EEPROM...  "));
            lcd.setCursor(0, 1);
            lcd.print(F("Please wait...  "));
        #endif  // nolcd
        
        saveEEPROM();
        
        #ifdef CAT_CONTROL
            delayCat(); // 2 secs
        #else
            delay(2000);
        #endif

        #ifndef NOLCD
            lcd.clear();
        #endif  // nolcd
    }

    #ifndef NOLCD
        // Welcome screen
        lcd.clear();
        lcd.print(F("  Aduino Arcs  "));
        lcd.setCursor(0, 1);
        lcd.print(F("Fv: "));
        lcd.print(FMW_VER);
        lcd.print(F("  Mfv: "));
        lcd.print(EEP_VER);
    #endif  / nolcd

    // A software controlling the CAT via USB will reset the sketch upon
    // connection, so we need turn the cat.check() when running the welcome
    // banners (use case: Fldigi)
    #ifdef CAT_CONTROL
        delayCat(); // 2 secs
    #else
        delay(2000);
    #endif

    #ifndef NOLCD
        lcd.setCursor(0, 0);
        lcd.print(F(" by Pavel CO7WT "));
    #endif  // nolcd

    #ifdef CAT_CONTROL
        delayCat(1000); // 1 sec
    #else
        delay(1000);
    #endif

    #ifndef NOLCD
        lcd.clear(); 
    #endif  // nolcd

    #ifdef ROTRAY
        // Check for setup mode
        if (digitalRead(btnPush) == LOW) {
            #ifdef CAT_CONTROL
                // CAT is disabled in SETUP mode
                cat.enabled = false;
            #endif

            #ifndef NOLCD
                // we are in the setup mode
                lcd.setCursor(0, 0);
                lcd.print(F(" You are in the "));
                lcd.setCursor(0, 1);
                lcd.print(F("   SETUP MODE   "));
                delay(2000);
                lcd.clear();
                // show setup mode
                showConfig();
            #endif  // nolcd

            // rise the flag of setup mode for every body to see it.
            runMode = false;
        }
    #endif  // rotary

    // setting up VFO A as principal.
    activeVFO = true;
    ptrVFO = &vfoa;
    ptrMode = &VFOAMode;

    // start the VFOa and it's mode
    updateAllFreq();
}


void loop() {
    #ifdef ROTARY
        // encoder check
        encoderState = encoder.process();
        if (encoderState == DIR_CW)  encoderMoved(+1);
        if (encoderState == DIR_CCW) encoderMoved(-1);
    #endif  // rotary

    // LCD update check in normal mode
    if (update and runMode) {
        #ifndef NOLCD
            // update and reset the flag
            updateLcd();
        #endif  // nolcd
        update = false;
    }

    #ifdef ROTARY
        // check Hardware PTT and make the RX/TX changes
        if (dbPTT.update()) {
            // state changed, if shorted to GND going to TX
            if (dbPTT.fell()) {
                // line asserted (PTT Closed) going to TX
                going2TX();
            } else {
                // line left open, going to RX
                going2RX();
            }

            // update flag
            update = true;
        }

        // debouce for the push
        dbBtnPush.update();
        tbool = dbBtnPush.fell();

        if (runMode) {
            // we are in normal mode

            // step (push button)
            if (tbool) {
                // VFO step change
                changeStep();
                update = true;
            }

            #ifdef SMETER
                // Second line of the LCD, I must show the bargraph only if not rit nor steps
                if ((!ritActive and showStepCounter == 0) and smeterOk)
                    showBarGraph();
            #endif
        } else {
            // setup mode

            // Push button is step in Config mode
            if (tbool) {
                // change the step and show it on the LCD
                changeStep();
                #ifndef NOLCD
                    showStep();
                #endif     // nolcd
            }

        }
    #endif  // rotary

    // timed actions, it ticks every 1/4 second (250 msecs)
    if ((millis() - lastMilis) >= TICK_INTERVAL) {
        // Reset the last reading to keep track
        lastMilis = millis();

        #ifdef SMETER
            // I must sample the input for the bar graph
            smeter();
        #endif

        // time counter for VFO remember after power off
        if (qcounter < SAVE_INTERVAL) {
            qcounter += 1;
        } else {
            // the saveEEPROM has already a mechanism to change only the parts
            // that has changed, to protect the EEPROM of wear out
            saveEEPROM();
            qcounter = 0;
        }

        // step show time show the first time
        if (showStepCounter >= STEP_SHOW_TIME) {
            #ifndef NOLCD
                showStep();
            #endif  // nolcd
        }

        // decrement timer
        if (showStepCounter > 0) {
            showStepCounter -= 1;
            // flag to redraw the bar graph only if zero
            if (showStepCounter == 0) barReDraw = true;
        }
    }

    #ifdef CAT_CONTROL
        // CAT check
        cat.check();
    #endif

    #ifdef ABUT
        // analog buttons check
        abm.check();
    #endif
}

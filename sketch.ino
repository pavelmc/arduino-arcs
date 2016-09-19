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
 *  * WJ6C for the idea and hardware support.
 *  * Many other Cuban hams with critics and opinions
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
 * Important information you need to know about this code
 *  * The Si5351 original lib use the PLLs like this
 *    * CLK0 uses the PLLA
 *    * CLK1 uses the PLLB
 *    * CLK2 uses the PLLB
 *
 *  * We are fine with that specs, as we need the VFO source to be unique
 *   (it has it's own PLL) to minimize the crosstalk between outputs
 *    > CLK0 is used to VFO (the VFO will *always* be above the RF freq)
 *    > CLK1 will be used optionally as a XFO for trx with a second conversion
 *    > CLK2 is used for the BFO
 *
 *  * Please have in mind that this IC has a SQUARE wave output and you need to
 *    apply some kind of low-pass/band-pass filtering to smooth it and get rid
 *    of the nasty harmonics
 ******************************************************************************/

#include <Rotary.h>         // https://github.com/mathertel/RotaryEncoder/
#include <si5351.h>         // https://github.com/etherkit/Si5351Arduino/
#include <Bounce2.h>        // https://github.com/thomasfredericks/Bounce2/
#include <anabuttons.h>     // https://github.com/pavelmc/AnaButtons/
#include <EEPROM.h>         // default
#include <Wire.h>           // default
#include <LiquidCrystal.h>  // default

// the fingerprint to know the EEPROM is initialized, we need to stamp something
// on it, as the 5th birthday anniversary of my daughter was in the date I begin
// to work on this project, so be it: 2016 June 1st
#define EEPROMfingerprint "20160601"

/*******************************************************************************
 *
 *                               !!! WARNING !!!
 *
 * Be aware that the library use the freq on 1/10 hz resolution, so 10 Khz
 * is expressed as 10_000_0 (note the extra zero)
 *
 * This will allow us to climb up to 160 Mhz & we don't need the extra accuracy
 *
 * Check you have in the Si5351 this param defined as this:
 *     #define SI5351_FREQ_MULT                    10ULL
 *
 * Also we use a module with a 27 Mhz xtal, check this also
 *     #define SI5351_XTAL_FREQ                    27000000
 *
* *****************************************************************************/

/*********************** USER BOARD SELECTION **********************************
 * if you have the any of the COLAB shields uncomment the following line.
 * (the sketch is configured by default for my particular hardware)
 ******************************************************************************/
//#define COLAB

/***********************  DEBUG BY SERIAL  *************************************
 * If you like to have a debug info by the serial port just uncomment this and
 * attach a serial terminal to the arduino 57600 @ 8N1.
 * This is a temporal helper feature, in the future it will support a CAT
 * protocol to interact with the radio
 * ****************************************************************************/
//#define SDEBUG

/*************************  FILTER PRE-CONFIGURATIONS **************************
 * As this project aims to easy the user configuration we will pre-stablish some
 * defaults for some of the most common hardware configurations we have in Cuba
 *
 * The idea is that if you find a matching hardware case is just a matter of
 * uncomment the option and comment the others, compile the sketch an upload and
 * you will get ball park values for your configuration.
 *
 * See the Setup_en|Setup_es PDF files in the doc directory, if you use this and
 * use a own hardware variant I can code it to make your life easier, write to
 * pavelmc@gmail.com for details [author]
 *
 * WARNING at least one must be un-commented for the compiler to work
 ******************************************************************************/
// Single conversion Radio using the SSB filter of an FT-80C/FT-747GX
// the filter reads: "Type: XF-8.2M-242-02, CF: 8.2158 Mhz"
#define SSBF_FT747GX

// Single conversion Radio using the SSB filter of a Polosa/Angara
// the filter reads: "ZMFDP-500H-3,1"
//
// WARNING !!!! This filters has a very high loss (measured around -20dB) if
// not tuned in the input and output
//
//#define SSBF_URSS_500H

// Double conversion (28.8MHz/200KHz) radio from RF board of a RFT SEG-15
// the radio has two filters, we used the one that reads:
// "MF 200 - E - 0235 / RTF 02.83"
//
// WARNING !!!! See notes below in the ifdef structure for the RFT SEG-15
//
//#define SSBF_RFT_SEG15

// The eeprom & sketch version; if the eeprom version is lower than the one on
// the sketck we force an update (init) to make a consistent work on upgrades
#define EEP_VER     3
#define FMW_VER     8

// the limits of the VFO, the one the user see, for now just 40m for now
// you can tweak it with the limits of your particular hardware
// this are LCD diplay frequencies.
#define F_MIN      65000000     // 6.500.000
#define F_MAX      75000000     // 7.500.000

// encoder pins
#define ENC_A    3              // Encoder pin A
#define ENC_B    2              // Encoder pin B
#define inPTT   13              // PTT/CW KEY Line with pullup from the outside
#define PTT     12              // PTT actuator, this will put thet radio on TX
#if defined (COLAB)
    // Any of the COLAB shields
    #define btnPush  11             // Encoder Button
#else
    // Pavel's hardware
    #define btnPush  4              // Encoder Button
#endif

// rotary encoder library setup
Rotary encoder = Rotary(ENC_A, ENC_B);

// the debounce instances
#define debounceInterval  10    // in milliseconds
Bounce dbBtnPush = Bounce();

// define the analog pin to handle the buttons
#define KEYS_PIN  2

// analog buttons library declaration (constructor)
AnaButtons ab = AnaButtons(KEYS_PIN);
byte anab = 0;  // this is to handle the buttons output

// lcd pins assuming a 1602 (16x2) at 4 bits
#if defined (COLAB)
    // COLAB shield + Arduino Mini Board
    #define LCD_RS      5
    #define LCD_E       6
    #define LCD_D4      7
    #define LCD_D5      8
    #define LCD_D6      9
    #define LCD_D7      10
#else
    // Pavel's hardware
    #define LCD_RS      8    // 14 < Real pins in a 28PDIP
    #define LCD_E       7    // 13
    #define LCD_D4      6    // 12
    #define LCD_D5      5    // 11
    #define LCD_D6      10   // 16
    #define LCD_D7      9    // 15
#endif

// lcd library setup
LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
// defining the chars

byte bar[8] = {
  B11111,
  B11111,
  B11111,
  B11111,
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

// Si5351 library setup
Si5351 si5351;

// run mode constants
#define MODE_LSB 0
#define MODE_USB 1
#define MODE_CW  2
#define MODE_MAX 2                // the mode count to cycle (-1)
#define MAX_RIT 99900             // RIT 9.99 Khz

// config constants
#define CONFIG_IF       0
#define CONFIG_VFO_A    1
#define CONFIG_MODE_A   2
#define CONFIG_VFO_B    3
#define CONFIG_MODE_B   4
#define CONFIG_LSB      5
#define CONFIG_USB      6
#define CONFIG_CW       7
#define CONFIG_XFO      8
#define CONFIG_PPM      9
// --
#define CONFIG_MAX 9               // the amount of configure options

// sampling interval for the AGC, 33 milli averaged every 10 reads: gives 3
// updates per second and a good visual effect
#define SM_SAMPLING_INTERVAL  33

// hardware pre configured values
#if defined (SSBF_FT747GX)
    // Pre configured values for a Single conversion radio using the FT-747GX
    long lsb =  -13500;
    long usb =   13500;
    long cw =        0;
    long xfo =       0;
    long ifreq =   82148000;
#endif

#if defined (SSBF_URSS_500H)
    // Pre configured values for a Single conversion radio using the Polosa
    // Angara 500H filter
    long lsb =  -17500;
    long usb =   17500;
    long cw =        0;
    long xfo =       0;
    long ifreq =   4980800;
#endif

#if defined (SSBF_RFT_SEG15)
    // Pre configured values for a Double conversion radio using the RFT SEG15
    // RF board using the filter marked as:
    long lsb =  -14350;
    long usb =   14350;
    long cw =        0;
    long ifreq =   282000000;
    /***************************************************************************
     *                     !!!!!!!!!!!    WARNING   !!!!!!!!!!
     *
     *  The Si5351 has serious troubles to keep the accuracy in the XFO & BFO
     *  for the case in where you want to substitute all the frequency
     *  generators inside the SEG-15.
     *
     *  This combination can lead to troubles with the BFO not moving in the
     *  steps you want.
     *
     *  This is because the XFO & BFO share the same VCO inside the Si5351 and
     *  the firmware has to make some compromises to get the two oscillators
     *  running at the same time, in this case some accuracy is sacrificed and
     *  you get as a result the BFO jumping in about 150 hz steps even if you
     *  instruct it as 1 Hz.
     *
     *  The obvious solution is to keep the XFO as a XTAL one inside the radio
     *  and move the IF Frequency to tune the differences of your own XFO Xtal
     *  in our test the XTAL was in the valued showed below.
     *
     *  In this case the firmware will keep track of the XFO but will never
     *  turn it on. It is used only for the calculations
    */
    signed long xfo =       279970000;
#endif

// put the value here if you have the default ppm corrections for you Si5351
// if you set it here it will be stored on the EEPROM on the initial start,
// otherwise set it to zero and you can set it up via the SETUP menu
long si5351_ppm = 224380;    // it has the *10 included

// the variables
long vfoa = 71100000;    // default starting VFO A freq
long vfob = 71755000;    // default starting VFO B freq
long tvfo = 0;           // temporal VFO storage for RIT usage
long txSplitVfo =  0;    // temporal VFO storage for RIT usage when TX
byte step = 3;                    // default steps position index: 1*10E3 = 1000 = 100hz
                                  // step position is calculated to avoid to use
                                  // a big array, see getStep()
boolean update = true;            // lcd update flag in normal mode
byte encoderState = DIR_NONE;     // encoder state
byte fourBytes[4];                // swap array to long to/from eeprom conversion
byte config = 0;                  // holds the configuration item selected
boolean inSetup = false;          // the setup mode, just looking or modifying
boolean mustShowStep = false;     // var to show the step instead the bargraph
#define showStepTimer   8000      // a relative amount of time to show the mode
                                  // aprox 3 secs
word showStepCounter = showStepTimer; // the timer counter
boolean runMode =      true;
boolean activeVFO =    true;
byte VFOAMode =        MODE_LSB;
byte VFOBMode =        MODE_LSB;
boolean ritActive =    false;
boolean tx =           false;
byte pep[15];                      // s-meter readings storage
long lastMilis = 0;       // to track the last sampled time
boolean smeterOk = false;
boolean split   = false;            // this holds th split state

// temp vars
boolean tbool   = false;

// the encoder has moved
void encoderMoved(int dir) {
    // check the run mode
    if (runMode) {
        // update freq
        updateFreq(dir);
        update = true;

    } else {
        // update the values in the setup mode
        updateSetupValues(dir);
    }
}


// update freq procedure
void updateFreq(int dir) {
    long delta;
    if (ritActive) {
        // we fix the steps to 10 Hz in rit mode
        delta = 100 * dir;
    } else {
        // otherwise we use the default step on the environment
        delta = getStep() * dir;
    }

    // move the active vfo
    if (activeVFO) {
        vfoa = moveFreq(vfoa, delta);
    } else {
        vfob = moveFreq(vfob, delta);
    }

    // update the output freq
    setFreqVFO();
}


// frequency move
long moveFreq(long freq, long delta) {
    // freq is used as the output var
    long newFreq = freq + delta;

    // limit check
    if (ritActive) {
        // check we don't exceed the MAX_RIT
        long diff = tvfo;
        diff -= newFreq;

        // update just if allowed
        if (abs(diff) <= MAX_RIT) freq = newFreq;
    } else {
        // do it
        freq = newFreq;

        // limit check
        if(freq > F_MAX) freq = F_MAX;
        if(freq < F_MIN) freq = F_MIN;
    }

    // return the new freq
    return freq;
}


// return the right step size to move
long getStep () {
    // we get the step from the global step var
    long ret = 1;

    // validation just in case
    if (step == 0) step = 1;

    for (byte i=0; i < step; i++) {
        ret *= 10;
    }

    return ret;
}


// Force freq update for all the environment vars
// (Active CFO and it's actual mode)
void updateAllFreq() {
    setFreqVFO();
    setFreqBFO();
    setFreqXFO();

    #if defined (SDEBUG)
    Serial.print(F("LSB: "));
    Serial.println(lsb, DEC);
    Serial.print(F("USB: "));
    Serial.println(usb, DEC);
    Serial.print(F(" CW: "));
    Serial.println(cw, DEC);
    Serial.print(F("RVFO: "));
    Serial.println(getActiveVFOFreq(), DEC);
    Serial.println(F("-----------------------"));
    #endif
}


// update the setup values
void updateSetupValues(int dir) {
    // we are in setup mode, showing or modifying?
    if (!inSetup) {
        // just showing, show the config on the LCD
        updateShowConfig(dir);
    } else {
        // I'm modifying, switch on the config item
        switch (config) {
            case CONFIG_IF:
                // change the VFO to A by default
                activeVFO = true;
                // change the IF value
                ifreq += getStep() * dir;
                // hot swap it
                updateAllFreq();
                break;
            case CONFIG_VFO_A:
                // change the VFO
                activeVFO = true;
                // change VFOa
                vfoa += getStep() * dir;
                // hot swap it
                updateAllFreq();
                break;
            case CONFIG_VFO_B:
                // change the VFO
                activeVFO = false;
                // change VFOb, this is not hot swapped
                vfob += getStep() * dir;
                break;
            case CONFIG_MODE_A:
                // change the mode for the VFOa
                activeVFO = true;
                changeMode();
                // set the default mode in the VFO A
                showModeSetup(VFOAMode);
                break;
            case CONFIG_MODE_B:
                // change the mode for the VFOb
                activeVFO =  false;
                changeMode();
                // set the default mode in the VFO B
                showModeSetup(VFOBMode);
                break;
            case CONFIG_USB:
                // force the VFO A
                activeVFO = true;
                // change the mode to USB
                setActiveVFOMode(MODE_USB);
                // change the USB BFO
                usb += getStep() * dir;
                // hot swap it
                updateAllFreq();
                break;
            case CONFIG_LSB:
                // force the VFO A
                activeVFO = true;
                // change the mode to LSB
                setActiveVFOMode(MODE_LSB);
                // change the LSB BFO
                lsb += getStep() * dir;
                // hot swap it
                updateAllFreq();
                break;
            case CONFIG_CW:
                // force the VFO A
                activeVFO = true;
                // change the mode to CW
                setActiveVFOMode(MODE_CW);
                // change the CW BFO
                cw += getStep() * dir;
                // hot swap it
                updateAllFreq();
                break;
            case CONFIG_PPM:
                // change the Si5351 PPM
                si5351_ppm += getStep() * dir;
                // instruct the lib to use the new ppm value
                si5351.set_correction(si5351_ppm);
                // hot swap it, this time both values
                updateAllFreq();
                break;
            case CONFIG_XFO:
                // change XFO ############## posible overflow
                xfo += getStep() * dir;
                updateAllFreq();
                break;
        }

        // update el LCD
        showModConfig();
    }
}


// print a string of spaces, to save some space
void spaces(byte m) {
    // print m spaces in the LCD
    while (m) {
        lcd.print(" ");
        m--;
    }
}


// show the mode for the passed mode in setup mode
void showModeSetup(byte mode) {
    // now I have to print it out
    lcd.setCursor(0, 1);
    spaces(11);
    showModeLcd(mode);
}


// update the configuration item before selecting it
void updateShowConfig(int dir) {
    // move the config item
    int tconfig = config;
    tconfig += dir;

    if (tconfig > CONFIG_MAX) tconfig = 0;
    if (tconfig < 0) tconfig = CONFIG_MAX;
    config = tconfig;

    // update the LCD
    showConfig();
}


// show the labels of the config
void showConfigLabels() {
    switch (config) {
        case CONFIG_IF:
            lcd.print(F("  IF frequency  "));
            break;
        case CONFIG_VFO_A:
            lcd.print(F("VFO A start freq"));
            break;
        case CONFIG_VFO_B:
            lcd.print(F("VFO B start freq"));
            break;
        case CONFIG_MODE_A:
            lcd.print(F("VFO A start mode"));
            break;
        case CONFIG_MODE_B:
            lcd.print(F("VFO B start mode"));
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
        case CONFIG_XFO:
            lcd.print(F("XFO frequency   "));
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
void showConfigValueSigned(long val) {
    if (config == CONFIG_PPM) {
        lcd.print(F("PPM: "));
    } else {
        lcd.print(F("Val:"));
    }

    // detect the sign
    showSign(val);

    // print it
    formatFreq(abs(val));
}


// Show the value for the setup item
void showConfigValue(long val) {
    lcd.print(F("Val:"));
    formatFreq(val);

    // if on config mode we must show up to hz part
    if (runMode) {
        lcd.print(F("0hz"));
    } else {
        lcd.print(F("hz"));
    }
}


// update the specific setup item
void showModConfig() {
    lcd.setCursor(0, 0);
    showConfigLabels();

    // show the specific values
    lcd.setCursor(0, 1);
    switch (config) {
        case CONFIG_IF:
            // we force the VFO and the actual mode on it
            activeVFO = true;
            showConfigValue(ifreq);
            break;
        case CONFIG_VFO_A:
            showConfigValue(vfoa);
            break;
        case CONFIG_VFO_B:
            showConfigValue(vfob);
            break;
        case CONFIG_MODE_A:
            showModeSetup(VFOAMode);
        case CONFIG_MODE_B:
            showModeSetup(VFOBMode);
            break;
        case CONFIG_USB:
            showConfigValueSigned(usb);
            break;
        case CONFIG_LSB:
            showConfigValueSigned(lsb);
            break;
        case CONFIG_CW:
            showConfigValueSigned(cw);
            break;
        case CONFIG_PPM:
            showConfigValueSigned(si5351_ppm);
            break;
        case CONFIG_XFO:
            showConfigValue(xfo);
            break;
    }
}


// format the freq to easy viewing
void formatFreq(long freq) {
    // for easy viewing we format a freq like 7.110 to 7.110.00
    long t;

    // get the freq in Hz as the lib needs in 1/100 hz resolution
    freq /= 10;

    // Mhz part
    t = freq / 1000000;
    if (t < 10) spaces(1);
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
        lcd.print(F("A"));
    } else {
        lcd.print(F("B"));
    }

    // split?
    if (split) {
        // ok, show the split status as a * sign
        lcd.print(F("*"));
    } else {
        // print a separator.
        spaces(1);
    }

    if (activeVFO) {
        formatFreq(vfoa);
        spaces(1);
        showModeLcd(VFOAMode);
    } else {
        formatFreq(vfob);
        spaces(1);
        showModeLcd(VFOBMode);
    }

    // second line
    lcd.setCursor(0, 1);
    if (tx == true) {
        lcd.print(F("TX "));
    } else {
        lcd.print(F("RX "));
    }

    // here goes the rx/tx bar graph or the other infos as RIT or STEPS

    // if we have a RIT or steps we manage it here and the bar will hold
    if (ritActive) showRit();

    // show the step if it must
    if (mustShowStep) showStep();
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

    // get the active VFO to calculate the deviation
    long vfo = getActiveVFOFreq();

    long diff = vfo;
    diff -= tvfo;

    // scale it down, we don't need hz resolution here
    diff /= 10;

    // we start on line 2, char 3 of the second line
    lcd.setCursor(3, 1);
    lcd.print(F("RIT "));

    // show the difference in Khz on the screen with sign
    // diff can overflow the input of showSign, so we scale it down
    showSign(diff);

    // print the freq now, we have a max of 10 Khz (9.990 Khz)
    diff = abs(diff);

    // Khz part
    word t = diff / 1000;
    lcd.print(t);
    lcd.print(".");
    // hz part
    t = diff % 1000;
    if (t < 100) lcd.print("0");
    if (t < 10) lcd.print("0");
    lcd.print(t);
    // second dot
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
    spaces(8);
}


// get the active VFO freq
long getActiveVFOFreq() {
    // which one is the active?
    if (activeVFO) {
        return vfoa;
    } else {
        return vfob;
    }
}


// get the active mode BFO freq
long getActiveBFOFreq() {
    // obtener el modo activo
    byte mode = getActiveVFOMode();

    /* ***********************
     * Remember we use allways up conversion so...
     *
     * LSB: the VFO is at the right spot as the ifreq
     * USB: we displace the VFO & BFO up in a BW amout (usb)
     * CW: we displace the VFO & BFO up in a BW amout (cw)
     *
     * */

    // return it
    switch (mode) {
        case MODE_LSB:
            return ifreq - xfo + lsb;
            break;
        case MODE_USB:
            return ifreq - xfo + usb;
            break;
        case MODE_CW:
            return ifreq - xfo + cw;
            break;
    }

    return ifreq;
}


// set the calculated freq to the VFO
void setFreqVFO() {
    // get the active VFO freq, calculate the final freq+IF and get it out
    long freq = getActiveVFOFreq();
    byte mode = getActiveVFOMode();

    /* ***********************
     * Remember we allways use up conversion so...
     *
     * LSB & USB: we displace the VFO & BFO up in a BW/2 amout
     * CW: TBD
     *
     * */

    // return it
    switch (mode) {
        case MODE_LSB:
            freq += lsb;
            break;
        case MODE_USB:
            freq += usb;
            break;
        case MODE_CW:
            freq += cw;
            break;
    }

    freq += ifreq;
    si5351.set_freq(freq, 0, SI5351_CLK0);

    #if defined (SDEBUG)
    // DEBUG: Spit the freq sent to the VFO
    Serial.print(F("VFO: "));
    Serial.println(freq, DEC);
    #endif
}


// set the bfo freq
void setFreqBFO() {
    // get the active vfo mode freq and get it out
    long frec = getActiveBFOFreq();
    // deactivate it if zero
    if (frec == 0) {
        // deactivate it
        si5351.output_enable(SI5351_CLK2, 0);

        #if defined (SDEBUG)
        // DEBUG: disabled
        Serial.println(F("XFO: Disabled"));
        #endif
    } else {
        // output it
        si5351.output_enable(SI5351_CLK2, 1);
        si5351.set_freq(frec, 0, SI5351_CLK2);

        #if defined (SDEBUG)
        // DEBUG: Spit the freq sent to the BFO
        Serial.print(F("BFO: "));
        Serial.println(frec, DEC);
        #endif
    }
}


// set the xfo freq
void setFreqXFO() {
    // RFT SEG-15 CASE: the XFO is in PLACE but it is not generated in any case
    #if defined (SSBF_RFT_SEG15)
        // XFO DISABLED AT ALL COST
        si5351.output_enable(SI5351_CLK1, 0);

        #if defined (SDEBUG)
        // DEBUG: disabled
        Serial.println(F("XFO: Disabled (SEG-15)"));
        #endif
    #else
        // just put it out if it's set
        if (xfo == 0) {
            // this is only enabled if we have a freq to send outside
            si5351.output_enable(SI5351_CLK1, 0);

            #if defined (SDEBUG)
            // DEBUG: disabled
            Serial.println(F("XFO: Disabled"));
            #endif
        } else {
            si5351.output_enable(SI5351_CLK1, 1);
            si5351.set_freq(xfo, 0, SI5351_CLK1);
            // WARNING This has a shared PLL with the BFO, maybe we need to reset the BFO?

            #if defined (SDEBUG)
            // DEBUG: Spit the freq sent to the XFO
            Serial.print(F("XFO: "));
            Serial.println(xfo, DEC);
            #endif
        }
    #endif
}


// return the active VFO mode
byte getActiveVFOMode() {
    if (activeVFO) {
        return VFOAMode;
    } else {
        return VFOBMode;
    }
}


// set the active VFO mode
void setActiveVFOMode(byte mode) {
    if (activeVFO) {
        VFOAMode = mode;
    } else {
        VFOBMode = mode;
    }
}


// change the modes in a cycle
void changeMode() {
    byte mode = getActiveVFOMode();

    // calculating the next mode in the queue
    if (mode == MODE_MAX) {
        // overflow: reset
        mode = 0;
    } else {
        // normal increment
        mode += 1;
    }

    setActiveVFOMode(mode);

    // Apply the changes
    updateAllFreq();
}


// change the steps
void changeStep() {
    // calculating the next step
    if (step < 7) {
        // simple increment
        step += 1;
    } else {
        // default start mode is 2 (10Hz)
        step = 2;
        // in setup mode and just specific modes it's allowed to go to 1 hz
        boolean allowedModes = false;
        allowedModes = allowedModes or (config == CONFIG_LSB);
        allowedModes = allowedModes or (config == CONFIG_USB);
        allowedModes = allowedModes or (config == CONFIG_PPM);
        allowedModes = allowedModes or (config == CONFIG_XFO);
        if (!runMode and allowedModes) step = 1;
    }

    // if in normal mode reset the counter to show the change in the LCD
    if (runMode) showStepCounter = showStepTimer;     // aprox half second

    // tell the LCD that it must show the change
    mustShowStep = true;
}


// RIT toggle
void toggleRit() {
    if (!ritActive) {
        // going to activate it: store the active VFO
        tvfo = getActiveVFOFreq();
        ritActive = true;
    } else {
        // going to deactivate: reset the stored VFO
        if (activeVFO) {
            vfoa = tvfo;
        } else {
            vfob = tvfo;
        }
        // deactivate it
        ritActive = false;
    }
}


// check if the EEPROM is initialized
boolean checkInitEEPROM() {
    // read the eprom start to determine is the fingerprint is in there
    char fp;
    for (byte i=0; i<8; i++) {
        fp = EEPROM.read(i);
        if (fp != EEPROMfingerprint[i]) return false;
    }

    //ok, the eeprom is initialized, but it's the same version of the sketch?
    byte eepVer = EEPROM.read(8);
    if (eepVer != EEP_VER) return false;

    // if you get here you are safe
    return true;
}


// split in bytes, take a long and load it on the fourBytes (MSBF)
void splitInBytes(long freq) {
    for (byte i=0; i<4; i++) {
        fourBytes[i] = char((freq >> (3-i)*8) & 255);
    }
}


// restore a long from the four bytes in fourBytes (MSBF)
long restoreFromBytes() {
    long freq;
    long temp;

    // the MSB need to be set by hand as we can't cast a long
    // or can we?
    freq = fourBytes[0];
    freq = freq << 24;

    // restore the lasting bytes
    for (byte i=1; i<4; i++) {
        temp = long(fourBytes[i]);
        freq += temp << ((3-i)*8);
    }

    // get it out
    return freq;
}


// read an long from the EEPROM, fourBytes as swap var
long EEPROMReadLong(word pos) {
    for (byte i=0; i<4; i++) {
        fourBytes[i] = EEPROM.read(pos+i);
    }
    return restoreFromBytes();
}


// write an long to the EEPROM, fourBytes as swap var
void EEPROMWriteLong(long val, word pos) {
    splitInBytes(val);
    for (byte i=0; i<4; i++) {
        EEPROM.write(pos+i, fourBytes[i]);
    }
}


// initialize the EEPROM mem, also used to store the values in the setup mode
void initEeprom() {
    /***************************************************************************
     * EEPROM structure
     * Example: @00 (8): comment
     *  @##: start of the content of the eeprom var (always decimal)
     *  (#): how many bytes are stored there
     *
     * =========================================================================
     * @00 (8): EEPROMfingerprint, see the corresponding #define at the code start
     * @08 (1): EEP_VER
     * @09 (4): IF freq in 4 bytes
     * @13 (4): VFO A in 4 bytes
     * @17 (4): VFO B in 4 bytes
     * @21 (4): XFO freq in 4 bytes
     * @25 (4): BFO feq for the LSB mode
     * @29 (4): BFO feq for the USB mode
     * @33 (4): CW the offset for the CW mode in TX
     * @37 (1): Encoded Byte in 4 + 4 bytes representing the USB and LSB mode
     *          of the A/B VFO
     * @38 (4): PPM correction for the Si5351
     *
     * =========================================================================
     *
    /**************************************************************************/

    // temp var
    long temp = 0;

    // write the fingerprint
    for (byte i=0; i<8; i++) {
        EEPROM.write(i, EEPROMfingerprint[i]);
    }

    // EEPROM version
    EEPROM.write(8, EEP_VER);

    // IF
    EEPROMWriteLong(ifreq, 9);

    // VFO a freq
    EEPROMWriteLong(vfoa, 13);

    // VFO B freq
    EEPROMWriteLong(vfob, 17);

    // XFO freq
    EEPROMWriteLong(xfo, 21);

    // BFO for the LSB mode
    temp = lsb + 2147483647;
    EEPROMWriteLong(temp, 25);

    // BFO for the USB mode
    temp = usb + 2147483647;
    EEPROMWriteLong(temp, 29);

    // BFO for the CW  mode
    temp = cw + 2147483647;
    EEPROMWriteLong(temp, 33);

    // Encoded byte with the default modes for VFO A/B
    byte toStore = byte(VFOAMode << 4) + VFOBMode;
    EEPROM.write(37, toStore);

    // Si5351 PPM correction value
    // this is a signed value, so we need to scale it to a unsigned
    temp = si5351_ppm + 2147483647;
    EEPROMWriteLong(temp, 38);

}


// load the eprom contents
void loadEEPROMConfig() {
    /***************************************************************************
     * EEPROM structure
     * Example: @00 (8): comment
     *  @##: start of the content of the eeprom var (always decimal)
     *  (#): how many bytes are stored there
     *
     * =========================================================================
     * @00 (8): EEPROMfingerprint, see the corresponding #define at the code start
     * @08 (1): EEP_VER
     * @09 (4): IF freq in 4 bytes
     * @13 (4): VFO A in 4 bytes
     * @17 (4): VFO B in 4 bytes
     * @21 (4): XFO freq in 4 bytes
     * @25 (4): BFO feq for the LSB mode
     * @29 (4): BFO feq for the USB mode
     * @33 (4): CW the offset for the CW mode in TX
     * @37 (1): Encoded Byte in 4 + 4 bytes representing the USB and LSB mode
     *          of the A/B VFO
     * @38 (4): PPM correction for the Si5351
     *
     * =========================================================================
     *
    /**************************************************************************/
    //temp var
    long temp = 0;

    // get the IF value
    ifreq = EEPROMReadLong(9);

    // get VFOa freq.
    vfoa = EEPROMReadLong(13);

    // get VFOb freq.
    vfob = EEPROMReadLong(17);

    // get XFO freq.
    xfo = EEPROMReadLong(21);

    // get BFO lsb freq.
    temp = EEPROMReadLong(25);
    lsb = temp -2147483647;

    // BFO lsb freq.
    temp = EEPROMReadLong(29);
    usb = temp -2147483647;

    // BFO CW freq.
    temp = EEPROMReadLong(33);
    cw = temp - 2147483647;

    // get the deafult modes for each VFO
    char salvar = EEPROM.read(37);
    VFOAMode = salvar >> 4;
    VFOBMode = salvar & 15;

    // Si5351 PPM correction value
    // this is a signed value, so we need to scale it to a unsigned
    temp = EEPROMReadLong(38);
    si5351_ppm = temp - 2147483647;
    // now set it up
    si5351.set_correction(si5351_ppm);
}


// show the bar graph for the RX or TX modes
void showBarGraph() {
    // we are working on a 2x16 and we have 13 bars (0-12)
    byte ave = 0, i;
    static byte barMax = 0;
    static boolean lastShowStep;

    // find the average
    for (i=0; i<15; i++) {
        ave += pep[i];
    }
    ave /= 15;

    // printing only the needed part of the bar, if growing or shrinking
    // if the same no action is required, remember we have to minimize the
    // writes to the LCD to minimize QRM

    // but there is a special case: just after the show step expires
    // that it must start from zero
    if (!mustShowStep and lastShowStep) barMax = 0;

    // growing bar: print the difference
    if (ave > barMax) {
        // LCD position & print the bars
        lcd.setCursor(3 + barMax, 1);

        // write it
        for (i = barMax; i <= ave; i++) {
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
    }

    // shrinking bar: erase the old ones print spaces to erase just the diff
    if (barMax > ave) {
        // the bar shares the space of the step label, so if the ave drops
        // below 4 and the barMax is over 4 we have to erase from 4 to the
        // actual value of ave to erase any trace of the step label
        if (ave < 4 and barMax > 4) {
            // we have to erase
            i = 4;
        } else {
            i = barMax;
        }

        // erase it
        while (i > ave) {
            lcd.setCursor(3 + i, 1);
            lcd.print(" ");
            i--;
        }
    }

    // put the var for the next iteration
    barMax = ave;

    // load the last step for the next iteration
    lastShowStep = mustShowStep;
}


// smeter reading, this take a sample of the smeter/txpower each time; an will
// rise a flag when they have rotated the array of measurements 2/3 times to
// have a moving average
void smeter() {
    // contador para el ciclo de lecturas en el array
    static byte smeterCount = 0;

    // it has rotated already?
    if (smeterCount < 9) {
        // take a measure and rotate the array

        // we are sensing a value that must move in the 0-1.1v so internal reference
        analogReference(INTERNAL);
        // read the value and map it for 13 chars (0-12) in the LCD bar
        word val;
        if (tx) {
            // we are on TX, sensing via A1
            val = analogRead(A1);
        } else {
            // we are in RX, sensing via A0
            val = analogRead(A0);
        }

        // watchout !!! map can out peaks, so smooth
        if (val > 1023) val = 1023;

        // scale it to 13 blocks (0-12)
        val = map(val, 0, 1023, 0, 12);

        // push it in the array
        for (byte i = 0; i < 14; i++) {
            pep[i] = pep[i+1];
        }
        pep[14] = val;

        // reset the reference for the buttons handling
        analogReference(DEFAULT);

        // increment counter
        smeterCount += 1;
    } else {
        // rise the flag about the need to show the bar graph and reset the count
        smeterOk = true;
        smeterCount = 0;
    }
}


// set a freq to the active VFO
void setActiveVFO(long f) {
    if (activeVFO) {
        vfoa = f;
    } else {
        vfob = f;
    }
}


// split check
void splitCheck() {
    if (split) {
        // revert back the VFO
        activeVFO = !activeVFO;
        updateAllFreq();
    }
}


// main setup procedure: get all ready to rock
void setup() {
    #if defined (SDEBUG)
        // start serial port at 57600 bps:
        Serial.begin(57600);
        // Welcome to the Serial mode
        Serial.println("");
        Serial.println(F("Arduino-ARCS Ready."));
    #endif

    // LCD init, create the custom chars first
    lcd.createChar(0, bar);
    lcd.createChar(1, s1);
    lcd.createChar(2, s3);
    lcd.createChar(3, s5);
    lcd.createChar(4, s7);
    lcd.createChar(5, s9);
    // now load the library
    lcd.begin(16, 2);
    lcd.clear();

    // buttons debounce encoder push
    pinMode(btnPush, INPUT_PULLUP);
    pinMode(ENC_A, INPUT_PULLUP);
    pinMode(ENC_B, INPUT_PULLUP);
    dbBtnPush.attach(btnPush);
    dbBtnPush.interval(debounceInterval);

    // pin mode of the PTT
    pinMode(inPTT, INPUT_PULLUP);
    pinMode(PTT, OUTPUT);

    // I2C init
    Wire.begin();

    // disable the outputs from the begining
    si5351.output_enable(SI5351_CLK0, 0);
    si5351.output_enable(SI5351_CLK1, 0);
    si5351.output_enable(SI5351_CLK2, 0);

    // Xtal capacitive load
    #if defined (COLAB)
        // COLAB boards have no capacitor in place, so max it.
        si5351.init(SI5351_CRYSTAL_LOAD_10PF, 0);
    #else
        // my board have 2x22pf caps in place
        si5351.init(SI5351_CRYSTAL_LOAD_0PF, 0);
    #endif

    // setup the PLL usage
    si5351.set_ms_source(SI5351_CLK0, SI5351_PLLA);
    si5351.set_ms_source(SI5351_CLK1, SI5351_PLLB);
    si5351.set_ms_source(SI5351_CLK2, SI5351_PLLB);

    // use low power on the Si5351
    si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA);
    si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_2MA);
    si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_2MA);


    // check the EEPROM to know if I need to initialize it
    boolean eepromOk = checkInitEEPROM();
    if (!eepromOk) {
        #if defined (SDEBUG)
            // serial advice
            Serial.println(F("Init EEPROM"));
        #endif

        // LCD
        lcd.setCursor(0, 0);
        lcd.print(F("Init EEPROM...  "));
        lcd.setCursor(0, 1);
        lcd.print(F("Please wait...  "));
        initEeprom();
        delay(1000);        // wait for 1 second
        lcd.clear();
    } else {
        // just if it's already ok
        loadEEPROMConfig();
        #if defined (SDEBUG)
            Serial.println(F("EEPROM data loaded."));
        #endif
    }

    // Welcome screen
    lcd.clear();
    //lcd.setCursor(0, 0);
    lcd.print(F("  Aduino Arcs  "));
    lcd.setCursor(0, 1);
    lcd.print(F("Fv: "));
    lcd.print(FMW_VER);
    lcd.print(F("  Mfv: "));
    lcd.print(EEP_VER);
    delay(2000);        // wait for 1 second
    lcd.setCursor(0, 0);
    lcd.print(F(" by Pavel CO7WT "));
    delay(2000);        // wait for 1 second
    lcd.clear();

    // Check for setup mode
    if (digitalRead(btnPush) == LOW) {
        // we are in the setup mode
        lcd.setCursor(0, 0);
        lcd.print(F(" You are in the "));
        lcd.setCursor(0, 1);
        lcd.print(F("   SETUP MODE   "));
        initEeprom();
        delay(2000);        // wait for 1 second
        lcd.clear();

        // rise the flag of setup mode for every body to see it.
        runMode = false;

        // show setup mode
        showConfig();

        #if defined (SDEBUG)
            Serial.println(F("You are in SETUP mode now"));
        #endif
    }

    // setting up VFO A as principal.
    activeVFO = true;

    // Enable the Si5351 outputs
    si5351.output_enable(SI5351_CLK0, 1);
    si5351.output_enable(SI5351_CLK2, 1);

    // start the VFOa and it's mode
    updateAllFreq();
}


/******************************************************************************
 * In setup mode the buttons change it's behaviour
 * - Encoder push button: remains as step selection
 * - VFO A/B: it's "OK" or "Enter"
 * - Mode: it's "Cancel & back to main menu"
 * - RIT:
 *   - On the USB/LSB/CW freq. setup menus, this set the selected value to zero
 *   - On the PPM adjust it reset it to zero
 ******************************************************************************/


// let's get the party started
void loop() {
    // encoder check
    encoderState = encoder.process();
    if (encoderState == DIR_CW) encoderMoved(+1);
    if (encoderState == DIR_CCW) encoderMoved(-1);

    // LCD update check in normal mode
    if (update and runMode) {
        // update and reset the flag
        updateLcd();
        update = false;
    }

    // check PTT and make the RX/TX changes
    tbool = digitalRead(inPTT);
    if (tbool and tx) {
        // PTT released, going to RX
        tx = false;
        digitalWrite(PTT, tx);

        // make changes if tx goes active when RIT is active
        if (ritActive) {
            // get the TX vfo and store it as the reference for the RIT
            tvfo = getActiveVFOFreq();
            // restore the rit VFO to the actual VFO
            setActiveVFO(txSplitVfo);
        }

        // make the split changes if needed
        splitCheck();

        // rise the update flag
        update = true;
    }

    if (!tbool and !tx) {
        // PTT asserted, going into TX
        tx = true;
        digitalWrite(PTT, tx);

        // make changes if tx goes active when RIT is active
        if (ritActive) {
            // save the actual rit VFO
            txSplitVfo = getActiveVFOFreq();
            // set the TX freq to the active VFO
            setActiveVFO(tvfo);
        }

        // make the split changes if needed
        splitCheck();

        // rise the update flag
        update = true;
    }

    // analog buttons
    anab = ab.getStatus();

    // debouce for the push
    dbBtnPush.update();
    tbool = dbBtnPush.fell();

    if (runMode) {
        // we are in normal mode

        // VFO A/B
        if (anab == ABUTTON1_PRESS) {
            // we force to deactivate the RIT on VFO change, as it will confuse
            // the users and have a non logical use, only if activated and
            // BEFORE we change the active VFO
            if (ritActive) toggleRit();

            // now we change the VFO.
            activeVFO = !activeVFO;

            // update VFO/BFO and instruct to update the LCD
            updateAllFreq();

            // set the LCD update flag
            update = true;
        }

        // mode change
        if (anab == ABUTTON2_PRESS) {
            changeMode();
            update = true;
        }

        // RIT
        if (anab == ABUTTON3_PRESS) {
            toggleRit();
            update = true;
        }

        // SPLIT
        if (anab == ABUTTON4_PRESS) {
            split = !split;
            update = true;
        }

        // step (push button)
        if (tbool) {
            // VFO step change
            changeStep();
            update = true;
        }

        // Second line of the LCD, I must show the bargraph only if not rit nor steps
        if ((!ritActive and !mustShowStep) and smeterOk) {
            // just show the bar graph about 3 times per second
            // it makes RQM on the receiver if left continously updating
            showBarGraph();
        }

        // decrement step counter
        if (mustShowStep) {
            // decrement the counter
            showStepCounter -= 1;

            // compare to show it just once, as showing it continuosly generate
            //RQM from the arduino
            if (showStepCounter == (showStepTimer - 1)) showStep();

            // detect the count end and restore to normal
            if (showStepCounter == 0) mustShowStep = false;
        }

    } else {
        // setup mode

        // Push button is step in Config mode
        if (tbool) {
            // change the step and show it on the LCD
            changeStep();
            showStep();
        }

        // VFO A/B: OK or Enter in SETUP
        if (anab == ABUTTON1_PRESS) {
            if (!inSetup) {
                // I'm going to setup a element
                inSetup = true;

                // change the mode to follow VFO A
                if (config == CONFIG_USB) VFOAMode = MODE_USB;

                if (config == CONFIG_LSB) VFOAMode = MODE_LSB;

                // config update on the LCD
                showModConfig();

            } else {
                // get out of the setup change
                inSetup = false;

                // save to the eeprom
                initEeprom();

                // lcd delay to show it properly (user feedback)
                lcd.setCursor(0, 0);
                lcd.print(F("##   SAVED    ##"));
                delay(250);

                // show setup
                showConfig();

                // reset the minimum step if set (1hz > 10 hz)
                if (step == 1) step += 1;
            }
        }

        // MODE: Cancel the config and get into the main SETUP mode
        if (inSetup and (anab == ABUTTON2_PRESS)) {
            // get out of here
            inSetup = false;

            // user feedback
            lcd.setCursor(0, 0);
            lcd.print(F(" #  Canceled  # "));
            delay(250);

            // show it
            showConfig();
        }

        // RIT: Reset to some defaults
        if (inSetup and (anab == ABUTTON3_PRESS)) {
            // where we are ?
            switch (config) {
                case CONFIG_USB:
                    // reset, usb
                    usb = 0;
                    break;
                case CONFIG_LSB:
                    // reset, lsb
                    lsb = 0;
                    break;
                case CONFIG_CW:
                    // reset, cw
                    cw = 0;
                    break;
                case CONFIG_PPM:
                    // reset, ppm
                    si5351_ppm = 0;
                    si5351.set_correction(0);
                    break;
            }

            // update the freqs for
            updateAllFreq();
            showModConfig();
        }
    }

    // sample and process the S-meter in RX & TX
    if ((millis() - lastMilis) >= SM_SAMPLING_INTERVAL) {
        // I must sample the input
        smeter();
        // and reset the lastreading to keep track
        lastMilis = millis();
    }
}




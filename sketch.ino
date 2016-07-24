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
 *  * WJ6C for the idea and support.
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
 *    > CLK1 will be used optionally as a XFO for transceivers with a second conversion
 *    > CLK2 is used for the BFO (this is changed corresponding to the SSB we choose)
 *
 *  * Please have in mind that this IC has a SQUARE wave output and you need to
 *    apply some kind of low-pass filtering to smooth it and get rid of the
 *    nasty harmonics
 ******************************************************************************/

#include <Rotary.h>         // https://github.com/mathertel/RotaryEncoder/
#include <si5351.h>         // https://github.com/thomasfredericks/Bounce2/
#include <Bounce2.h>        // https://github.com/etherkit/Si5351Arduino/
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
 * is expressed as 10 000 0 (note the extra zero)
 *
 * This will allow us to climb up to 160 Mhz & we don't need the extra accuracy
 *
 * Check you have in the Si5351 this param defined as this:
 *     #define SI5351_FREQ_MULT                    10ULL
 *
 * Also we use a module with a 27 Mhz xtal, check this also
 *     #define SI5351_XTAL_FREQ                    27000000
 *
* *******************************************************************************/

// the limits of the VFO, the one the user see, for now just 40m for now
#define F_MIN        5000000    // 6.900.000
#define F_MAX     1600000000    // 7.500.000

// encoder pins
#define ENC_A    3      // Encoder pin A
#define ENC_B    2      // Encoder pin B
#define btnPush  4     // Encoder Button
#define debounceInterval  10        // in milliseconds

// the debounce instances
Bounce dbBtnPush = Bounce();

// define the analog pin to handle the buttons
#define KEYS_PIN  2

// analog buttons library declaration (constructor)
AnaButtons ab = AnaButtons(KEYS_PIN);
byte anab = 0;  // thi is to handle the buttons output

// lcd pins assuming a 1602 (16x2) at 4 bits
#define LCD_RS      8    // 14
#define LCD_E       7    // 13
#define LCD_D4      6    // 12
#define LCD_D5      5    // 11
#define LCD_D6      10   // 16
#define LCD_D7      9    // 15

// lcd library setup
LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// Si5351 library setup
Si5351 si5351;

// rotary encoder library setup
Rotary encoder = Rotary(ENC_A, ENC_B);

// the variables
// mental note: the used on ISR routines has to be declared as volatiles
signed long lsb =       15000;     // BFO for the lsb (offset from the FI)
signed long usb =      -15000;     // BFO for the usb (offset from the FI)
signed long cw =        -6000;     // BFO for the cw (offset from the FI)
unsigned long xfo =         0;     // second conversion XFO, zero to disable it
unsigned long vfoa = 71100000;     // default starting VFO A freq
unsigned long vfob = 71250000;     // default starting VFO A freq
unsigned long tvfo =         0;    // temporal VFO storage for RIT usage
unsigned long steps[] = {10,  100,  1000,  10000, 100000, 1000000, 10000000};
      // defined steps : 1hz, 10hz, 100hz, 1Khz,  10Khz,  100Khz,  1Mhz
      // for practical and logical reasons we restrict the 1hz step to the
      // SETUP procedures, as 10hz is fine for everyday work.
byte step = 2;                     // default steps position index: 100hz
unsigned long ifreq =   250000000;  // intermediate freq
boolean update =    true;          // lcd update flag in normal mode
volatile byte encoderState = DIR_NONE;   // encoder state, this is volatile
byte fourBytes[4];                  // swap array to long to/from eeprom conversion
byte config = 0;                    // holds the configuration item selected
boolean setup_in = false;           // the setup mode, just looking or modifying
#define CONFIG_MAX 7                // the amount of configure options
boolean showMode = true;            // show mode or step in the normal mode
#define showStepTimer    15000      // a relative amount of time to show the mode
word showStepCounter =  showStepTimer; // the timer counter
#define STEP_MAX  6                 // count of the max steps to show

// run mode constants
#define NORMAL_MODE true
#define VFO_A_ACTIVE true
#define MODE_LSB 0
#define MODE_USB 1
#define MODE_CW  2
#define MODE_MAX 2              // the highest mode to cycle
#define MAX_RIT 99900

// config constants
#define CONFIG_IF       0
#define CONFIG_VFO_A    1
#define CONFIG_VFO_B    2
#define CONFIG_LSB      3
#define CONFIG_USB      4
#define CONFIG_MODE_A   5
#define CONFIG_MODE_B   6
#define CONFIG_PPM      7

// run variables
boolean run_mode =      NORMAL_MODE;
boolean active_vfo =    VFO_A_ACTIVE;
byte vfoa_mode =        MODE_LSB;
byte vfob_mode =        MODE_USB;
boolean rit_active =    false;

// put the value here if you have the default ppm corrections for you Si535
// if you set it here it will be stored on the EEPROM on the initial start,
// otherwise you can set it up via the SETUP menu
signed long si5351_ppm = 299900;    // it has the *10 included (29.990)


// interrupt routine
void IR() {
    // put the encoder state output in the environment
    encoderState = encoder.process();
}


// frequency move
unsigned long moveFreq(unsigned long freq, signed long delta) {
    // freq is used as the output var
    unsigned long newFreq = freq + delta;

    // limit check
    if (rit_active) {
        // check we don't exceed the MAX_RIT
        signed long diff = tvfo;
        diff -= newFreq;

        // update just if allowed
        if (abs(diff) <= MAX_RIT)
            freq = newFreq;
    } else {
        // do it
        freq = newFreq;

        // limit check
        if(freq > F_MAX)
            freq = F_MAX;
        if(freq < F_MIN)
            freq = F_MIN;
    }

    // return the new freq
    return freq;
}


// update freq procedure
void updateFreq(short dir) {
    signed long delta = dir;
    if (rit_active) {
        // we fix the steps to 100 Hz in rit mode
        delta = steps[2] * dir;
    } else {
        delta *= steps[step];
    }

    // move the active vfo
    if (active_vfo == VFO_A_ACTIVE) {
        vfoa = moveFreq(vfoa, delta);
    } else {
        vfob = moveFreq(vfob, delta);
    }

    // update the output freq
    setFreqToVFO();
}


// the encoder has moved
void encoderMoved(short dir) {
    // check the run mode
    if (run_mode == NORMAL_MODE) {
        // update freq
        updateFreq(dir);
        update = true;

    } else {
        // update the values in the setup mode
        updateSetupValues(dir);
    }
}


// update the setup values
void updateSetupValues(short dir) {
    // we are in setup mode, showing or modifying?
    if (!setup_in) {
        // just showing, show the config on the lcd
        updateShowConfig(dir);
    } else {
        // I'm modifying, switch on the config item
        switch (config) {
            case CONFIG_IF:
                // change the IF value
                ifreq += steps[step] * dir;
                // hot swap it
                setFreqToVFO();
                break;
            case CONFIG_VFO_A:
                // change VFOa
                vfoa += steps[step] * dir;
                // hot swap it
                setFreqToVFO();
                break;
            case CONFIG_VFO_B:
                // change VFOb, this is not hot swapped
                vfob += steps[step] * dir;
                break;
            case CONFIG_MODE_A:
                // change the mode for the VFOa
                active_vfo = VFO_A_ACTIVE;
                changeMode();
                // set the default mode in the VFO A
                showModeSetup(vfoa_mode);
                break;
            case CONFIG_MODE_B:
                // change the mode for the VFOb
                active_vfo = !VFO_A_ACTIVE;
                changeMode();
                // set the default mode in the VFO B
                showModeSetup(vfob_mode);
                break;
            case CONFIG_USB:
                // change the USB BFO
                usb += steps[step] * dir;
                // hot swap it
                setFreqBFO();
            case CONFIG_LSB:
                // change the LSB BFO
                lsb += steps[step] * dir;
                // hot swap it
                setFreqBFO();
                break;
            case CONFIG_PPM:
                // change the Si5351 PPM
                si5351_ppm += steps[step] * dir;
                // instruct the lib to use the new ppm value
                si5351.set_correction(si5351_ppm);
                // hot swap it, this time both values
                setFreqToVFO();
                setFreqBFO();
                break;
        }

        // update el LCD
        showModConfig();
    }
}


// show the mode for the passed mode in setup mode
void showModeSetup(byte mode) {
    //reset the show mode flag
    showMode = true;

    // now I have to print it out
    lcd.setCursor(0, 1);
    lcd.print(F("           "));
    showModeLcd(mode);
}


// update the configuration item before selecting it
void updateShowConfig(short dir) {
    // move the config item
    short tconfig = config;
    tconfig += dir;

    if (tconfig > CONFIG_MAX)
        tconfig = 0;

    if (tconfig < 0)
        tconfig = CONFIG_MAX;

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


// show the ppm as a signed long
void showConfigValueSigned(signed long val) {
    if (config == CONFIG_PPM) {
        lcd.print(F("PPM: "));
    } else {
        lcd.print(F("Val:"));
    }

    // detect the sign
    if (val > 0)
        lcd.print("+");
    if (val < 0)
        lcd.print("-");
    if (val == 0)
        lcd.print(" ");

    formatFreq(abs(val));
}


// Show the value for the setup item
void showConfigValue(unsigned long val) {
    lcd.print(F("Val:"));
    formatFreq(val);

    // if on config mode we must show up to hz part
    if (run_mode == NORMAL_MODE) {
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
            showConfigValue(ifreq);
            break;
        case CONFIG_VFO_A:
            showConfigValue(vfoa);
            break;
        case CONFIG_VFO_B:
            showConfigValue(vfob);
            break;
        case CONFIG_MODE_A:
            showModeSetup(vfoa_mode);
        case CONFIG_MODE_B:
            showModeSetup(vfob_mode);
            break;
        case CONFIG_USB:
            showConfigValueSigned(usb);
            break;
        case CONFIG_LSB:
            showConfigValueSigned(lsb);
            break;
        case CONFIG_PPM:
            showConfigValueSigned(si5351_ppm);
            break;
    }
}


// format the freq to easy viewing
void formatFreq(unsigned long freq) {
    // for easy viewing we format a freq like 7.110 Khz to 7.110.00
    unsigned long t;

    // get the freq in Hz as the lib needs in 1/100 hz resolution
    freq /= 10;

    // Mhz part
    t = freq / 1000000;
    if (t < 10)
        lcd.print(" ");
    if (t == 0) {
        lcd.print("  ");
    } else {
        lcd.print(t);
        // first dot
        lcd.print(".");
    }
    // Khz part
    t = (freq % 1000000);
    t /= 1000;
    if (t < 100)
        lcd.print("0");
    if (t < 10)
        lcd.print("0");
    lcd.print(t);
    // second dot
    lcd.print(".");
    // hz part
    t = (freq % 1000);
    if (t < 100)
        lcd.print("0");
    // check if in config and show up to 1hz resolution
    if (run_mode != NORMAL_MODE) {
        if (t < 10)
            lcd.print("0");
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
     *  |>14.280.25 lsb  |
     *  |  7.110.00 lsb  |
     *  ------------------
     ******************************************************/

    // first line
    lcd.setCursor(0, 0);
    // active a?
    if (active_vfo == VFO_A_ACTIVE) {
        lcd.print(">");
    } else {
        lcd.print(" ");
    }

    // show OTHER vfo or RIT
    if ((rit_active) & (active_vfo == !VFO_A_ACTIVE)) {
        // show RIT of the other VFO
        showRit(vfob);
    } else {
        // show normal VFO mode
        formatFreq(vfoa);
        lcd.print(" ");
        showModeLcd(vfoa_mode);
    }

    // second line
    lcd.setCursor(0, 1);
    // active b?
    if (active_vfo == !VFO_A_ACTIVE) {
        lcd.print(">");
    } else {
        lcd.print(" ");
    }

    // show OTHER vfo or RIT
    if ((rit_active) & (active_vfo == VFO_A_ACTIVE)) {
        // show RIT of the other VFO
        showRit(vfoa);
    } else {
        // show normal VFO mode
        formatFreq(vfob);
        lcd.print(" ");
        showModeLcd(vfob_mode);
    }
}


// show rit in LCD
void showRit(unsigned long vfo) {
    /***************************************************************************
     * RIT show something like this on the line of the non active VFO
     *
     *   |0123456789abcdef|
     *   |----------------|
     *   | RIT -9.99 Khz  |
     *   |----------------|
     *
     *             WARNING !!!!!!!!!!!!!!!!!!!!1
     *  If the user change the VFO we need to *RESET* disable the RIT before.
     *
     **************************************************************************/

    // we already have the fist blank line in place to we have only 15 chars
    lcd.print("RIT ");

    signed long diff = vfo;
    diff -= tvfo;

    // show the difference in Khz on the screen with sign
    if (diff > 0) {
        lcd.print("+");
    }

    if (diff < 0) {
        lcd.print("-");
    }

    if (diff == 0) {
        lcd.print(" ");
    }

    // print the freq now, we have a max of 10 Khz (9.990 Khz)
    // get the freq in Hz as the lib needs in 1/100 hz resolution
    diff = abs(diff) / 10;

    // Khz part
    word t = diff / 1000;
    lcd.print(t);
    lcd.print(".");
    // hz part
    t = diff % 1000;
    if (t < 100)
        lcd.print("0");
    if (t < 10)
        lcd.print("0");
    lcd.print(t);
    // second dot
    lcd.print(" khz  ");
}


// show the mode on the LCD, also the VFO step momentary if instructed
void showModeLcd(byte mode) {
    //  mode or step?
    if (showMode) {
        // print it
        switch (mode) {
            case MODE_USB:
              lcd.print(F("USB  "));
              break;
            case MODE_LSB:
              lcd.print(F("LSB  "));
              break;
            case MODE_CW:
              lcd.print(F("CW   "));
              break;
        }
    } else {
        // show vfo step
        showStep();
    }
}


// show the vfo step
void showStep() {
    // in nomal or setup mode?
    if (run_mode == NORMAL_MODE) {
        // for on which vfo?
        if (active_vfo == VFO_A_ACTIVE) {
            lcd.setCursor(11, 0);
        } else {
            lcd.setCursor(11, 1);
        }
    } else {
        // in setup mode is just in the begining of the second line
        lcd.setCursor(0, 1);
    }

    // show it
    switch (step) {
        case 0:
            lcd.print(F("  1hz"));
            break;
        case 1:
            lcd.print(F(" 10hz"));
            break;
        case 2:
            lcd.print(F("100hz"));
            break;
        case 3:
            lcd.print(F(" 1Khz"));
            break;
        case 4:
            lcd.print(F("  10k"));
            break;
        case 5:
            lcd.print(F(" 100k"));
            break;
        case 6:
            lcd.print(F(" 1Mhz"));
            break;
    }
}


// get the active VFO freq
unsigned long getActiveVFOFreq() {
    // which one is the active?
    if (active_vfo == VFO_A_ACTIVE) {
        return vfoa;
    } else {
        return vfob;
    }
}


// get the active mode BFO freq
unsigned long getActiveBFOFreq() {
    // obtener el modo activo
    byte mode = getActiveVFOMode();

    // return it
     switch (mode) {
        case MODE_USB:
            return ifreq + lsb;
            break;
        case MODE_LSB:
            return ifreq + usb;
            break;
        case MODE_CW:
            return ifreq + cw;
            break;
    }

    // it must return something if something goes wrong
    return 0;
}


// set the calculated freq to the VFO
void setFreqToVFO() {
    // get the active VFO freq, calculate the final freq+IF and get it out
    unsigned long frec = getActiveVFOFreq();
    frec += ifreq;
    si5351.set_freq(frec, 0, SI5351_CLK0);
}


// set the bfo freq
void setFreqBFO() {
    // get the active vfo mode freq and get it out
    unsigned long frec = getActiveBFOFreq();
    si5351.set_freq(frec, 0, SI5351_CLK2);
}


// return the active VFO mode
byte getActiveVFOMode() {
    if (active_vfo == VFO_A_ACTIVE) {
        return vfoa_mode;
    } else {
        return vfob_mode;
    }
}


// set the active VFO mode
void setActiveVFOMode(byte mode) {
    if (active_vfo == VFO_A_ACTIVE) {
        vfoa_mode = mode;
    } else {
        vfob_mode = mode;
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

    // get it out
    setFreqBFO();
}


// change the steps
void changeStep() {
    // calculating the next step
    if (step == STEP_MAX) {
        // overflow, reset
        if (run_mode == NORMAL_MODE) {
            // if normal mode the minimum step is 10hz
            step = 1;
        } else {
            // if setup mode the min step depends on the item
            if ((config == CONFIG_LSB) or (config == CONFIG_USB) or (config == CONFIG_PPM)) {
                // overflow, 1 hz is only allowed on SETUP mode and on the LSB/USB/PPM
                step = 0;
            } else {
                // the minimum step is 10hz
                step = 1;
            }
        }
    } else {
        // simple increment
        step += 1;
    }

    // if in normal mode reset the counter to show the change in the LCD
    if (run_mode == NORMAL_MODE)
        showStepCounter = showStepTimer;     // aprox half second

    // tell the LCD that it must show the change
    showMode = false;
}


// RIT toggle
void changeRit() {
    if (rit_active) {
        // going to activate it: store the active VFO
        tvfo = getActiveVFOFreq();
    } else {
        // going to deactivate: reset the stored VFO
        if (active_vfo == VFO_A_ACTIVE) {
            vfoa = tvfo;
        } else {
            vfob = tvfo;
        }
    }
}


// check if the EEPROM is initialized
boolean checkInitEEPROM() {
    // read the eprom start to determine is the fingerprint is in there
    char fp;
    for (byte i=0; i<8; i++) {
        fp = EEPROM.read(i);
        if (fp != EEPROMfingerprint[i]) {
            return false;
        }
    }

    return true;
}


// split in bytes, take a unsigned long and load it on the fourBytes (MSBF)
void splitInBytes(unsigned long freq) {
    for (byte i=0; i<4; i++) {
        fourBytes[i] = char((freq >> (3-i)*8) & 255);
    }
}


// restore a unsigned long from the bour bytes in fourBytes (MSBF)
unsigned long restoreFromBytes() {
    unsigned long freq;
    unsigned long temp;

    // the MSB need to be set by hand as we can't cast a unisigned long, or we?
    freq = fourBytes[0];
    freq = freq << 24;

    // restore the lasting bytes
    for (byte i=1; i<4; i++) {
        temp = long(fourBytes[i]);
        temp = temp << ((3-i)*8);
        freq += temp;
    }

    // get it out
    return freq;
}


// read an unsigned long from the EEPROM, fourBytes as swap var
unsigned long EEPROMReadLong(word pos) {
    for (byte i=0; i<4; i++) {
        fourBytes[i] = EEPROM.read(pos+i);
    }
    return restoreFromBytes();
}


// write an unsigned long to the EEPROM, fourBytes as swap var
void EEPROMWriteLong(unsigned long val, word pos) {
    splitInBytes(val);
    for (byte i=0; i<4; i++) {
        EEPROM.write(pos+i, fourBytes[i]);
    }
}


// initialize the EEPROM mem, also used to store the values in the setup mode
void initEeprom() {
    /***************************************************************************
     * EEPROM structure (version 002)
     * Example: @00 (8): comment
     *  @##: start of the content of the eeprom var (always decimal)
     *  (#): how many bytes are stored there
     *
     * =========================================================================
     * @00 (8): EEPROMfingerprint, see the corresponding #define at the code start
     * @08 (4): IF freq in 4 bytes
     * @12 (4): VFO A in 4 bytes
     * @16 (4): VFO B in 4 bytes
     * @20 (4): BFO feq for the LSB mode
     * @24 (4): BFO feq for the USB mode
     * @28 (1): Encoded Byte in 4 + 4 bytes representing the USB and LSB mode
     *          of the A/B VFO
     * @29 (4): PPM correction for the Si5351 (TO TEST YET)
     * =========================================================================
     *
    /**************************************************************************/

    // temp var
    unsigned long temp = 0;

    // write the fingerprint
    for (byte i=0; i<8; i++) {
        EEPROM.write(i, EEPROMfingerprint[i]);
    }

    // IF
    EEPROMWriteLong(ifreq, 8);

    // VFO a freq
    EEPROMWriteLong(vfoa, 12);

    // VFO B freq
    EEPROMWriteLong(vfob, 16);

    // BFO for the LSB mode
    temp = lsb + 2147483647;
    EEPROMWriteLong(temp, 20);

    // BFO for the USB mode
    temp = usb + 2147483647;
    EEPROMWriteLong(temp, 24);

    // Encoded byte with the default modes for VFO A/B
    byte toStore = byte(vfoa_mode << 4) + vfob_mode;
    EEPROM.write(28, toStore);

    // Si5351 PPM correction value
    // this is a signed value, so we need to scale it to a unsigned
    temp = si5351_ppm + 2147483647;
    EEPROMWriteLong(temp, 29);
}


// load the eprom contents
void loadEEPROMConfig() {
    //temp var
    unsigned long temp = 0;

    // get the IF value
    ifreq = EEPROMReadLong(8);

    // get VFOa freq.
    vfoa = EEPROMReadLong(12);

    //  get VFOb freq.
    vfob = EEPROMReadLong(16);

    // get BFO lsb freq.
    temp = EEPROMReadLong(20);
    lsb = temp - 2147483647;

    // BFO lsb freq.
    temp = EEPROMReadLong(24);
    usb = temp - 2147483647;

    // get the deafult modes for each VFO
    char salvar = EEPROM.read(28);
    vfoa_mode = salvar >> 4;
    vfob_mode = salvar & 15;

    // Si5351 PPM correction value
    // this is a signed value, so we need to scale it to a unsigned
    temp = EEPROMReadLong(29);
    si5351_ppm = temp - 2147483647;
    // now set it up
    si5351.set_correction(si5351_ppm);
}


// main setup procedure: get all ready to rock
void setup() {
    // buttons debounce encoder push
    pinMode(btnPush,INPUT_PULLUP);
    dbBtnPush.attach(btnPush);
    dbBtnPush.interval(debounceInterval);

    // LCD init
    lcd.begin(16, 2);
    lcd.clear();

    // I2C init
    Wire.begin();

    // Interrupt init, the arduino way
    attachInterrupt(0, IR, CHANGE);
    attachInterrupt(1, IR, CHANGE);

    // Xtal capacitive load
    si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0);

    // setup the PLL usage
    si5351.set_ms_source(SI5351_CLK0, SI5351_PLLA);
    si5351.set_ms_source(SI5351_CLK1, SI5351_PLLB);
    si5351.set_ms_source(SI5351_CLK2, SI5351_PLLB);

    // check the EEPROM to know if I need to initialize it
    boolean eepromOk = checkInitEEPROM();
    if (!eepromOk) {
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
    }

    // Welcome screen
    lcd.setCursor(0, 0);
    lcd.print(F("  Aduino Arcs  "));
    lcd.setCursor(0, 1);
    lcd.print(F("Fv:001   Mfv:002"));
    delay(1000);        // wait for 1 second
    lcd.setCursor(0, 0);
    lcd.print(F(" by Pavel CO7WT "));
    delay(1000);        // wait for 1 second
    lcd.clear();

    // Check for setup mode
    if (digitalRead(btnPush) == LOW) {
        // we are in the setup mode
        lcd.setCursor(0, 0);
        lcd.print(F(" You are in the "));
        lcd.setCursor(0, 1);
        lcd.print(F("   SETUP MODE   "));
        initEeprom();
        delay(1000);        // wait for 1 second
        lcd.clear();

        // rise the flag of setup mode for every body to see it.
        run_mode = !NORMAL_MODE;

        // show setup mode
        showConfig();

        // setting up hot mode on VFO a
        active_vfo = VFO_A_ACTIVE;
    }

    // start the VFOa and it's mode
    setFreqToVFO();
    setFreqBFO();
}


/******************************************************************************
 * In setup mode the buttons change it's behaviour
 * - Encoder push button: it's "OK" or "ENTER"
 * - Mode: it's "Cancel & back to main menu"
 * - RIT: Frequency step control.
 * - VFO A/B:
 *   - On the USB/LSB freq. setup menus, this set the selected value to the IF
 *   - On the PPM adjust it reset it to zero
 ******************************************************************************/


// let's get the party started
void loop() {
    // encoder check
    if (encoderState != DIR_NONE) {
        if (encoderState == DIR_CW) {
            // FWD
            encoderMoved(+1);
        } else {
            // RWD
            encoderMoved(-1);
        }

        // encoder reset satate
        encoderState = DIR_NONE;
    }

    // LCD update check in normal mode
    if ((update == true) and (run_mode == NORMAL_MODE)) {
        // update and reset the flag
        updateLcd();
        update = false;
    }

    // analog buttons
    anab = ab.getStatus();

    // debouce for the push
    dbBtnPush.update();

    if (run_mode == NORMAL_MODE) {
        // we are in normal mode

        // VFO A/B
        if (anab == ABUTTON1_PRESS) {
            // we force to deactivate the RIT on VFO change, as it will confuse
            // the users and have a non logical use, only if activated and
            // BEFORE we change the active VFO
            if (rit_active) {
                rit_active = false;
                changeRit();
            }

            // now we change the VFO.
            active_vfo = !active_vfo;

            // update VFO/BFO and instruct to update the LCD
            setFreqToVFO();
            setFreqBFO();

            // set the LCD update flag
            update = true;
        }

        // mode change
        if (anab == ABUTTON2_PRESS) {
            changeMode();
            update = true;
        }

        // step (push button)
        if (dbBtnPush.fell()) {
            // VFO step change
            changeStep();
            update = true;
        }

        // RIT
        if (anab == ABUTTON3_PRESS) {
            // toggle rit mode
            rit_active = !rit_active;
            changeRit();
            update = true;
        }

        // timing the step show in the LCD
        if (!showMode) {
            // dow to zero
            showStepCounter -= 1;

            if (showStepCounter == 0) {
                // game over
                showMode = true;
                update = true;
            }
        }

    } else {
        // setup mode

        // OK (step)
        if (dbBtnPush.fell()) {
            if (!setup_in) {
                // I'm going to setup a element
                setup_in = true;

                // change the mode to follow VFO A
                if (config == CONFIG_USB)
                    vfoa_mode = MODE_USB;

                if (config  == CONFIG_LSB)
                    vfoa_mode = MODE_LSB;

                // config update on the LCD
                showModConfig();

            } else {
                // get out of the setup change
                setup_in = false;

                // save to the eeprom
                initEeprom();

                // lcd delay to show it pro perly (user feedback)
                lcd.setCursor(0, 0);
                lcd.print(F("##   SAVED    ##"));
                delay(250);

                // show setup
                showConfig();

                // reset the minimum step if set (1hz > 10 hz)
                if (step == 0)
                    step += 1;
            }
        }

        // cancel (mode)
        if (anab == ABUTTON2_PRESS) {
            if (setup_in) {
                // get out of here
                setup_in = false;

                // user feedback
                lcd.setCursor(0, 0);
                lcd.print(F(" #  Canceled  # "));
                delay(250);

                // show it
                showConfig();
            }
        }

        // step but just in setup mode (RIT)
        if ((anab == ABUTTON3_PRESS) and (setup_in)) {
            // change the step and show it on the LCD
            changeStep();
            showStep();
        }

        // reset the USB/LSB values to the IF values (VFO)
        if ((anab == ABUTTON1_PRESS) and (setup_in)) {
            // where we are ?
            if (config == CONFIG_USB) {
                // reset, activate and lcd update
                usb = -15000;
                setFreqBFO();
                showModConfig();
            }

            if (config == CONFIG_LSB){
                // reset, activate and lcd update
                lsb = 15000;
                setFreqBFO();
                showModConfig();
            }

            if (config == CONFIG_PPM){
                // reset, activate and lcd update
                si5351_ppm = 0;
                si5351.set_correction(0);
                setFreqToVFO();
                setFreqBFO();
                showModConfig();
            }
        }
    }
}

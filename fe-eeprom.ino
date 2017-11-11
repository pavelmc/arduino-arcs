/******************************************************************************
 *
 * This file is part of the Arduino-Arcs project, see
 *      https://github.com/pavelmc/arduino-arcs
 *
 * Copyright (C) 2016...2017 Pavel Milanes (CO7WT) <pavelmc@gmail.com>
 *
 * This program is free software under the GNU GPL v3.0
 *
 * ***************************************************************************/


/*
EEPROM amount vary from board to board, a simple list here:

Larger AVR processors have larger EEPROM sizes, E.g:
- Arduno Duemilanove: 512b EEPROM storage. (ATMega 168)
- Arduino Uno:        1kb EEPROM storage. (ATMega 328)
- Arduino Mega:       4kb EEPROM storage. (ATMega 2560)

Rather than hard-coding the length, you should use the pre-provided length
function. This will make your code portable to all AVR processors.

EEPROM.length()
*/


// check if the EEPROM is initialized
boolean checkInitEEPROM() {
    byte t;
    bool flag = true;   // true if eeprom is initialized and match

    // check the firmware version
    t = EEPROM.read(0);
    if (t != FMW_VER) flag = false;

    // check the eeprom version
    t = EEPROM.read(1);
    if (t != EEP_VER) flag = false;

    // return it
    return flag;
}


// initialize the EEPROM mem, also used to store the values in the setup mode
// this procedure has a protection for the EEPROM life using update semantics
// it actually only write a cell if it has changed
void saveEEPROM() {
    // write it
    EEPROM.put(0, u);
}


// load the eprom contents
void loadEEPROMConfig() {
    // get it
    EEPROM.get(0, u);

    // force to operation
    XTAL_C = XTAL + u.ppm;
    updateAllFreq();

    // force a reset
    Si5351_resets();
}


#ifdef MEMORIES
    // save memory location
    void saveMEM(word memItem, boolean configured) {
        // real or empty
        if (!configured) {
            // default values
            memo.configured = false;
            memo.vfo       = 7110000;
            memo.vfoMode   = MODE_LSB;
        } else {
            // ok, real ones, set the values
            memo.configured = true;
            memo.vfo        = *ptrVFO;
            memo.vfoMode    = *ptrMode;
        }

        // write it
        EEPROM.put(MEMSTART + (sizeof(mmem) * memItem), memo);
    }


    // load memory location
    boolean loadMEM(word memItem) {
        // get the values
        EEPROM.get(MEMSTART + (sizeof(mmem) * memItem), memo);

        // is the mem valid?
        if (!memo.configured) return false;

        // load it
        *ptrVFO     = memo.vfo;
        *ptrMode    = memo.vfoMode;

        // return true
        return true;
    }


    // wipe mem, this is loaded in the init process of the eeprom.
    void wipeMEM() {
        // run for the entire mem area writing the defaults to it, with no-go flag
        for (word i = 0; i <= memCount; i++ ) saveMEM(i, 0);
    }

#endif // memories

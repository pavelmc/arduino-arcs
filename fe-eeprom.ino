/******************************************************************************
 *
 * This file is part of the Arduino-Arcs project, see
 *      https://github.com/pavelmc/arduino-arcs
 *
 * Copyright (C) 2016 Pavel Milanes (CO7WT) <pavelmc@gmail.com>
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
    // read the eeprom config data
    EEPROM.get(0, conf);

    // check for the initializer and version
    if (conf.version == EEP_VER) {
        // so far version is the same, but the fingerprint has a different trick
        for (int i=0; i<sizeof(conf.finger); i++) {
            if (conf.finger[i] != EEPROMfingerprint[i]) return false;
        }
        // if it reach this point is because it's the same
        return true;
    }

    #ifdef MEMORIES
        // so far int's a new eeprom version, we need to wipe the MEM area to avoid
        // problems with the users
        wipeMEM();
    #endif // memories

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
    conf.ifreq      = ifreq;
    conf.lsb        = lsb;
    conf.usb        = usb;
    conf.cw         = cw;
    conf.ppm        = si5351_ppm;
    conf.version    = EEP_VER;
    strcpy(conf.finger, EEPROMfingerprint);

    // write it
    EEPROM.put(0, conf);
}


// load the eprom contents
void loadEEPROMConfig() {
    // write it
    EEPROM.get(0, conf);

    // load the parameters to the environment
    vfoa        = conf.vfoa;
    VFOAMode    = conf.vfoaMode;
    ifreq       = conf.ifreq;
    lsb         = conf.lsb;
    usb         = conf.usb;
    cw          = conf.cw;
    XTAL_C = XTAL + conf.ppm;
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

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

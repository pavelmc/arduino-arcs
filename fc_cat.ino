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
        // we use 1 hz resolution, so scale it
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
        return *ptrVFO;
    }


    // get mode from CAT
    byte catGetMode() {
        // get the active VFO mode and pass it
        return *ptrMode;
    }


    // get the s meter status to CAT
    byte catGetSMeter() {
        // returns a byte in wich the s-meter is scaled to 4 bits (1023 > 15)
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
            return (tx<<7) + (split<<5) + sMeter;
        #else
            return (tx<<7) + (split<<5);
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

#endif  // cat


// smart Delay trick, normal when no cat and non-blocking when cat
void smartDelay() {
    #ifdef CAT_CONTROL
        delayCat(); // 2 secs
    #else
        delay(2000);
    #endif
}

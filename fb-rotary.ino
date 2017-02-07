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


#ifdef ROTARY
    // the encoder has moved
    void encoderMoved(int dir) {
        // check if in memory
        #ifdef MEMORIES
            if (vfoMode == false) {
                // we are in mem mode, move it
                int tmem = mem;
                tmem += dir;

                // limits check
                if (tmem < 0) tmem = memCount;
                if (tmem > memCount) tmem = 0;

                mem = word(tmem);

                // update flag and return
                update = true;
                return;
            }
        #endif

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

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
            if (!vfoMode) {
                // we are in mem mode, move it
                int tmem = mem + dir;

                // limits check
                if (tmem < 0) tmem = memCount;
                if (tmem > (int)memCount) tmem = 0;

                // update the mem setting
                mem = word(tmem);
                loadMEM(mem);
                updateAllFreq();

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
        }

        #ifdef LCD   // no meaning if no lcd
            #ifdef ABUT    // no meaning if no Analog buttons
                if (!runMode) {
                    // update the values in the setup mode
                    updateSetupValues(dir);
                }
            #endif   // abut
        #endif  // nolcd
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
            am = am or (config == CONFIG_USB) or (config == CONFIG_PPM);
            if (!runMode and am) step = 1;
        }

        // if in normal mode reset the counter to show the change in the LCD
        if (runMode) showStepCounter = STEP_SHOW_TIME;
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

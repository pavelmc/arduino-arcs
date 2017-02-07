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


    #ifdef MEMORIES
        // VFO/MEM mode change
        void btnVFOMEM() {
            // toggle the flag
            vfoMode = !vfoMode;

            // rise the update flag
            update = true;
        }

        // mem > vfo | vfo > mem, taking into account in what mode we are
        void btnVFOsMEM() {
            // detect in which mode I'm, to decide what to do
            if (vfoMode == true) {
                // VFO > MEM
                saveMEM(mem, true);
            } else {
                // MEM > VFO
                loadMEM(mem);
            }
        }

        // erase the actual mem position
        void btnEraseMEM() {
            // erase the actual mem position
            saveMEM(mem, false);

            // if in vfo mode no problem, but...
            if (vfoMode == false) {
                // if in MEM mode jump to the NEXT valid mem.
                // but if we cycle trough the full mem we need to stop
                // at the next zero index
                boolean stop = false;
                word oldmem = mem;
                mem += 1;

                while (loadMEM(mem) == false) {
                    // if we reach the start point, that's a empty mem space.
                    // just stop and jump to ZERO
                    if (mem == oldmem) {
                        mem = 0;
                        return;
                    }

                    // next !
                    mem += 1;

                    // limit check
                    if (mem > memCount) mem == 0;
                }
            }
        }
    #endif

#endif

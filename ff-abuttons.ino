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
        // beep
        beep();
        
        // normal mode
        if (runMode) {
            #ifdef MEMORIES
                // if we are in mem mode VFO A/B switch has no sense
                if (!vfoMode) return;
            #endif
            
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
        // beep
        beep();
        
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
        // beep
        beep();
        
        // normal mode
        if (runMode) {
            #ifdef MEMORIES
                // if we are in mem mode RIT has no sense
                if (!vfoMode) return;
            #endif
            
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
        // beep
        beep();
        
        // normal mode
        if (runMode) {
            #ifdef MEMORIES
                // if we are in mem mode split has no sense
                if (!vfoMode) return;
            #endif
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
            // beep
            beep();
        
            // toggle the flag
            vfoMode = !vfoMode;

            // force the update of the mem
            if (!vfoMode) loadMEM(mem);

            // rise the update flag
            update = true;
        }

        // mem > vfo | vfo > mem, taking into account in what mode we are
        void btnVFOsMEM() {
            // beep
            beep();
        
            // detect in which mode I'm, to decide what to do
            if (vfoMode) {
                // VFO > MEM
                saveMEM(mem, true);
                // swap to mem mode
                vfoMode = false;
            } else {
                // MEM > VFO
                loadMEM(mem);
                // swap to the VFO mode
                vfoMode = true;
            }

            // update flag for the LCD
            update = true;
        }

        // erase the actual mem position
        void btnEraseMEM() {
            // beep
            beep();
        
            // erase the actual mem position
            saveMEM(mem, false);

            // now force an update of the LCD
            update = true;
        }
    #endif // memories
#endif

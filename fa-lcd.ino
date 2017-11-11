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


#ifdef LCD
    #ifdef ABUT
        // check some values don't go below zero, as some values are not meant
        // to be below zero, this check and fix that
        void belowZero(long *value) { if (*value < 0) *value = 0; }


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
                        // change the IF value
                        u.ifreq += getStep() * dir;
                        belowZero(&u.ifreq);
                        break;
                    case CONFIG_IF2:
                        // change the high IF value
                        u.if2 += getStep() * dir;
                        belowZero(&u.if2);
                        break;
                    case CONFIG_VFO_A:
                        // change VFOa
                        swapVFO(0); // set a
                        *ptrVFO += getStep() * dir;
                        belowZero(ptrVFO);
                        break;
                    case CONFIG_VFO_B:
                        // change VFOb
                        swapVFO(1); // set b
                        *ptrVFO += getStep() * dir;
                        belowZero(ptrVFO);
                        break;
                    case CONFIG_MODE_A:   // whichever VFO is active
                    case CONFIG_MODE_B:
                        // hot swap it
                        changeMode();
                        // set the default mode in the VFO A
                        showModeSetup(*ptrMode);
                        break;
                    case CONFIG_USB:
                        // change the mode to USB
                        *ptrMode = MODE_USB;
                        // change the USB BFO
                        u.usb += getStep() * dir;
                        break;
                    case CONFIG_CW:
                        // change the mode to CW
                        *ptrMode = MODE_CW;
                        // change the CW BFO
                        u.cw += getStep() * dir;
                        break;
                    case CONFIG_PPM:
                        // change the Si5351 PPM
                        u.ppm += getStep() * dir;
                        // instruct the lib to use the new ppm value
                        CXTAL = XTAL + u.ppm;
                        // reset the Si5351
                        Si5351_resets();
                        break;
                }

                // for all cases update the freqs
                updateAllFreq();

                // update el LCD
                showModConfig();
            }
        }


        // update the configuration item before selecting it
        void updateShowConfig(int dir) {
            // move the config item, it's a byte, so we use a temp var here
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
                    lcd.print(F(" Main IF freq.  "));
                    break;
                case CONFIG_IF2:
                    lcd.print(F(" High IF (opt)  "));
                    break;
                case CONFIG_VFO_A:
                    lcd.print(F("   VFO A freq   "));
                    break;
                case CONFIG_VFO_B:
                    lcd.print(F("   VFO B freq   "));
                    break;
                case CONFIG_MODE_A:
                    lcd.print(F("   VFO A mode   "));
                    break;
                case CONFIG_MODE_B:
                    lcd.print(F("   VFO B mode   "));
                    break;
                case CONFIG_USB:
                    lcd.print(F(" BFO freq. USB  "));
                    break;
                case CONFIG_CW:
                    lcd.print(F(" BFO freq. CW   "));
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


        // show the mode for the passed mode in setup mode
        void showModeSetup(byte mode) {
            // now I have to print it out
            lcd.setCursor(0, 1);
            spaces(11);
            showModeLcd(mode); // this one has a implicit extra space after it
        }


        // show the ppm as a signed long
        void showConfigValue(long val) {
            // add spacers
            spaces(3);

            // Show the sign only on config and not VFO & IF
            boolean t;
            t = config == CONFIG_VFO_A or config == CONFIG_IF;
            if (!runMode and !t) showSign(val);

            // print it
            formatFreq(abs(val));

            // if on normal mode we show in 10 Hz
            if (runMode) lcd.print("0");
            lcd.print(F("Hz"));
        }


        // update the specific setup item
        void showModConfig() {
            lcd.setCursor(0, 0);
            showConfigLabels();

            // show the specific values
            lcd.setCursor(0, 1);
            switch (config) {
                case CONFIG_IF:
                    showConfigValue(u.ifreq);
                    break;
                case CONFIG_IF2:
                    showConfigValue(u.if2);
                    break;
                case CONFIG_VFO_A:
                    showConfigValue(u.a);
                    break;
                case CONFIG_VFO_B:
                    showConfigValue(u.b);
                    break;
                case CONFIG_MODE_A:
                    showModeSetup(u.aMode);
                    break;
                case CONFIG_MODE_B:
                    showModeSetup(u.bMode);
                    break;
                case CONFIG_USB:
                    showConfigValue(u.usb);
                    break;
                case CONFIG_CW:
                    showConfigValue(u.cw);
                    break;
                case CONFIG_PPM:
                    showConfigValue(u.ppm);
                    break;
            }
        }
    #endif // abut|rotary


    // print the sign of a passed parameter
    void showSign(long val) {
        // just print it
        if (val > 0) lcd.print("+");
        if (val < 0) lcd.print("-");
        if (val == 0) lcd.print(" ");
    }


    // print a string of spaces, to save some flash/eeprom "space"
    void spaces(byte m) {
        // print m spaces in the LCD
        while (m) { lcd.print(" ");  m--; }
    }


    // format the freq to easy viewing
    void formatFreq(long freq) {
        // for easy viewing we format a freq like 7.110 to 7.110.00
        long t;

        // Mhz part
        t = freq / 1000000;
        if (t < 10) lcd.print(" ");
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


    // main update Lcd procedure
    void updateLcd() {
        // need to check if memories are available
        #ifdef MEMORIES
            // VFO or memory
            if (vfoMode) {
                // VFO mode, normal LCD
                vfoUpdateLcd();
            } else {
                memUpdateLcd();
            }
        #else
            vfoUpdateLcd();
        #endif // memories

    }


    #ifdef MEMORIES
        // lcd update in mem mode
        void memUpdateLcd() {
            /******************************************************
             *   0123456789abcdef
             *  ------------------
             *  |01 14.280.25 lsb|
             *
             ******************************************************/

            // first line
            lcd.setCursor(0, 0);

            // channel number with provision for the leading space below 10
            if (mem < 10) lcd.print("0");
            lcd.print(mem);
            lcd.print(" ");

            lcdRefresh();
        }
    #endif // memories


    // lcd update in normal mode
    void vfoUpdateLcd() {
        /******************************************************
         *   0123456789abcdef
         *  ------------------
         *  |A* 14.280.25 lsb|      normal
         *
         *  |A  14.280.25 lsb|      split
         ******************************************************/

        // first line
        lcd.setCursor(0, 0);
        // active a?
        if (activeVFO) {
            lcd.print("A");
        } else {
            lcd.print("B");
        }

        // split?
        if (split) {
            // ok, show the split status as a * sign
            lcd.print("* ");
        } else {
            // print a separator.
            spaces(2);
        }


        // main lcd routine
        lcdRefresh();
    }


    // refresh the lcd routine
    void lcdRefresh() {

        #ifdef MEMORIES
            if (!vfoMode and !memo.configured) {
                // memory not configured, so not showing the freq
                lcd.print(F("--.---.-- ---"));
            } else {
                // show VFO and mode
                formatFreq(*ptrVFO);
                // space and mode
                spaces(1);
                showModeLcd(*ptrMode);
            }


        #else
            // show VFO and mode
            formatFreq(*ptrVFO);
            spaces(1);
            showModeLcd(*ptrMode);
        #endif

        // second line
        lcd.setCursor(0, 1);
        if (tx) {
            lcd.print(F("TX "));
        } else {
            lcd.print(F("RX "));
        }

        // if we have a RIT or steps we manage it here and the bar will hold
        if (ritActive) showRit();
    }


    // show rit in LCD
    void showRit() {
        /***********************************************************************
         * RIT show something like this on the line of the non active VFO
         *
         *   |0123456789abcdef|
         *   |----------------|
         *   |RX RIT -9.99 khz|
         *   |----------------|
         *
         * WARNING !!! If the user change the VFO we need to *RESET*
         * & disable the RIT ASAP.
         *
         **********************************************************************/

        // get the active VFO to calculate the deviation & scale it down
        long diff = (*ptrVFO - tvfo)/10;

        // we start on line 2, char 3 of the second line
        lcd.setCursor(3, 1);
        lcd.print(F("RIT "));

        // show the difference in Khz on the screen with sign
        // diff can overflow the input of showSign, so we scale it down
        showSign(diff);

        // print the freq now, we have a max of 10 Khz (9.990 Khz)
        diff = abs(diff);

        // Khz part (999)
        word t = diff / 100;
        lcd.print(t);
        lcd.print(".");
        // hz part
        t = diff % 100;
        if (t < 10) lcd.print("0");
        lcd.print(t);
        spaces(1);
        // unit
        lcd.print(F("kHz"));
    }


    // show the mode on the LCD
    void showModeLcd(byte mode) {
        // print it
        switch (mode) {
            case MODE_LSB:
                lcd.print(F("LSB "));
                break;
            case MODE_USB:
                lcd.print(F("USB "));
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
        switch (step) {
            case 1:
                lcd.print(F("  1Hz"));
                break;
            case 2:
                lcd.print(F(" 10Hz"));
                break;
            case 3:
                lcd.print(F("100Hz"));
                break;
            case 4:
                lcd.print(F(" 1kHz"));
                break;
            case 5:
                lcd.print(F("10kHz"));
                break;
            case 6:
                lcd.print(F(" 100k"));
                break;
            case 7:
                lcd.print(F(" 1MHz"));
                break;
        }
        spaces(11);
    }


    // do you have SMETER?
    #ifdef SMETER
        // defining the chars for the Smeter
        /**  As per the LCD datasheet:
         * Each char is a matrix of 8x8
         * but internally  they are:
         * > 5 bits per line (lower 5 bits)
         * > 7 lines
         * > the underscore line
         ***/

        byte half[8] = {
          B11000,
          B11000,
          B11000,
          B11000,
          B11000,
          B11000,
          B11000,
          B00000
        };

        byte full[8] = {
          B11011,
          B11011,
          B11011,
          B11011,
          B11011,
          B11011,
          B11011,
          B00000
        };


        // show the bar graph for the RX or TX modes
        void showBarGraph() {
            // we are working on a 2x16 and we have 13 bars to show (0-14)
            // as we are on a double line we have 0-24 in value
            unsigned long ave = 0, i;
            volatile static byte barMax = 28;
            byte fb, hb;

            // pack for average
            for (i=0; i<BARGRAPH_SAMPLES; i++) ave += pep[i];
            // reduce to mean
            ave /= BARGRAPH_SAMPLES;

            // set the smeter reading on the global scope for CAT readings
            sMeter = ave;

            // scale it down to 0-24 from word
            byte local = map(ave, 0, 1023, 0, 28);

            // printing only the needed part of the bar, if growing or shrinking
            // if the same no action is required, remember we have to minimize the
            // writes to the LCD to minimize QRM

            // check for the bar redraw
            if (barReDraw) {
                barMax = 0;
                // always show at least a half bar
                if (local == 0) local = 1;
            }

            // growing bar: print the difference
            if (local > barMax) {
                // how many bars
                fb = (local - barMax) / 2;
                hb = (local - barMax) % 2;

                // LCD position
                lcd.setCursor(3 + (barMax/2), 1);

                // full bars
                if (fb > 0)
                    for (word i = 0; i < fb; i++)
                        lcd.write(byte(0)); // full bar

                // half bars
                // must be always just one half bar
                if (hb > 0)
                    lcd.write(byte(1));     // half bar
            }

            // shrinking bar: erase the old ones
            // just print spaces to erase just the diff
            if (barMax > local) {
                // base position, lower value
                fb = local / 2;     // base position
                hb = local % 2;

                // fail safe we always want a single bar even if zero
                if (local = 0) hb = 1;

                // LCD position
                lcd.setCursor(3 + fb, 1);

                // half bars
                if (hb > 0) {
                    // must be always just one half bar
                    lcd.write(byte(1));     // half bar
                }

                // erase the next resting bars
                spaces(((barMax + 1) - local) / 2);
            }

            // put the var for the next iteration
            barMax = local;

            // reset the bar redraw flag
            barReDraw = false;
        }


        // take a sample an inject it on the array
        void takeSample() {
            // reference is 5v
            word val;
            byte adcPin = 1;

            // check if TX
            if (tx) adcPin = 0;
            // take sample
            val = analogRead(adcPin);

            // push it in the array
            for (byte i = 0; i < BARGRAPH_SAMPLES - 1; i++) pep[i] = pep[i + 1];
            pep[BARGRAPH_SAMPLES - 1] = val;
        }


        // smeter reading, this take a sample of the smeter/txpower each time; an will
        // rise a flag when they have rotated the array of measurements 2/3 times to
        // have a moving average
        void smeter() {
            // static smeter array counter
            volatile static byte smeterCount = 0;

            // no matter what, I must keep taking samples
            takeSample();

            // it has rotated already?
            if (smeterCount > (BARGRAPH_SAMPLES * 2 / 3)) {
                // rise the flag about the need to show the bar graph and reset the count
                smeterOk = true;
                smeterCount = 0;
            } else {
                // just increment it
                smeterCount += 1;
            }
        }
    #endif

#endif  // lcd

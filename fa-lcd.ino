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


#ifndef NOLCD
    // defining the chars for the Smeter
    byte bar[8] = {
      B11111,
      B11111,
      B11111,
      B10001,
      B11111,
      B11111,
      B11111
    };

    byte s1[8] = {
      B11111,
      B10011,
      B11011,
      B11011,
      B11011,
      B10001,
      B11111
    };

    byte s3[8] = {
      B11111,
      B10001,
      B11101,
      B10001,
      B11101,
      B10001,
      B11111
    };

    byte s5[8] = {
      B11111,
      B10001,
      B10111,
      B10001,
      B11101,
      B10001,
      B11111
    };

    byte s7[8] = {
      B11111,
      B10001,
      B11101,
      B11011,
      B11011,
      B11011,
      B11111
    };

    byte s9[8] = {
      B11111,
      B10001,
      B10101,
      B10001,
      B11101,
      B11101,
      B11111
    };


    // check some values don't go below zero
    void belowZero(long *value) {
        // some values are not meant to be below zero, this check and fix that
        if (*value < 0) *value = 0;
    }


    // update the setup values
    void updateSetupValues(int dir) {
        // we are in setup mode, showing or modifying?
        if (!inSetup) {
            // just showing, show the config on the LCD
            updateShowConfig(dir);
        } else {
            // change the VFO to A by default
            swapVFO(1);
            // I'm modifying, switch on the config item
            switch (config) {
                case CONFIG_IF:
                    // change the IF value
                    ifreq += getStep() * dir;
                    belowZero(&ifreq);
                    break;
                case CONFIG_VFO_A:
                    // change VFOa
                    *ptrVFO += getStep() * dir;
                    belowZero(ptrVFO);
                    break;
                case CONFIG_MODE_A:
                    // hot swap it
                    changeMode();
                    // set the default mode in the VFO A
                    showModeSetup(VFOAMode);
                    break;
                case CONFIG_USB:
                    // change the mode to USB
                    *ptrMode = MODE_USB;
                    // change the USB BFO
                    usb += getStep() * dir;
                    break;
                case CONFIG_LSB:
                    // change the mode to LSB
                    *ptrMode = MODE_LSB;
                    // change the LSB BFO
                    lsb += getStep() * dir;
                    break;
                case CONFIG_CW:
                    // change the mode to CW
                    *ptrMode = MODE_CW;
                    // change the CW BFO
                    cw += getStep() * dir;
                    break;
                case CONFIG_PPM:
                    // change the Si5351 PPM
                    si5351_ppm += getStep() * dir;
                    // instruct the lib to use the new ppm value
                    XTAL_C = XTAL + si5351_ppm;
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


    // show the mode for the passed mode in setup mode
    void showModeSetup(byte mode) {
        // now I have to print it out
        lcd.setCursor(0, 1);
        spaces(11);
        showModeLcd(mode); // this one has a implicit extra space after it
    }


    // print a string of spaces, to save some flash/eeprom "space"
    void spaces(byte m) {
        // print m spaces in the LCD
        while (m) {
            lcd.print(" ");
            m--;
        }
    }


    // show the labels of the config
    void showConfigLabels() {
        switch (config) {
            case CONFIG_IF:
                lcd.print(F("  IF frequency  "));
                break;
            case CONFIG_VFO_A:
                lcd.print(F("   VFO A freq   "));
                break;
            case CONFIG_MODE_A:
                lcd.print(F("   VFO A mode   "));
                break;
            case CONFIG_USB:
                lcd.print(F(" BFO freq. USB  "));
                break;
            case CONFIG_LSB:
                lcd.print(F(" BFO freq. LSB  "));
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


    // print the sign of a passed parameter
    void showSign(long val) {
        // just print it
        if (val > 0) lcd.print("+");
        if (val < 0) lcd.print("-");
        if (val == 0) lcd.print(" ");
    }


    // show the ppm as a signed long
    void showConfigValue(long val) {
        lcd.print(F("Val:"));

        // Show the sign only on config and not VFO & IF
        boolean t;
        t = config == CONFIG_VFO_A or config == CONFIG_IF;
        if (!runMode and !t) showSign(val);

        // print it
        formatFreq(abs(val));

        // if on normal mode we show in 10 Hz
        if (runMode) lcd.print("0");
        lcd.print(F("hz"));
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
            case CONFIG_MODE_A:
                showModeSetup(VFOAMode);
            case CONFIG_USB:
                showConfigValue(usb);
                break;
            case CONFIG_LSB:
                showConfigValue(lsb);
                break;
            case CONFIG_CW:
                showConfigValue(cw);
                break;
            case CONFIG_PPM:
                showConfigValue(si5351_ppm);
                break;
        }
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


    // lcd update in normal mode
    void updateLcd() {
        // this is the designed normal mode LCD
        /******************************************************
         *   0123456789abcdef
         *  ------------------
         *  |A 14.280.25 lsb |
         *  |RX 0000000000000|
         *
         *  |RX +9.99 Khz    |
         *
         *  |RX 100hz        |
         *  ------------------
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
            lcd.print("*");
        } else {
            // print a separator.
            spaces(1);
        }

        // show VFO and mode
        formatFreq(*ptrVFO);
        spaces(1);
        showModeLcd(*ptrMode);

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
        /***************************************************************************
         * RIT show something like this on the line of the non active VFO
         *
         *   |0123456789abcdef|
         *   |----------------|
         *   |RX RIT -9.99 khz|
         *   |----------------|
         *
         *             WARNING !!!!!!!!!!!!!!!!!!!!1
         *  If the user change the VFO we need to *RESET* & disable the RIT ASAP.
         *
         **************************************************************************/

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
            case MODE_USB:
              lcd.print(F("USB "));
              break;
            case MODE_LSB:
              lcd.print(F("LSB "));
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
        if (step == 1) lcd.print(F("  1Hz"));
        if (step == 2) lcd.print(F(" 10Hz"));
        if (step == 3) lcd.print(F("100Hz"));
        if (step == 4) lcd.print(F(" 1kHz"));
        if (step == 5) lcd.print(F("10kHz"));
        if (step == 6) lcd.print(F(" 100k"));
        if (step == 7) lcd.print(F(" 1MHz"));
        spaces(11);
    }

    // show the bar graph for the RX or TX modes
    void showBarGraph() {
        // we are working on a 2x16 and we have 13 bars to show (0-12)
        unsigned long ave = 0, i;
        volatile static byte barMax = 0;

        // find the average
        for (i=0; i<BARGRAPH_SAMPLES; i++) ave += pep[i];
        ave /= BARGRAPH_SAMPLES;

        // set the smeter reading on the global scope for CAT readings
        sMeter = ave;

        // scale it down to 0-12 from word
        byte local = map(ave, 0, 1023, 0, 12);

        // printing only the needed part of the bar, if growing or shrinking
        // if the same no action is required, remember we have to minimize the
        // writes to the LCD to minimize QRM

        // if we get a barReDraw = true; then reset to redrawn the entire bar
        if (barReDraw) {
            barMax = 0;
            // forcing the write of one line
            if (local == 0) local = 1;
        }

        // growing bar: print the difference
        if (local > barMax) {
            // LCD position & print the bars
            lcd.setCursor(3 + barMax, 1);

            // write it
            for (i = barMax; i <= local; i++) {
                switch (i) {
                    case 0:
                        lcd.write(byte(1));
                        break;
                    case 2:
                        lcd.write(byte(2));
                        break;
                    case 4:
                        lcd.write(byte(3));
                        break;
                    case 6:
                        lcd.write(byte(4));
                        break;
                    case 8:
                        lcd.write(byte(5));
                        break;
                    default:
                        lcd.write(byte(0));
                        break;
                }
            }

            // second part of the erase, preparing for the blanking
            if (barReDraw) barMax = 12;
        }

        // shrinking bar: erase the old ones print spaces to erase just the diff
        if (barMax > local) {
            i = barMax;
            while (i > local) {
                lcd.setCursor(3 + i, 1);
                lcd.print(" ");
                i--;
            }
        }

        // put the var for the next iteration
        barMax = local;
        //reset the redraw flag
        barReDraw = false;
    }


    // take a sample an inject it on the array
    void takeSample() {
        // reference is 5v
        word val;

        if (!tx) {
            val = analogRead(1);
        } else {
            val = analogRead(0);
        }

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
    
#endif  // nolcd

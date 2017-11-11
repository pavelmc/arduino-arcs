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
        // we are working on a 2x16 and we have 13 bars to show (0-13)
        // as we are on a double line we have 0-24 in value
        unsigned long ave = 0, i;
        volatile static byte barMax = 26;
        byte fb, hb;

        // pack for average
        for (i=0; i<BARGRAPH_SAMPLES; i++) ave += pep[i];
        // reduce to mean
        ave /= BARGRAPH_SAMPLES;

        // set the smeter reading on the global scope for CAT readings
        sMeter = ave;

        // scale it down to 0-24 from word
        byte actual = map(ave, 0, 1023, 0, 26);

        // printing only the needed part of the bar, if growing or shrinking
        // if the same no action is required, remember we have to minimize the
        // writes to the LCD to minimize QRM

        // check for the bar redraw
        if (barReDraw) {
            barMax = 0;
            // always show at least a half bar
            if (actual == 0) actual = 1;
        }

        // growing bar: print the difference
        if (actual > barMax) {
            // reset barMax to a even number to avoid half bars loose
            if (barMax % 2) barMax += -1;

            // how many bars
            fb = (actual - barMax) / 2;
            hb = (actual - barMax) % 2;

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
        if (barMax > actual) {
            // base position, lower value
            fb = actual / 2;     // base position
            hb = actual % 2;

            // fail safe we always want a single bar even if zero
            if (actual = 0) hb = 1;

            // LCD position
            lcd.setCursor(3 + fb, 1);

            // half bars
            if (hb > 0) {
                // must be always just one half bar
                lcd.write(byte(1));     // half bar
            }

            // erase the next resting bars
            spaces(((barMax + 1) - actual) / 2);
        }

        // put the var for the next iteration
        barMax = actual;

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

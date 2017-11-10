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


// set a freq to a clk
// Frequency in Hz; must be within [7,810 kHz to ~220 MHz]
void si5351aSetFrequency(byte clk, unsigned long frequency) {
    #define c 1048575;
    unsigned long fvco;
    unsigned long outdivider;
    byte R = 1;
    byte a;
    unsigned long b;
    float f;
    unsigned long MSx_P1;
    unsigned long MSNx_P1;
    unsigned long MSNx_P2;
    unsigned long MSNx_P3;
    byte shifts = 0;

    // With 900 MHz beeing the maximum internal PLL-Frequency
    outdivider = 900000000 / frequency;

    // If output divider out of range (>900) use additional Output divider
    while (outdivider > 900) {
        R = R * 2;
        outdivider = outdivider / 2;
    }

    // finds the even divider which delivers the intended Frequency
    if (outdivider % 2) outdivider--;

    // Calculate the PLL-Frequency (given the even divider)
    fvco = outdivider * R * frequency;

    // Convert the Output Divider to the bit-setting required in register 44
    switch (R) {
        case 1: R = 0; break;
        case 2: R = 16; break;
        case 4: R = 32; break;
        case 8: R = 48; break;
        case 16: R = 64; break;
        case 32: R = 80; break;
        case 64: R = 96; break;
        case 128: R = 112; break;
    }

    a = fvco / XTAL_C;
    f = fvco - a * XTAL_C;
    f = f * c;
    f = f / XTAL_C;
    b = f;

    MSx_P1 = 128 * outdivider - 512;
    f = 128 * b / c;
    MSNx_P1 = 128 * a + f - 512;
    MSNx_P2 = f;
    MSNx_P2 = 128 * b - MSNx_P2 * c;
    MSNx_P3 = c;

    // PLLs and CLK# registers are allocated with a shift, we handle that with
    // the shifts var to make code smaller
    if (clk > 0 ) shifts = 8;

    // plls, A & B registers separated by 8 bytes
    si5351ai2cWrite(26 + shifts, (MSNx_P3 & 65280) >> 8);   // Bits [15:8] of MSNx_P3 in register 26
    si5351ai2cWrite(27 + shifts, MSNx_P3 & 255);
    si5351ai2cWrite(28 + shifts, (MSNx_P1 & 196608) >> 16);
    si5351ai2cWrite(29 + shifts, (MSNx_P1 & 65280) >> 8);   // Bits [15:8]  of MSNx_P1 in register 29
    si5351ai2cWrite(30 + shifts, MSNx_P1 & 255);            // Bits [7:0]  of MSNx_P1 in register 30
    si5351ai2cWrite(31 + shifts, ((MSNx_P3 & 983040) >> 12) | ((MSNx_P2 & 983040) >> 16)); // Parts of MSNx_P3 and MSNx_P1
    si5351ai2cWrite(32 + shifts, (MSNx_P2 & 65280) >> 8);   // Bits [15:8]  of MSNx_P2 in register 32
    si5351ai2cWrite(33 + shifts, MSNx_P2 & 255);            // Bits [7:0]  of MSNx_P2 in register 33

    // CLK# registers are exactly 8 * clk bytes shifted from a base register.
    shifts = clk * 8;

    // multisynths
    si5351ai2cWrite(42 + shifts, 0);                // Bits [15:8] of MS0_P3 (always 0) in register 42
    si5351ai2cWrite(43 + shifts, 1);                // Bits [7:0]  of MS0_P3 (always 1) in register 43
    // See datasheet, special trick when R=4
    if (outdivider == 4) {
        si5351ai2cWrite(44 + shifts, 12 | R);
        si5351ai2cWrite(45 + shifts, 0);            // Bits [15:8] of MSx_P1 must be 0
        si5351ai2cWrite(46 + shifts, 0);            // Bits [7:0] of MSx_P1 must be 0
    } else {
        si5351ai2cWrite(44 + shifts, ((MSx_P1 & 196608) >> 16) | R);  // Bits [17:16] of MSx_P1 in bits [1:0] and R in [7:4]
        si5351ai2cWrite(45 + shifts, (MSx_P1 & 65280) >> 8);    // Bits [15:8]  of MSx_P1 in register 45
        si5351ai2cWrite(46 + shifts, MSx_P1 & 255);             // Bits [7:0]  of MSx_P1 in register 46
    }
    si5351ai2cWrite(47 + shifts, 0);                        // Bits [19:16] of MS0_P2 and MS0_P3 are always 0
    si5351ai2cWrite(48 + shifts, 0);                        // Bits [15:8]  of MS0_P2 are always 0
    si5351ai2cWrite(49 + shifts, 0);                        // Bits [7:0]   of MS0_P2 are always 0

    // no pll reset as it makes noise, click noise, just reset one time on the setup
}


// reset the PLL this will produce a click noise
void Si5351_resets() {
    // PLL & synths reset

    // This soft-resets PLL A & and enable it's output
    si5351ai2cWrite(177, 32);
    si5351ai2cWrite(16, 79);

    // This soft-resets PLL B & and enable it's output
    si5351ai2cWrite(177, 128);
    si5351ai2cWrite(17, 111);
}


// write a byte value to a resgister on the Si5351
void si5351ai2cWrite(byte regist, byte value){
    Wire.beginTransmission(96);
    Wire.write(regist);
    Wire.write(value);
    Wire.endTransmission();
}


// set the calculated freq to the VFO
void setFreqVFO() {
    // temp var to hold the calculated value
    long freq = *ptrVFO;

    //setting about xtal in the mix of the 1st to 2nd IF
    if (u.xtal != 0) {
        // add the xtal to the mix to get the VFO on the correct freq
        freq += u.xtal;
    } else {
        // add just the unique IF
        freq += u.ifreq;
    }

    // apply the ssb/cw correction factor
    if (*ptrMode == MODE_USB) freq += u.usb;
    if (*ptrMode == MODE_CW)  freq += u.cw;

    // set the Si5351 up with the change
    si5351aSetFrequency(0, freq);
}


// Force freq update for all the environment vars
void updateAllFreq() {
    // VFO update
    setFreqVFO();

    // BFO update
    long freq = u.ifreq;

    // deactivate it if zero
    if (freq == 0) {
        // deactivate it
        si5351aSetFrequency(1, 0);
    } else {
        // mod it by mode
        if (*ptrMode == MODE_USB) freq += u.usb;
        if (*ptrMode == MODE_CW)  freq += u.cw;

        // output it
        si5351aSetFrequency(1, freq);
    }
}

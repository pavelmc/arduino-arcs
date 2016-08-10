Arduino ARCS Manual
=========================

This is a short manual to know how to operate the sketch and the button's functions. The sketch has two modes, the user.'s mode and the setup mode, let's review them

User mode
=========

General description
-------------------

The user mode is the natural way the hardware boots if you don't touch anything, yo will see the Arduino ARCS banner and a note of the firmware version and the memory firmware version and the author, then you will get the main screen.

The main screen is composed of two lines the top line and the bottom line, both lines has the same information for each of the two VFOs the sketch has, upper is VFO A and lower is VFO B. You know which VFO is selected by the ">" sign in front of it.

The frequency is printed grouped by units and separated by a dot to easy viewing, first the Mhz units, then the khz part and finally the Hz part.

The frequency is truncated at the 10th of hz part to be honest, the Si5351 has a resolution of 0.1 hz in our configuration and an accuracy of 0 ppm error against the reference crystal used (that's per the OEM datasheet), then all the accuracy is referenced to the accuracy of the reference crystal and it has (usually on free air) about 10 ppm of error; using a 27 Mhz xtal we can expect a extreme variation of about 270 hz with temperature, voltage and humidity fluctuations.

In the best case scenario if you have a relatively stable operating temperature and humidity you can get about less 10hz of variation normally under this conditions.

This is why we have not 1hz accuracy in place, we can implement it, sure, the chip will be happy moving in 1hz steps but that "accuracy" will be undermined by the real stability of the reference crystal unless you managed to get a temp oven for the crystal, and that is an overkill to out goal.

If you have a crystal in an oven and you want to use 1hz steps I will be happy to mod the sketch for you.


VFO button
----------

This Button toggles from the two available VFOs, as simple as that.

RIT button
----------

This button toggles the RIT function for the **active** VFO and will show the RIT detail in the other position, the RIT uses a +/- 9.9 Khz, so you can go further than that; this can be modified by software. When you have the RIT active and hit this button again, it will reset the VFO to the previous value as you expect.

The step on the RIT mode is fixed by software at 100hz, you can get it at the 10hz step if you change it on the source code in the sketch.

A note: when you have the RIT active on one VFO and you want to switch to the other VFO the active RIT will be reseted before the switch to the other VFO.

MODE button
-----------

The mode button change the mode of the **active** VFO in a closed loop from one of the predefined modes, so far: LSB > USB > CW and then back to LSB.

STEP button _(encoder push button)_
---------------------------------

The encoder push button has the control of the steps in the rotation of the encoder, pushing it will cycle steps in 10hz, 100hz, 1khz, 10khz, 100khz and 1 Mhz and then back to 10hz.

Upon the modification of the step, the LCD will show the value in the area used to show the S-meter bargraph for about three seconds.

Encoder
-------

The encoder is simply the vernier to selecting the frequency of the active VFO in the steps selected by the push button described before.


SETUP mode
==========

The setup mode is reached by powering on the circuit and holding the encoder button pressed until you see the acknowledge in the LCD for it being on the SETUP mode.

In the setup mode you will find a menu with the following options to select and change on the encoder rotation and push.

 * IF frequency
 * VFO A start frequency
 * VFO B start frequency
 * VFO frequency for the LSB mode
 * VFO frequency for the USB mode
 * VFO frequency for the CW mode
 * VFO A start mode
 * VFO B start mode
 * XFO frequency
 * Si5351 PPM error correction

All this options are cycled via the encoder rotation in one or another direction and selected via the encoder push button; we will describe the buttons use in the setup mode and be aware that they change a little in the setup mode, we will name it as the usual function and will name the special function between parenthesis.

Encoder push button _(Select & Confirm)_
-----------------------------------

This button is used to select the option in the main menu to be modified, and also to confirm it's change after it was modified and get it back to the main menu saving it on the EERPOM.

MODE button _(Cancel)_
--------------------

This button has effect only when you have selected and option in the main menu and it's purpose is to leave this option unchanged and get back to the main menu. If you changed the value of an option selected (by rotating the encoder) but you want to cancel it's modification this option is what you need.

RIT button _(Step)_
-----------------

This button has only effect when you have selected to modify a menu entry that leads to a numeric option (for example frequencies, PPM error, etc.) and it will change the step for the encoder rotation showing the value selected in the LCD until you rotate the encoder.

VFO button _(Reset)_
------------------

This button has a special meaning in the options refereed to LSB & USB BFO frequencies and the Si5351 PPM correction. Pressing this buttons will reset the values of that option to it's default values as this:

 * BFO LSB frequency = -15000
 * BFO USB frequency = +15000
 * BFO CW frequency = +10000
 * Si5351 PPM error correction = 0

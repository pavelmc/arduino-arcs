
# Arduino-Arcs #

_**Amateur Radio Control & Clocks (RF) Solution**_

This is an amateur approach to a radio control solution with clock (RF sources) generation (VFO and BFO) for homebrewed and old (not frequency synthesized) transceivers; of course based on the Arduino platform.

You can [take a peek here](http://www.qrz.com/db/wj6c) to see what's it looks like in the bench.

## Motivations ##

This project arose as a solution for the Cuban hams who have built a simple QRP transceivers but lacks the principal part: a digital controller to avoid the "DRIFT" and get some of the standard radio features a ham's needs today in a radio.

[Juan Carlos (WJ6C)](http://www.qrz.com/db/wj6c), Axel (CO6ATL), [Heriberto (CM2KMK)](http://www.qrz.com/db/wj6c) and many others hams were trying with some Arduino sketches found on the network until [I (CO7WT)](http://www.qrz.com/db/co7wt) decided to give a hand using my skills and expertise on MCU programming with [Jal for the PICs](http://www.justanotherlanguage.com) & [Arduino](http://www.arduino.cc).

This code is the result of a group of Cubans that joint together to fulfill the expectations for a simple, affordable and yet modern radio logic controller for homebrewed radios that any ham in the world can use.

This work is and always will be in constant development, this must be always considered a Beta version.

## Hardware ##

We are using the ubiquitous Arduino boards for the main brain, a list of the needed hardware follows.

- Arduino as the brain; developed for the Arduino Uno R3 as base, but it's adaptable to other boards, we are using mainly the Mini & Mini Pro now (Yes it's less than 16 kb at the end)
- LCD 16 columns and 2 lines as display (LiquidCrystal default Arduino lib)
- Si5351 as VFO and BFO, any of the breakout board out there will work, we choose to use a embedded code for the Si5351 control. Our code is a modified version of one of the examples in the [QRP Labs site](http://qrp-labs.com) (with my mods to allow two outputs instead of only one) and trick learned from [DK7IH](https://radiotransmitter.wordpress.com/category/si5351a/) to eliminate the unpleasant clicks when tunning.
- For the human interface part we use a bunch of [analog push buttons with dual functions](https://github.com/pavelmc/BMUx/) and a [rotary encoder](https://github.com/mathertel/RotaryEncoder) + [push button](https://github.com/thomasfredericks/Bounce2/).
- Since September of 2016 the sketch has CAT support, if you are using an Arduino R3 or others with a USB port you are done; if not you need any of the [cheap ebay USB to RS-232/TTL converters](http://www.google.com/q=cp-2021+USB+serial+ttl+converter) just watch out for the correct drivers if you use any Windows OS.

You can see the schematic diagram (designed for an Arduino Uno R3 in mind) in the file sketch_schema.png (I can provide Fritzing files on request, but only the schematic part is ok.)

## Features implemented by now ##

- **Usual features** of normal commercial transceivers.
    - Two VFO (A/B)
    - RIT with +/- 9.99 Khz
    - Variable VFO speed in steps of 10hz, 100hz (Default), 1khz, 10khz, 100khz, 1Mhz (just push of the encoder)
    - Split for contest/pileups
    - VFOs A & B are preserved in the EEPROM across power cycle. _(See Note 1 below)_
- **S-Meter in the LCD** _(Vref is 5.0V)_
- **Relative TX power in the LCD** _(using the same S-Meter tech, no power calcs, yet)_
- Initially mono-band in 40m, but **adaptable to any band**
- **Full user customization** of the IF and BFO modes (side bands) via configuration menus. (The VFO is assumed always above the RF and side bands are adjusted, se notes on Si5351 noise)
- **Hot tunning of the parameters** when in configuration mode for ease the adjust.
- **Basic CAT control** (like a Yaesu FT-857D) using my GPL 3.0 [ft857d library](https://github.com/pavelmc/ft857d) _(See Note 2 below)_
- The sketch is coded with **feature segmentation in mind**, you can rule out the CAT support if you don't need it, disable the ugly S-meter or rule out all other HID and just compile with CAT to get a slim CAT radio solution that fits in a tiny ATMega8 core _(I'm no kidding, it do fit in that chip!)_
- **Memories** Using the internal EEPROM and limited to 100 (0-99), because the channel number is displayed in two LCD chars. Also you will get what your Chip has to offer: bigger chips my exceed the 100 mem channels count and get toped, but other not and you will have less than 100 mem channels.

_**Note 1:** The firmware save the VFO info in a 10 minutes interval._

_I was once concerned with the EERPOM wear out, but not anymore: The auto-save every 10 minutes **with a heavy use** of the transceiver can lead to about 3 years of life for the internal EEPROM _under datasheet conditions_. But you can expect at least x2 of that amount as the real world data shows (that's at least 6 years with heavy use or more than 10 years with little use)_

_Since October 2016 we are now using the new EEPROM.h library (since Arduino IDE 1.6.9 and later); that lib do a trick to preserve the internal EEPROM life against the wear out, so the real life of the EEPROM must be more than 6 years minimum, that for a **heavy daily use**._

_**Note 2:** If you use a regular Arduino board and upload the code via serial port you do have a bootloader in place. In this case when you setup your CAT program (in your PC) you will have to set the "retries" parameter to 3 or more and the "timeout" parameter to 500 msec or more to get the CAT working. (you will have to play with it a bit to get it working)_

_This is because the CAT setup process in your PC/software will reset the Arduino board (and hence the "radio")_

_Then the bootloader will introduce a short delay before our firmware kicks in and the PC may complain about "radio not answering to commands". The fix is in the previous paragraph. If you have no bootloader and upload the code via ICP you must not see this problems._

## Features in the TODO list ##

This are the wish list so far, with no particular order.

- Multi-band support with a function to jump quickly from band to band.
- Band selector for BPF switching via a external I2C port expander like the boards with the PCF8574, or alternatively a PIC programed to the task.
- Real Power and SWR measurements.
- Better S-Meter graphics
- Support for various LCDs, for example the Nokia 5110, some of the TFTs and some OLEDs.

## Ino then Arturo now Amake ##

Old users of this sketch may notice that it's now full Arduino IDE compatible _(since January/2017 with the release of the Arduino IDE version 1.8.0)_ and you don't need to make any more changes to the name of the file.

I use Ubuntu Linux for daily work here, and I don't like the Arduino IDE as a IDE. I use the [Geany IDE](http://www.geany.org) to code in Jal, Python, Arduino etc. In the past two projects helped me to link my Geany IDE to the Arduino tool-chain: [ino](https://github.com/amperka/ino) and it's folk [Arturo](https://github.com/scottdarch/Arturo/) when Ino reached the end of development. Thanks to the developers of this two projects for the great tools.

Since the release of the Arduino IDE version 1.8.0 it's possible to use a new tool "arduino-builder" that make possible to compile and upload a sketch entirely from the command line, but that tool is a nightmare to configure for the job in the command line.

And that was the born of [Amake](https://github.com/pavelmc/amake/), this a bash script that introduce a layer to get the compilation and uploading of arduino sketches from the command line like a breeze, and it's full Arduino IDE compatible. It's my contribution to make the life simpler ;-)

## Si5351 known issues ##

The Si5351 chip is not the panacea of the frequency generator (but it's a close one), as almost all good things in life it has a dark side, know more about in [this](https://github.com/pavelmc/arduino-arcs/Si5351_issues.md) document.

73 Pavel CO7WT.

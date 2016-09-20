
# Arduino-Arcs #

_**Amateur Radio Control & Clocks Solution**_

This is an amateur approach to a radio control solution with clock (RF sources) generation (VFO, XFO and BFO) for homebrewed and old (not frequency synthesized) transceivers; of course based on the Arduino platform.

You can [take a peek here](http://www.qrz.com/db/wj6c) to see what's it looks like in the bench.

## Motivations ##

This project arose as a solution for the Cuban hams who have built a simple QRP transceivers but they lack the principal part; a digital controller to avoid the "DRIFT".

[Juan Carlos (WJ6C)](http://www.qrz.com/db/wj6c), Axel, [Heriberto (CM2KMK)](http://www.qrz.com/db/wj6c) and many others hams were trying with some Arduino sketches found on the network until [I (CO7WT)](http://www.qrz.com/db/co7wt) decided to give a hand using my skills and expertises on MCU programming with [Jal for the PICs](http://www.justanotherlanguage.com)  & [Arduino](http://www.arduino.cc).

This code is the result of a group of Cubans that joint together to fulfill the expectations for a simple, affordable and yet modern radio logic controller for homebrewed radios that any ham in the world can use. As a user has pointed recentry: _"That sketch can be named the universal radio front panel"_

This work is and always will be in constant development, this is considered a Beta version.

## Hardware ##

We are using the ubiquitous Arduino board for the main brain, a list of the hardware follows.

- Arduino as the brain (developed for the Arduino Uno R3 as base, but it's adaptable to other boards)
- LCD 16 columns and 2 lines as display (LiquidCrystal default Arduino lib)
- Si5351 as VFO, BFO and optionally a XFO for a second conversion radios, any if the breakout board out here will work, we use the [Arduino Si5351 Etherkit library](https://github.com/etherkit/Si5351Arduino)
- [Digital push buttons](https://github.com/thomasfredericks/Bounce2/), [analog push buttons](https://github.com/pavelmc/AnaButtons/) and a [rotary encoder](https://github.com/mathertel/RotaryEncoder) with a push button as HID

About the schematics I can offer this files made with [Fritzing](http://www.fritzing.org):

- Whole Fritzin file: sketch.fzz (the PCB view is a proposal and it's not recomended for production as it's)
- Arduino wiring diagram: sketch_bb.png
- Schematic diagram: sketch_esquema.png

## Features implemented by now ##

- Usual features of normal commercial transceivers.
    - Two VFO (A/B)
    - RIT with +/- 9.99 Khz
    - Variable VFO speed (steps: 10hz, 100hz, 1khz, 10khz, 100khz, 1Mhz) with the push of the encoder
    - Split work for contest/pileups
- S-Meter in the LCD ([WARNING], by now the reference is the internal 1.1V one)
- Relative TX power in the LCD (using the same S-Meter tech)
- Initially mono-band in 40m.
- Full user customization of the IF & BFOs and more via configuration menus.
- Full user customization of the XFO for radios with a second conversion on the IF. _(See note below)_
- Hot tunning of the parameters when in configuration mode for ease the adjust.
- Basic CAT control (like a Yaesu FT-857d) using my GPL 3.0 [ft857d library](https://github.com/pavelmc/ft857d).

_**Note:** It's a known fact that the Si5351 can output 3 frequencies at a time, but in this case two of the freqs shares a common VCO inside the chip; the Si5351 library make some sacrifices to get the outputs actives mining the accuracy of one of the outputs with a shared VCO._

_Take that into account if you plan to use the 3 outputs, if one of the two shared VCO outputs is below 1 Mhz you will like to get the accuracy sacrificed._

## Features in the TODO list ##

This are the wish list so far, with no particular order.

- The VFO status is remembered with the power off/on cycles (require the use of an external EEPROM)
- Memories, with an external EEPROM (24Cxx) on the already existent I2C bus, the amount of the memories will be proportional to the size of the EEPROM and the kind of it will be auto-detected (I2C bus scan) and can be fine tuned via configuration.
- Multi-band support with a function to jump quickly from band to band.
- Band selector for BPF switching via a external I2C linked PIC as a output expander.
- Power and SWR measurements.

## sketch.ino vs arduino-arcs.ino? ##

I have received a few questions about why the project file is called sketch.ino instead arduino-arcs, or why the Arduino IDE warn about moving the sketch.ino file to it's own folder... if this has happened to you, keep reading below.

From my point of vew Arduino IDE software is so limited in features to be a decent developer's IDE. So I make use of a github project named [Arturo](https://github.com/scottdarch/Arturo/) (kudos for the Arturo team) that is a folk of [ino](https://github.com/amperka/ino), that allows me to use my beloved [Geany IDE](http://www.geany.org) and use the build and upload features from inside it.

If you use the default Arduino IDE then after download/clone the project just rename the sketch.ino project file to arduino-arcs.ino before clicking if to get opened by the Arduino IDE or you will get a warning about "moving the project to it's own folder", etc...

It's annoying for you, I know, but please do it for me.

## Si5351 known issues ##

The Si5351 chip is not the panacea of the frequency generator (but it's close), as almost all good things in life it has it's dark side, know more about in the Si5351_issues.md file in this proyect.

73 Pavel CO7WT.


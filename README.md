
# Arduino-Arcs #

_**Amateur Radio Control & Clocks (RF) Solution**_

This is an amateur approach to a radio control solution with clock (RF sources) generation (VFO, XFO and BFO) for homebrewed and old (not frequency synthesized) transceivers; of course based on the Arduino platform.

You can [take a peek here](http://www.qrz.com/db/wj6c) to see what's it looks like in the bench.

## Motivations ##

This project arose as a solution for the Cuban hams who have built a simple QRP transceivers but lacks the principal part: a digital controller to avoid the "DRIFT" and some of the standard radio features a ham's needs today.

[Juan Carlos (WJ6C)](http://www.qrz.com/db/wj6c), Axel, [Heriberto (CM2KMK)](http://www.qrz.com/db/wj6c) and many others hams were trying with some Arduino sketches found on the network until [I (CO7WT)](http://www.qrz.com/db/co7wt) decided to give a hand using my skills and expertises on MCU programming with [Jal for the PICs](http://www.justanotherlanguage.com) & [Arduino](http://www.arduino.cc).

This code is the result of a group of Cubans that joint together to fulfill the expectations for a simple, affordable and yet modern radio logic controller for homebrewed radios that any ham in the world can use.

This work is and always will be in constant development, this must be always considered a Beta version.

## Hardware ##

We are using the ubiquitous Arduino boards for the main brain, a list of the needed hardware follows.

- Arduino as the brain (developed for the Arduino Uno R3 as base, but it's adaptable to other boards, we are using now the Mini Pro 5V/ATmega328p actually)
- LCD 16 columns and 2 lines as display (LiquidCrystal default Arduino lib)
- Si5351 as VFO, BFO and optionally a XFO for a second conversion radios, any if the breakout board out here will work, we use the [Arduino Si5351 Etherkit library](https://github.com/etherkit/Si5351Arduino)
- [Digital push buttons](https://github.com/thomasfredericks/Bounce2/), [analog push buttons](https://github.com/pavelmc/AnaButtons/) and a [rotary encoder](https://github.com/mathertel/RotaryEncoder) with a push button as HID
- Since September of 2016 the sketch has CAT support, if you are using an Arduino R3 or others with a USB port you are done; if not you need any of the [cheap ebay USB to RS-232/TTL converters](http://www.google.com/q=cp-2021+USB+serial+ttl+converter) just watch out for the correct drivers if you use any Windows OS.

You can see the schematic diagram (designed for an Arduino Uno R3 in mind) in the file sketch_schema.png (I can provide Fritzing files on request, but just the schematic part is ok.)

## Features implemented by now ##

- Usual features of normal commercial transceivers.
    - Two VFO (A/B)
    - RIT with +/- 9.99 Khz
    - Variable VFO speed (steps: 10hz, 100hz, 1khz, 10khz, 100khz, 1Mhz) with the push of the encoder
    - Split work for contest/pileups
    - VFOs A/B and it's matching mode are preserved in the EEPROM across resets: the status is saved to eeprom in each VFO A/B button press. _(See Note 1 below)_
- S-Meter in the LCD (**WARNING**, by now the reference is the internal 1.1V one)
- Relative TX power in the LCD (using the same S-Meter tech)
- Initially mono-band in 40m.
- Full user customization of the IF & BFOs and more via configuration menus.
- Full user customization of the XFO for radios with a second conversion on the IF. _(See Note 2 below)_
- Hot tunning of the parameters when in configuration mode for ease the adjust.
- Basic CAT control (like a Yaesu FT-857D) using my GPL 3.0 [ft857d library](https://github.com/pavelmc/ft857d).

_**Note 1:** We are working on a automatic save, but for now it's manual._

_I'm concerned with the EERPOM wear out, auto-save every 30 minutes + VFO switch (both if changed) with a heavy use of the transceiver can lead to about 7 years of life for the EEPROM under datasheet conditions, but x2 of that (at least) as experience shows._

_**Note 2:** It's a known fact that the Si5351 can output 3 frequencies at a time, but in this case two of the freqs shares a common VCO inside the chip; the Si5351 library make some sacrifices to get the outputs actives mining the accuracy of one of the outputs with a shared VCO._

_Take that into account if you plan to use the 3 outputs, if one of the two shared VCO outputs is below 1 Mhz you will likely get the accuracy sacrificed. See more on the file Si5351_issues.md_

## Features in the TODO list ##

This are the wish list so far, with no particular order.

- Memories, with the internal EEPROM the amount of the memories will be proportional to the size of the EEPROM.
- Multi-band support with a function to jump quickly from band to band.
- Band selector for BPF switching via a external I2C bus linked PIC as a output expander.
- Power and SWR measurements.

## sketch.ino vs arduino-arcs.ino? ##

I have received a few questions about why the project file is called sketch.ino instead arduino-arcs, or why the Arduino IDE warn about moving the sketch.ino file to it's own folder... if this has happened to you, keep reading below.

From my point of vew the Arduino IDE software is a very basic IDE, it need more to be decent developer's IDE. So I make use of a github project named [Arturo](https://github.com/scottdarch/Arturo/) (kudos for the Arturo team) that is a folk of [ino](https://github.com/amperka/ino), that allows me to use my beloved [Geany IDE](http://www.geany.org) and use the build and upload features from inside it.

If you use the default Arduino IDE then after download/clone the project just rename the sketch.ino project file to arduino-arcs.ino before clicking it to get opened by the Arduino IDE, or you will get a warning about "moving the project to it's own folder", etc...

It's annoying for you, I know, but please do it for me.

## Si5351 known issues ##

The Si5351 chip is not the panacea of the frequency generator (but it's a close one), as almost all good things in life it has a dark side, know more about in the Si5351_issues.md file in this proyect.

73 Pavel CO7WT.

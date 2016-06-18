
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

You can see a wiring example in the Schematics.jpg file.

** WARNING ** the circuit has changed with the use of the analog buttons, this will be updated soon.

## Features implemented by now ##

- Usual features of normal commercial transceivers.
    - Two VFO (A/B)
    - RIT with +/- 9.99 Khz
    - Variable VFO speed (steps: 10hz, 100hz, 1khz, 10khz, 100khz, 1Mhz)
- Initially mono-band in 40m.
- Full user customization of the IF & BFOs and more via configuration menus.
- Hot tunning of the parameters when in configuration mode for ease the adjust.

## Features in the TODO list ##

This are the wish list so far, with no particular order.

- Split, we are working on it now.
- XFO for radios with a second conversion radios.
- Memories, with an external EEPROM (24Cxx) on the already existent I2C bus, the amount of the memories will be proportional to the size of the EEPROM and the kind of it will be auto-detected (I2C bus scan) and can be fine tuned via configuration.
- The VFO status is remembered with the power off/on cycles (require the use of an external EEPROM)
- A fast step with the push and rotation of the encoder.
- Multi-band support with a function to jump quickly from band to band.
- CAT control, simulating a professional transceiver by using a protocol for a widely deployed radio model ( Yaesu FT-817ND?)

## Si5351 Clock generator issues ##

There are some founded worries about the use of the Si5351 Clock generator in RF transceiver business, but none of the possible bad point are fully characterized yet (with good technical data in a build transceiver), the most important of the claimed problems are this:

### Crosstalk ###

Some users are pointing that the outputs of the chip is not clean enough for the use in heterodyne transceivers, [here you can see of what we are talking about](http://nt7s.com/2014/12/si5351a-investigations-part-8/), as you can see some of the signal of one output can sneak into the other output, this is a proved bad thing.

Then the question now is: **how bad is it?**

The graphs in the last link shows the worst scenario, you can see that the worst case is only about -35 dB down the main frequency, that will result in about 25mW on the output of a signal with 100W of the fundamental frequency.

With proper measures in design and other tricks (Band Stop filters) you can get that crosstalk down below 50dB and the problem can be minimized at the point to be almost eliminated.

### Square wave output ###

Thanks to Mr. Fourier we know that square waves are the fundamental frequency plus all the harmonics of it... but a signal full of it's harmonics is a bad thing in a VFO or any other place where we need a pure sine signal.

That's why we designed the sketch to always use a real VFO frequency **above** the fundamental RF frequency and all the outputs of the chips will be followed with a matched low pass filter or a band pass filter to get rid of the most of the harmonics to minimize this problem.

### Jitter ###

There are some sources talking about the jitter in the output frequencies, but this author can't find any technical info on the web about how bad is it.

For SSB communications we (humans) can tolerate a jitter of about 2-5 hz without noticing it, and free running LC oscillators can have more than that and we can deal with that, so I think this will not defeat the purpose of this Chip as a frequency generator in the cheap ham market.

## Resume ##

There are some hams that are building transceivers with this chip (Si5351) and posting the results on the web, none of them have reported any of this problems as an impairment for that particular radio.

That's the main reason (and the price of the chip!) of why we chose this and not the Si570 or the AD9850/51 even knowing this chip is not the panacea.

# Si5351 Clock generator issues #

There are some founded worries about the use of the Si5351 Clock generator in transceiver business, and now with more and more builders using it, that problems are beginning to fade away.

This article will grow with experience notes about the problems and our experience about them. For the record, we have so far 5 different setups testing this sketch + hardware.

* BitX 20 _(converted to 40m, IF 8.2158 Mhz from a FT-747GX SSB filter, **CO7WT**)_
* Anritsu transceiver _(Unknown model, IF1 1.6 Mhz, IF2 455 Khz, **CO7LX**)_
* RFT SEG-15 _(WWII transceiver, IF1 28.2 Mhz, IF2 200 Khz, **CO7KD**)_
* Furuno FS1000 _(IF 9 Mhz, **CO7YB**)_
* Homebrew 500Khz singe conversion SSB radio _(Using a SSB filter from a USSR/CCCP "KARAT" transceiver, **CM7MTL**)_

Please note that almost all of this radios are actually kind of Frankenstein's cousins, as they preserve the main RF/IF chain from the mentioned radios, but can have another final stage (homebrewed or borrowed for a commercial unit) as well as chassis/front panels, etc.

## Crosstalk ##

Some users are pointing that the outputs of the chip is not clean enough for the use in heterodyne transceivers, [here you can see of what we are talking about](http://nt7s.com/2014/12/si5351a-investigations-part-8/), as you can see, some of the signal of one output can sneak into the other output, this is a proved bad thing.

Then the question now is: **how bad is it?**

The graphs in the last link shows the worst scenario, you can see that the worst case is only about -35 dB down the main frequency, that will result in about 25mW of the spurious frequency at the output of a main signal with 100W for the fundamental frequency.

With proper measures in design and other tricks (Bandstop/bandpass filters) you can get that crosstalk down below 50dB and the problem can be minimized at the point to be almost eliminated.

### Experience from our side ###

From a technician point of view with laboratory grade equipment, you are gonna find the crosstalk, it's there. From a normal homebrewer with COTS (Cost Off The Shelf) equipment it's not a big deal on reception and it has a no-noticeable impact on your transmission even with 150W in one of our local setups.

## Square wave output ##

Thanks to Mr. Fourier we know that square waves are a pure sine wave of the fundamental frequency plus all the harmonics of it until infinity... but a signal full of it's harmonics is a bad thing in a VFO or any other place where we need a pure sine signal, right?

That's why we designed the sketch to always use a final VFO frequency **above** the fundamental RF frequency and all the outputs of the chips will be followed with a matched low pass or a band pass filter to get rid of the most of the harmonics to minimize this problem.

Reception is affected in a different manner even when the VFO is over the RF; if you had to generate a BFO or a VFO signal below 2Mhz you will get more and more spurious spikes across the spectrum. Eventually at a BFO of 455/500 kHz you will need some filtering in place for a proper operation.

A trick from one of our builders CO7LX: he has a 1rst IF of 1.6 MHz then a second IF of 455 kHz. The reception with the BFO at 455 kHz coupled directly (just a dc blocking capacitor) was really poor. After inspection wit and SDR we noted a lot of spurious spikes and an unusual high noise floor.

As this signal is fixed and if needed just moved a couple of kHz we make a simple filter with two TOKO IF cans coupled back to back to form a crude and yet simple band pass filter and we managed to push down the noise floor and the numerous spurious signals to a very pleasant level.

### Experience from our side ###

The simple fix is to put a low pass filter for the used frequency range, or even better, a band pass filter you will be safe and this will do the job, believe us, this is a "must" not an "if".

## Jitter & phase noise ##

There are some sources talking about the jitter in the output frequencies, but this author can't find any technical info on the web about how bad is it.

For SSB communications we (humans) can tolerate a jitter of about 2-5 hz without noticing it, and free running LC oscillators can have more than that and we can deal with that, so I think this will not defeat the purpose of this Chip as a frequency generator in the cheap ham market.

The code for managing the Si5351 uses a integer division always, with that we get sure you get as low jitter and phase noise as possible.

## Birdies and Background Noise ##

In almost all setups out there in the internet you will find a few users talking about the birdies in the receiver and even about an artificial (high) background noise, well, after experimentation with this setup with a few receiver configurations here in Cuba I can tell you this:

1 - Not all birdies comes from the Si5351, some are Arduino and/or LCD related.

In one of my setups I tested to power off the Arduino and the LCD but leaving the Si5351 powered and generating the frequency for the rest of the receiver chain, I can spot at least two birdies that will go completely off once you erase the Arduino + LCD from the equation, so the Si5351 is not the sole culprit. Also the background noise levels drops.

2 - Yes, you can experiment an artificially (high) background noise, mainly from the Arduino talking with the LCD.

In one of the developing stages, early in the barGraph introduction I suddenly experienced a high background noise, after firing my Yaesu FT-747GX on and check I can confirm that the noise comes from inside the hombrewed radio...

Later investigations revealed that the MCU (Arduino) to LCD communications and the ADC reading generate a lot of noise, high enough to mask some times the weak signals and even get at annoying levels at certain frequencies.

To deal with that this sketch minimizes the writes to the LCD to the least possible and the results are very good compared to the initials with the all time refreshing LCD.

A user on a mailing list commented recently that keeping the Arduino + LCD + Si5351 module in a shielded compartment away from the sensitive RF components can eliminate this "artificially (high) background noise". Local testing made by Soris CO7YV proved that this is a great help to make the radio even quieter.

## Datasheet tells three outputs, but in practive it's a gamble of 2+? ##

The internals of the Si5351 are described in depth in the datasheet and further publications, long story short: the Chip has an external crystal as a reference for two (2) independent PLL VCOs in the VHF to UHF range, then you select a chain of divisors to process that VCOs outputs to get your desired frequency.

The trick is that with three outputs you will have to share two of the outputs with a common PLL VCO frequency and you will have one of them that can be expressed in the correct math to get right on the frequency under certain circumstances.

### Experience from our side ###

In the RFT SEG-15 we test the use of the three frequencies from the sketch, one for the VFO with and independent VCO (29 to 60 Mhz) and then a XFO of 28.0 Mhz and a BFO of 200 Khz sharing the same VCO.

Strange things happened when we start to tune the frequencies out in the radio: the BFO will not move in steps of less than 150 Hz even if you set a step of 1 Hz, further investigations revealed that the library was trading stability (aka: struggle to keep the output active) vs. accuracy of the output.

In this configuration we found that with a 28.0 Mhz and a another less than 1 Mhz output, sharing the same VCO inside the Si5351 we face accuracy problems on the < 1 Mhz output.

So we sacrificed the XFO and go back to use the original crystal XFO inside the radio, taking that into account in the sketch: for the RFT SEG-15 the XFO is taken into account and used on the calculations but not activated in any way.

That lead to the desision of using just two of the 3 outputs of the Chip, almost all radio manufacturers use a real crystal for the fixed conversion from the 1rst to the 2nd IF; please do use it.

## Resume ##

There are many hams that are building transceivers with this chip (Si5351) and posting the results on the web, none of them have reported any of this mentioned problems as an impairment for that particular radio. So far in our experiments, we are confirming this, the Si5351 is suited to the job.

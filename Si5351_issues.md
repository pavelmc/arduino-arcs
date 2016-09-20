## Si5351 Clock generator issues ##

There are some founded worries about the use of the Si5351 Clock generator in RF transceiver business, but none of the possible bad point are fully characterized yet (with good technical data in a build transceiver), the most important of the claimed problems are this:

Note: this article will grow with experience notes from our labor to solve the lack of info, we have now 5 different setups testing this hardware, so far:

* BitX 20 (converted to 40m, IF 8.2158 Mhz from a FT-747GX SSB filter) (CO7WT)
* Anritsu transceiver (unknown model) (IF1 1.6 Mhz, IF2 455 Khz) (CO7LX)
* RTF SEG-15 WWII transceiver (IF1 28.2 Mhz, IF2 200 Khz) (CO7KD)
* Furuno FS1000 (IF 9 Mhz) (CO7YB)
* Homebrew 500Khz singe conversion SSB radio using a SSB filter from a USSR KARAT transceiver. (CM7MTL)

### Crosstalk ###

Some users are pointing that the outputs of the chip is not clean enough for the use in heterodyne transceivers, [here you can see of what we are talking about](http://nt7s.com/2014/12/si5351a-investigations-part-8/), as you can see some of the signal of one output can sneak into the other output, this is a proved bad thing.

Then the question now is: **how bad is it?**

The graphs in the last link shows the worst scenario, you can see that the worst case is only about -35 dB down the main frequency, that will result in about 25mW on the output of a signal with 100W of the fundamental frequency.

With proper measures in design and other tricks (Band Stop filters) you can get that crosstalk down below 50dB and the problem can be minimized at the point to be almost eliminated.

**My measurements on this:** from a technician point of view with laboratory grade equipment you are gonna find the crosstalk, from a homebrewer with normal equipment it's not a big deal and it has a no-noticeable impact on your transmission even with 150W in one of our local setups.

### Square wave output ###

Thanks to Mr. Fourier we know that square waves are the fundamental frequency plus all the harmonics of it... but a signal full of it's harmonics is a bad thing in a VFO or any other place where we need a pure sine signal.

That's why we designed the sketch to always use a real VFO frequency **above** the fundamental RF frequency and all the outputs of the chips will be followed with a matched low pass filter or a band pass filter to get rid of the most of the harmonics to minimize this problem.

**Tip:** Just putting a low pass filter for the used frequency range, or even better, a band pass filter you will be safe. In one of our setups one radio has a BFO at 455 KHz, and the reception was very noisy in deed, the user omitted a low pass filter on this signal for the BFO, a improvised BPF with two TOKO IF Cans and a 1nF condenser was enough to clean the 455 Khz signal en get a clean reception.

### Jitter ###

There are some sources talking about the jitter in the output frequencies, but this author can't find any technical info on the web about how bad is it.

For SSB communications we (humans) can tolerate a jitter of about 2-5 hz without noticing it, and free running LC oscillators can have more than that and we can deal with that, so I think this will not defeat the purpose of this Chip as a frequency generator in the cheap ham market.

## Birdies and Background Noise ##

In almost all setups out there you will find a few users talking about the birdies in the receiver and even about an artificial background noise, well, after experimentation with this setup with a few receiver configurations here in Cuba I can tell you this:

1 - Not all birdies comes from the Si5351, some are Arduino and/or LCD related.

In one of my setups I can power off the Arduino and the LCD but leaving the Si5351 powered on and generating the frequency for the rest of the receiver chain, I can spot at least two birdies that will go completely off once you rest the Arduino + LCD from the equation, so the Si5351 is not the sole culprit.

Also the background noise levels drops (Tip: **don't** use a long cable to connect the arduino to the LCD)

2 - Yes, you can experiment an artificially high background noise, mainly from the Arduino talking with the LCD.

In one of the developing stages, early in the barGraph introduction I suddenly experienced a high background noise, after firing my Yaesu FT-747GX on and check I can confirm that the noise comes from inside the radio...

Later investigations revealed that the MCU (Arduino) to LCD communications and the ADC reading generate a lot of noise, high enough to mask some times the weak signals and get at annoying levels at certain frequencies.

To deal with that this sketch minimizes the writes to the LCD to the least possible and the results are good compared to the initial all time refreshing LCD.

## Resume ##

There are some hams that are building transceivers with this chip (Si5351) and posting the results on the web, none of them have reported any of this problems as an impairment for that particular radio.

So far in out experiments we are confirming this, the Si5351 is suited to the job.

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

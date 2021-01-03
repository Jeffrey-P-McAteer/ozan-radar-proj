
# Unnamed Project

Goal: map depth using piezoelectric crystals responsible for generating + receiving audio waves.

 - Include delay offset adjustment argument
 - Include several different waveforms + frequency arg + amplitude arg?

# Status

 - [x] List audio devices
 - [ ] Record mic input
 - [ ] Send ping out via speaker
 - [ ] Show waves/decode useful signals from mic
 - [ ] Add aux. arguments: wave type (sine,square,sawtooth) frequency, amplitude, distance offset


# Dev. Req.

`libasound2-dev`

# Building

```bash
make pingit && ./pingit --help
```

# Usage

```bash
# Figure out your mic + speaker names (we use ALSA)
./pingit list-hw

# Send a square-wave ping every 1 second and print computed distance
# as we receive audio. This example adds a distance correction
# of 4 meters.
./pingit ping -d 1000 


```




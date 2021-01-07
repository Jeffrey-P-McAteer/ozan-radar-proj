
# Unnamed Project

Goal: map depth using piezoelectric crystals responsible for generating + receiving audio waves.

 - Include delay offset adjustment argument
 - Include several different waveforms + frequency arg + amplitude arg?

# Status

 - [x] List audio devices
 - [x] Record mic input
 - [x] Send ping out via speaker
 - [ ] Show waves/decode useful signals from mic (aka distance calculation)
 - [ ] Add aux. arguments: wave type (sine,square,sawtooth) frequency, amplitude, distance offset

# Misc. commands

The human mind can recognize 100x faster than it can recall, so here you go:

```bash
git add -A .
git commit -a -m "Some message"
git push
git pull

ssh pi

# Where "shared" is the name of the session
tmux new-session -s shared
tmux attach-session -t shared

```

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

# Watch for new HW as it is plugged in + print the names
./pingit detect-hw

# Send a sound wave every 2 seconds to "sysdefault:CARD=PCH".
# record from "default" microphone, and if the ambient noise is > 3x what is was
# detect that as a response sound wave.
./pingit ping -d 2000 -i default -o sysdefault:CARD=PCH -s 3.0 -w /tmp/stuff.raw


```




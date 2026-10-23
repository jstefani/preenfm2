# preenfm2 firmware

Ixox/preenfm2 is the official repository of the preenfm2 firmware.

You can find here the compiled firmware, its code source and some hardware files for the PCB, MCU board and cases.

To flash the preenfm2 for the first time, [follow these instructions](https://github.com/Ixox/preenfm2/tree/master/flash)


## Compiling the firmware

To compile the firmware, you'll need [arm-gcc version 4.7](https://launchpad.net/gcc-arm-embedded/+milestone/4.7-2014-q2-update)

Add the bin directory to your PATH, and run **'make'**, you'll get the list of the available targets.

```bash
$ make
You must chose a target 
Don't forget to clean between different build targets
   clean : clean build directory
   pfm : build pfm2 firmware
   pfmcv : build pfm2 firmware for Eurorack 
   installdfu : flash last compiled firmware through DFU
   zip : create zip with all inside
```
Since some refactoring, the bootloader does not compile anymore. But it's available in its binary format.

Then put your preenfm2 in [bootloader mode](http://ixox.fr/preenfm2/manual/upgrade-firmware/). Look at DFU part 4.

To flash the firmware on the preenfm2 using the DFU protocol :

```bash
make installdfu
```

Once it's done, unplug the power cable and plug it back.

## DX7 engine (branch dx7-msfa-engine)

DX7 sysex patches can be played by a port of the msfa engine used by Dexed
(see `src/msfa/README.md`) instead of being converted to a preenfm2 patch.

* **Settings > DX7 Engine**: `Native` (default) plays DX7 banks on the DX7
  engine, `Convert` keeps the historical hexter conversion.
* The engine is chosen per timbre by the patch loaded into it: a DX7 bank
  patch runs on the DX7 engine, a preenfm2 patch runs on the preenfm2 engine.
  Existing preenfm2 patches are unaffected.
* Load > DX7: the first three banks are virtual. Selecting one generates a
  new bank of 32 random DX7 patches; browse them like any bank. `*Random*`
  stays in musical ranges (name shows the character: PRC, KEY, PAD, LED),
  `*Chaos*` draws every parameter over its full range (expect gems and
  duds), `*ChaosPrc*` is chaos with percussive envelopes. MIDI bank numbers
  still address the files.
* Up to 8 DX7 voices are shared by all timbres. Pitch bend, mod wheel
  (vibrato), hold pedal, the arpeggiator and the per-timbre filter/gain/pan
  stage work.
* Editing: on a DX7 timbre the edit screen shows DX7 rows. Synth button:
  Algo / Feedback / OscSync / Voices. Osc button: cycles operators 1-6; Env
  button: cycles the operator views Freq (mode, coarse, fine, detune), Level
  (output level, velocity, AMS, rate scaling), Rates, Levels, KeyScale, Curves.
  Matrix button: pitch EG rates / levels. LFO button: speed, delay, PMD, AMD /
  sync, wave, PMS, transpose. Playing notes follow the edits.
* Saving: Save > DX7 writes the timbre's DX7 voice into a slot of a `.syx`
  bank on the key (standard 32 voice format, checksum updated, readable by
  Dexed and a real DX7), after a 10 character name. Tools > Crea > DX7 creates
  a new bank of INIT VOICEs. Save > Bnk still stores the converted preenfm2
  patch. Combos only store preenfm2 patches.

## New Filters in 2.11

Many effects have been added in the firmware 2.11. They were coded by [Toltekradiation](http://ixox.fr/forum/index.php?topic=69544.0).

His github repo is [here](https://github.com/pvig/preenfm2). You'll find there some description of the different effects.

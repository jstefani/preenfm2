# preenfm2 firmware 2.30

This is a maintained continuation of the [preenfm2](https://github.com/Ixox/preenfm2) firmware by Xavier Hosxe, starting at version 2.30 and based on the official 2.21b. It runs on the same hardware: flash it like any official firmware, and go back to the official one the same way.

Fixes that also make sense for the official firmware are offered upstream as pull requests.

Binaries are attached to the [releases](https://github.com/jstefani/preenfm2/releases). The official firmware, hardware files and documentation are at [Ixox/preenfm2](https://github.com/Ixox/preenfm2) and [ixox.fr](http://ixox.fr/preenfm2/).

To flash the preenfm2 for the first time, [follow these instructions](flash).

## What's new in 2.30

* **DX7 engine.** DX7 sysex banks play on a port of the msfa engine used by Dexed, so they sound like the original instead of being converted. See [DX7 engine](#dx7-engine) below.
* **DX7 editing and saving** from the front panel, to standard `.syx` banks.
* **Random DX7 banks**: `*Random*`, `*Chaos*` and `*ChaosPrc*` at the top of Load > DX7.
* **No click on patch change**: the output is muted for a few ms around a preset load.
* **Screen saver on menu screens**, and the control that wakes the screen is not acted on.
* **Builds with a current arm-none-eabi-gcc** (10.x).

### Known limitations

* Combos only store preenfm2 patches: a part running the DX7 engine is reloaded from a combo as the converted preenfm2 patch.
* Save > Bnk on a DX7 part stores the converted preenfm2 patch; use Save > DX7 to keep the DX7 voice.
* The Eurorack (CV) build compiles but has not been tested on Eurorack hardware.

### Upgrading from 2.21b

* `Settings.txt` gains a `dx7engine` line (0 = Native, 1 = Convert). It is written the next time the settings are saved; without it the default, Native, applies.
* Load > DX7 lists the three virtual random banks before the banks on the key. MIDI bank select still addresses the files as before.

## Compiling the firmware

You need the [Arm GNU toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) (`arm-none-eabi-gcc`, tested with 10.3; on macOS `brew install --cask gcc-arm-embedded` or the `arm-gcc-bin@10` formula). The Makefile uses the `arm-none-eabi-gcc` found on your `PATH`; override it with `make GCC_PATH=/path/to/bin ...`.

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

`make pfm` produces `build/p2_<version>.bin`. Copy it to `/pfm2` on the USB key and flash it from the bootloader (power on while holding a button), or put the preenfm2 in [DFU mode](http://ixox.fr/preenfm2/manual/upgrade-firmware/) and run `make installdfu`.

The bootloader is available in binary form in [flash](flash).

## DX7 engine

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

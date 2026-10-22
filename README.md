
OpenTracker is an open-source, clean-room firmware for the **original Polyend Tracker**,
written from behavioural specifications. It contains no Polyend code, fonts, icons or
graphics, and is not affiliated with or endorsed by Polyend.

## Screenshots (2.0.1, on an original Tracker)

| Pattern | Config: USB Storage |
|---|---|
| ![Pattern](screenshots/2.0.1/pattern.png) | ![Config, USB Storage](screenshots/2.0.1/config-usb-storage.png) |
| **Sample Recorder** | **Synth Parameters: Load Patch** |
| ![Sample Recorder](screenshots/2.0.1/sample-recorder.png) | ![Synth Load Patch](screenshots/2.0.1/synth-load-patch.png) |


2.0 brings the newer Tracker features to the original hardware: 16 tracks, synth
engines, stereo samples, Mini-compatible project files and USB Storage mode. These notes cover
the whole 2.x line 

## New in 2.0

### Tracker Mini project compatibility
- Projects are saved in the **Tracker 2.2.1 format**: 16-track patterns, the three
  synth slots, track names and mutes for tracks 13–16.
- **Synth patches** use the Mini's `.patch` files. They are stored with the project and in a
  library at `Patches/<ENGINE>/`. All 299 Mini factory patches load and save back unchanged.
- Older original-Tracker projects open as they are, with tracks 13–16 empty.

### 16 tracks
- Tracks 13–16 work like MIDI tracks 9–12: notes, MIDI channels, CC A–F, pitch bend,
  velocity, mute and solo. Performance Play mode, Pattern length and Step repeater act on them
  too (no voice FX, levels or per-track patterns, as on the Mini).
- Pattern pages through all 16 tracks. Song shows a scrolling 12-track window.
- Fill, copy and paste work across all 16 tracks.

### Synth engines (instruments S1–S3)
- **Four synth slots**, played from any track as instruments S1–S4.
- Three engines, our own sound design following the Mini's parameter sets:
  - **ACD**: acid bass and lead, with a 24 dB and a ladder-style filter, and glide;
  - **VAP**: virtual-analog poly, with morphing oscillators, hard sync, 15 filter types and a
    6-slot modulation matrix;
  - **PERC**: drum kit with kick, snare, hats, cymbal, perc and tom.
  - **FAT**: 3-osc virtual analog synth engine with detune control.
- **WTFM** patches load and save unchanged, but stay silent and show
  "not available" until 2.1.
- **Macros:** FX columns A–F drive macros 1–6, and Volume drives macro 7.
- **Voices:** a shared pool of 4 voices by default, adjustable in Config > General > Synth
  voices (1–8).
- **Screens, after the Mini's layout:**
  - Synth Parameters: model, polyphony, gain, pan, sends.
  - Synth Macros.
  - Edit Patch: sections on the pads; Save, Save As, Init.
  - Load Patch popup with Preview.
  - Synth 1–3 level columns with meters on Global Mixer page 3.
- Exports include separate stems `synth_1..3.wav`.

### Stereo samples
- Stereo WAVs keep both channels on import.
- They play in stereo in one-shot, loop, slice and beat-slice modes. Pan works as balance.

### Performance and monitoring
- **CPU meter** in the title bar (Config > General > CPU meter: Auto, Always or Off).
- **Voice limiter:** if the audio gets close to its limit, only synth voices are cut; sample
  tracks never are.
- **Faster loading:** project loading does about half the work in our host model (not yet
  timed on the device).
- **Smoother display:** frames go to the screen over DMA without waiting.
- **More code in fast internal RAM:** the hottest audio and synth code runs from it.
- **USB serial diagnostics:** CPU, peak breakdown, display, SD and radio/recorder lines.

### USB Storage mode (2.0.1)
- **Config > USB > USB Storage: On** shows the SD card on a connected computer as a USB drive,
  as on the Tracker Mini. Copy projects, samples and patches without taking the card out.
- Playback stops and the project is saved first. Keep the power and the USB cable connected
  while files are transferred.
- To finish: eject the drive on the computer, then press **Exit** (or set the row back to
  **Off**). The project reloads from the card. If the computer still uses the drive, Exit
  first reminds you to eject it.
- The setting is always **Off** after power-on.
- About 1 MB/s (full-speed USB; measured 1,033 KB/s on a real Tracker). Tested with Linux
  and macOS; Linux can take a few seconds longer to notice the drive. USB MIDI and the
  serial log keep working.
- If the cable is pulled during a transfer, the Tracker recovers within half a second.
- The drive has a stable serial number, so a computer keeps recognising the same Tracker.
  On Windows, the Tracker may appear once as a new device (new COM/MIDI port).

## Install

1. Copy `PolyendTrackerOTv201.ptf` to `/Firmware` on the SD card. Keep the name; the
   bootloader only accepts 24-character names.
2. Go to **Config > Firmware update**, select it and confirm. If the screen stays dark
   after the update, press the power button once.

**Going back:** keep the stock `PolyendTracker_1.9.2.ptf` in `/Firmware`. Install it the
same way, or put its file name into `/Firmware/ot_restore.txt` and power-cycle. Keep only the
image you want, plus stock, in `/Firmware`: the rear reset button installs a random `.ptf`
from that folder.

**Note:** projects saved by 2.0 use the Mini's 16-track patterns. Stock firmware for the
original Tracker reads only tracks 1–8 of those patterns (expected from the stock loader's
rules; not yet confirmed on a stock unit).

## Known limitations

- The synth sound is own design: settings match the Mini, the sound is close but not
  identical.
- WTFM engine: planned for 2.1.x 
- The synth voice pool defaults to 4 because of the processor's speed.
- USB Storage mode: back up the card before first use; Linux systems without a desktop may need
  `partprobe` or a re-plug to see the card.
- The USB ID is the pid.codes test ID 1209:0001.

## Test songs

The package's `Projects/` folder holds five OT Test songs and seven BLAST GRID synth remixes
(Title, World 1, World 4, Battle, Battle FM, Boss, Final Boss). Copy them to `/Projects` on
the card.

Downloads: https://github.com/sandlbn/polyend-tracker-re/releases

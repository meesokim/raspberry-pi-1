# SPC-1000 Emulator Porting to Raspberry Pi 1 Bare-Metal (Circle OS)

This project contains the bare-metal port of the SPC-1000 emulator for Raspberry Pi 1 (ARM1176JZF-S) using the Circle bare-metal environment and custom SDL2 wrapper libraries.

## 🚀 Key Improvements & Achievements

### 1. Sound output & Volume Normalization
- **Direct SDL Audio Pipeline**: Bypassed the unstable `SDL_mixer` subsystem to directly open the hardware audio channels using raw `SDL_OpenAudioDevice` (via `SDL_INIT_AUDIO` subsystem initialization), ensuring stable bare-metal audio stream synthesis.
- **Lazy Initialization for PSG Wrapper**: Fixed a critical Circle compiler initialization timing issue where static/global constructors ran before memory controllers were active (leading to `NULL` wrapper pointers). Implemented lazy initialization (`if (!psg) init()`) for all wrapper functions inside `ay8910.h`.
- **Upmixing Mono to Stereo**: Correctly mapped the mono PSG channel data into the Pi's stereo HDMI/headphone channels.

### 2. Eliminating Keyboard Input Lag & Resolving Boot Freeze
- **Frame-based V-Blank Interrupts**: Replaced continuous level-triggered interrupt assertions (`assert_irq`) with a clean `pulse_irq(0xFF)` once per frame (60Hz). This resolved the "interrupt storm" which flooded the Z80 CPU, returning the emulator from a 10% speed crawl back to 100% full emulation speed.
- **Stable Input Event Loop**: Reverted the event loop to the original `if (SDL_PollEvent(&event))` polling mechanism. Since the Z80 is now running at full speed, this provides lag-free responsiveness while preventing SD card driver/USB queue collisions on bare-metal.
- **Logging Optimization**: Removed all synchronous SD card write logs (`fopen`, `fprintf`, `fclose`) from high-frequency emulated Z80 I/O and audio loop regions, preventing boot freezes and maintaining full performance.

### 3. Absolute Path Cassette Tape Loading
- Modified the relative tape-loading file paths in `cassette.cpp` to resolve fully-qualified absolute paths using the global folder directory prefix (`sd:/taps/`), enabling smooth tape game load actions (`LOAD` command) directly from the SD card.

---

## 🛠️ Compilation

To compile and produce the final bootable bare-metal image:

```bash
make
```

This compiles the codebase into `kernel.elf` and extracts the final bootable binary `kernel.img`. Copy `kernel.img` onto the root directory of your Raspberry Pi SD card along with the firmware files (`bootcode.bin`, `start.elf`) to boot.

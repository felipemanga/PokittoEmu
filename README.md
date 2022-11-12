# PokittoEmu

[![Linux x86_64](https://github.com/felipemanga/PokittoEmu/actions/workflows/c-cpp-linux-x86_64.yml/badge.svg)](https://github.com/felipemanga/PokittoEmu/actions/workflows/c-cpp-linux-x86_64.yml) [![Emscripten](https://github.com/felipemanga/PokittoEmu/actions/workflows/c-cpp-emscripten.yml/badge.svg)](https://github.com/felipemanga/PokittoEmu/actions/workflows/c-cpp-emscripten.yml) [![Windows i686](https://github.com/felipemanga/PokittoEmu/actions/workflows/c-cpp-windows-i686.yml/badge.svg)](https://github.com/felipemanga/PokittoEmu/actions/workflows/c-cpp-windows-i686.yml) [![MacOS x86_64](https://github.com/felipemanga/PokittoEmu/actions/workflows/c-cpp-macos-x86_64.yml/badge.svg)](https://github.com/felipemanga/PokittoEmu/actions/workflows/c-cpp-macos-x86_64.yml)

An emulator for the [Pokitto](https://www.pokitto.com/) DIY handheld game system.

The development library can be found at the [PokittoLib](https://github.com/pixelbath/PokittoLib) repository.

## Usage
To run a game directly, pass it as the first command line parameter:

```
PokittoEmu path/to/jetpack.bin
```

...or on Windows:

```
PokittoEmu.exe path\to\jetpack.bin
```

Note: Relative or absolute paths are both supported. The above syntax also works for `.pop` files.


## Emulator Controls
| Key        | Function                 |
| ---------- | ------------------------ |
| Esc        | Quit PokittoEmu          |
| F2         | Save a screenshot as PNG |
| F3         | Toggle screen recording  |
| F5         | Restart                  |
| F8         | Pause/resume emulation   |
| Arrow keys | D-pad directions         |
| A          | A button                 |
| S/B        | B button                 |
| D/C        | C button                 |
| F          | Flash button             |

Screenshots and recorded GIFs will be saved in the same folder as the currently-running game.

## CLI Options
All switches listed below are optional.

| Option     | Description         |
|------------|---------------------|
| `-A rate` | Set audio sample rate to `rate` |
| `-r` | Automatically record the screen |
| `-V` | Enable verbose logging |
| `-w [0|1]` | Set "ignore bad writes" setting (passing without a number defaults to `1`) |
| `-W` | Toggle "ignore bad writes" setting |
| `-e addr` | Sets entry point to `addr` |
| `-s frames` | Take a screenshot after `frames` frames |
| `-g` | Set debugger port number to `1234` |
| `-G port` | Set debugger port number to `port` |
| `-I image.img` | Load SD card image from `image.img` |
| `-o` | Set output image path to `out.img` |
| `-O image.img` | Write SD card image to `image.img` |
| `-p` | Enable profiler |
| `-P` | Enable profiler and try to guess hot functions |
| `-v` | (Linux only) Enable verifier |
| `-x` | Set PEX port to `2000` |
| `-X port` | Set PEX port to `port` |


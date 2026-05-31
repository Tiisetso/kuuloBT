# kuuloBT

A C program to connect Bluetooth audio devices to Linux based iMacs at Hive. 

Uses the D-Bus API (BlueZ) for Bluetooth management and PulseAudio's C API for audio routing.

This software was almost entirely programmed using LLM agents.

## Build

```bash
make
```

## Usage

```bash
./kuulobt
```
If you wish to install the program for regular use. Copy or the binary to your .local/bin
```bash
cp kuulobt ~/.local/bin/
chmod +x ~/.local/bin/kuulobt
```
To run the application
```bash
kuulobt
```

## Interactive Device Selector

When scanning or connecting, use:
- **↑ / ↓** — Navigate the device list
- **Enter** — Connect to the selected device
- **q / Esc** — Cancel

## Options

| Flag     | Description                              |
|----------|------------------------------------------|
| `-m MAC` | Specify device MAC address               |
| `-v`     | Verbose output                           |
| `-t SEC` | Scan duration in seconds (default: 10)   |

## How It Works

- **Bluetooth**: Communicates directly with BlueZ via the system D-Bus — no subprocesses needed for pairing/connecting
- **Audio**: Uses PulseAudio's C API to discover Bluetooth sinks, set the A2DP profile, and route audio
- **Selector**: Raw terminal input (termios) for responsive arrow-key navigation
- **Diagnostics**: Checks adapter status, audio modules, ALSA cards, PulseAudio sinks, and user config
- **Fix**: Restarts PulseAudio, reloads Bluetooth modules, and writes optimized config — all user-level, no sudo

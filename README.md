# 🎧 kuuloBT

A C program to connect Bluetooth audio devices to Linux without sudo.

Uses the D-Bus API (BlueZ) for Bluetooth management and PulseAudio's C API for audio routing.

## Build

```bash
make
```

**Dependencies** (must be installed): `libdbus-1-dev`, `libpulse-dev`

## Usage

```bash
./kuulobt help          # Show all commands
./kuulobt scan          # Scan for devices, select with arrow keys
./kuulobt connect       # Interactive device selector
./kuulobt connect -m XX:XX:XX:XX:XX:XX   # Connect by MAC address
./kuulobt disconnect    # Disconnect current device
./kuulobt status        # Show current connection & audio status
./kuulobt diagnose      # Full system diagnostics
./kuulobt fix           # Attempt automatic audio fixes
./kuulobt reset         # Remove pairing and start fresh
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

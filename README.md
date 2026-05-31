# 🎧 blueconnect

A C program to connect Apple AirPods Pro to Ubuntu Linux without sudo.

Uses the D-Bus API (BlueZ) for Bluetooth management and PulseAudio's C API for audio routing.

## Build

```bash
make
```

**Dependencies** (must be installed): `libdbus-1-dev`, `libpulse-dev`

## Usage

```bash
./blueconnect help          # Show all commands
./blueconnect scan          # Scan for nearby Bluetooth devices
./blueconnect connect       # Auto-detect and connect AirPods
./blueconnect connect -m XX:XX:XX:XX:XX:XX   # Connect by MAC address
./blueconnect disconnect    # Disconnect AirPods
./blueconnect status        # Show current connection & audio status
./blueconnect diagnose      # Full system diagnostics
./blueconnect fix           # Attempt automatic audio fixes
./blueconnect reset         # Remove pairing and start fresh
```

## Typical Workflow

1. Open AirPods case near the computer, hold the button on the back
2. `./blueconnect scan` — find the AirPods MAC address
3. `./blueconnect connect -m XX:XX:XX:XX:XX:XX` — connect
4. If no audio, run `./blueconnect diagnose` then `./blueconnect fix`

## Options

| Flag     | Description                              |
|----------|------------------------------------------|
| `-m MAC` | Specify device MAC address               |
| `-v`     | Verbose output                           |
| `-t SEC` | Scan duration in seconds (default: 10)   |

## How It Works

- **Bluetooth**: Communicates directly with BlueZ via the system D-Bus, no `bluetoothctl` subprocess needed for pairing/connecting
- **Audio**: Uses PulseAudio's C API to discover Bluetooth sinks, set the A2DP profile, and route audio
- **Diagnostics**: Checks adapter status, audio modules, ALSA cards, PulseAudio sinks, and user config
- **Fix**: Restarts PulseAudio, reloads Bluetooth modules, and writes optimized config — all user-level, no sudo

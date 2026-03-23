# GW1 Map Tracker Relay

A WebSocket relay server for the GW1 Map Tracker project. This server sits between the GWToolbox++ plugin (which reads player position from Guild Wars 1) and the web-based map viewer.

## ⚠️ WARNING: COMPLETELY UNTESTED

This project has not been tested at all. It was built as a proof of concept and may not work. Use at your own risk. Expect bugs, crashes, and broken functionality.

## How It Works

1. The GWToolbox++ module connects as a `plugin` and sends player position data
2. The relay server forwards that data to any connected `viewer` clients
3. The web map viewer connects as a `viewer` and displays the position on thatshaman's GW1 interactive map

## Setup

### Relay Server (this repo)
- Deploy to Render, Railway, or any Node.js host
- Free tier works fine
- No environment variables needed

### GWToolbox++ Module
- Built into GWToolboxdll
- Find "Map Tracker" in GWToolbox settings
- Enter your relay host URL and session key
- Enable tracking

### Web Viewer
- Open the map viewer in your browser
- Connect using the same session key

## API

Clients connect via WebSocket with query parameters:
- `?key=YOUR_SESSION_KEY&role=plugin` — for the GWToolbox sender
- `?key=YOUR_SESSION_KEY&role=viewer` — for the web map viewer

## Position Data Format
```json
{"type":"position","x":1234.56,"y":7890.12,"map":18}
```

## Dependencies

- [ws](https://github.com/websockets/ws) — WebSocket library for Node.js

## Status

🔴 **Untested** — This has never been run end-to-end. The GWToolbox module compiles successfully but has not been loaded into a live game. The web viewer has not been tested against a live relay. Everything could be broken.

## License

MIT

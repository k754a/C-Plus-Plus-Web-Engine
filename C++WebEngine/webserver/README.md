# C++Browse — Web Server Package

This is the complete web version of the C++Browse browser engine, ready to deploy
on a Linux server. It replicates the Windows SDL3 desktop browser UI in a web
frontend, powered by the same C++ parsing/layout engine (ported to output JSON).

## What's included

```
webserver/
├── engine/          C++ engine source + Makefile (outputs JSON layout)
├── service/         socket.io mini-service (port 3003, spawns engine per request)
├── gateway/         Node.js reverse-proxy gateway (port 8080, public)
├── frontend/        Next.js browser UI (port 3000, pixel-font icons)
├── build.sh         Builds everything (C++ + Node deps + Next.js)
├── run.sh           Starts all 3 services
├── stop.sh          Stops all services
└── logs/            Runtime logs
```

## Prerequisites

Install these on your Linux server:

```bash
# Debian/Ubuntu:
sudo apt install g++ make curl nodejs npm

# Or with NodeSource for a newer Node.js (v18+ required):
curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -
sudo apt install g++ make curl nodejs
```

That's it — **no Caddy, no Redis, no database**. Just `g++`, `make`, `curl`, and `node`/`npm`.

## Build & Run (2 commands)

```bash
./build.sh    # Compiles the C++ engine, installs Node deps, builds Next.js
./run.sh      # Starts the gateway (8080), frontend (3000), and engine service (3003)
```

Then open in your browser:

```
http://YOUR_SERVER_IP:8080/
```

To stop: `./stop.sh`

## How it works

1. **Gateway** (port 8080) — A Node.js reverse proxy. Routes requests with
   `?XTransformPort=3003` to the engine service, everything else to the frontend.
   This lets the frontend's socket.io client reach the engine through a single
   public port.

2. **Frontend** (port 3000) — A Next.js app that replicates the Windows SDL3
   browser UI (tab bar, nav bar with pixel-font icons, content area with custom
   scrollbar). Connects to the engine via socket.io.

3. **Engine service** (port 3003) — A socket.io server that spawns one short-lived
   C++ engine process per navigation. The engine fetches the URL (curl for HTTPS,
   raw sockets for HTTP), parses HTML/CSS, builds the DOM + layout tree, and
   outputs JSON. This scales to many users — processes are per-request, not
   per-user.

4. **Bookmarks** — Stored in `engine/starred_pages.STAR` (same format as the
   Windows version). Shared across all users.

## Architecture notes

- **Multi-user safe**: Each navigation spawns a brief C++ process (1–5 seconds)
  that exits after returning JSON. 10 idle users ≈ negligible memory. 3
  simultaneous page loads ≈ 3 short ~30MB processes.
- **Font loading**: The modified `PixelifySans-edited.ttf` files were converted
  from CFF/OTTO to TrueType (glyf) outlines so Chromium accepts them. The font
  maps Cyrillic/Greek codepoints (ђ њ љ ж ξ ы) to pixel-art icons used in the
  nav bar. If the font doesn't load, icons render as tofu boxes.
- **No Windows engine changes**: Only the web version was modified. All changes
  are marked with `// CHANGED WITH AI` comments in the source.

## Production deployment (systemd)

For a permanent setup, create systemd services. Example for the gateway:

```ini
# /etc/systemd/system/cppbrowse-gateway.service
[Unit]
Description=C++Browse Gateway (port 8080)
After=network.target

[Service]
Type=simple
WorkingDirectory=/path/to/webserver/gateway
ExecStart=/usr/bin/node gateway.js
Restart=always
User=www-data

[Install]
WantedBy=multi-user.target
```

Create similar units for the service (`WorkingDirectory=.../service`, `ExecStart=node server.js`)
and frontend (`WorkingDirectory=.../frontend`, `ExecStart=npx next start -p 3000`).

## Troubleshooting

- **Page loads but stays on "Loading..."**: Make sure you're accessing port 8080
  (the gateway), NOT port 3000 directly. The gateway routes the socket.io
  connection to the engine service; port 3000 alone can't reach it.
- **Icons show as letters/boxes**: The pixel font failed to load. Check
  `frontend/public/fonts/` contains `PixelifySans-edited.ttf` (TrueType, not CFF).
- **Navigation fails**: Check `logs/service.log`. The engine needs `curl`
  installed for HTTPS. Verify `engine/engine` binary exists (run `./build.sh`).
- **Port 8080 in use**: Edit `gateway/gateway.js` and change `GATEWAY_PORT`.

#!/bin/bash
# CHANGED WITH AI: Stop script — kills all C++Browse services.
cd "$(dirname "$0")"

for name in service frontend gateway; do
  pidfile="logs/${name}.pid"
  if [ -f "$pidfile" ]; then
    pid=$(cat "$pidfile")
    if kill -0 "$pid" 2>/dev/null; then
      kill "$pid" 2>/dev/null
      echo "Stopped $name (pid $pid)"
    fi
    rm -f "$pidfile"
  fi
done

# Also kill any stragglers by command pattern
pkill -f "node server.js" 2>/dev/null || true
pkill -f "node gateway.js" 2>/dev/null || true
pkill -f "next start" 2>/dev/null || true

echo "All services stopped."

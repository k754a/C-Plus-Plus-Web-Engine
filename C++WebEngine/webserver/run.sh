#!/bin/bash
# CHANGED WITH AI: Run script for the C++Browse web server package.
# Starts the engine service (3003), the Next.js frontend (3000), and the
# gateway (8080 — publicly accessible).
cd "$(dirname "$0")"

# Stop any existing instances
./stop.sh 2>/dev/null || true
sleep 1

mkdir -p logs

echo "Starting engine service (port 3003)..."
cd service
node server.js > ../logs/service.log 2>&1 &
echo $! > ../logs/service.pid
cd ..

echo "Starting Next.js frontend (port 3000)..."
cd frontend
npm run start > ../logs/frontend.log 2>&1 &
echo $! > ../logs/frontend.pid
cd ..

echo "Starting gateway (port 8080)..."
cd gateway
node gateway.js > ../logs/gateway.log 2>&1 &
echo $! > ../logs/gateway.pid
cd ..

sleep 3

echo ""
echo "============================================"
echo "  C++Browse is running!"
echo "============================================"
echo ""
echo "  Open:  http://YOUR_SERVER_IP:8080/"
echo ""
echo "  Logs:   logs/service.log   (engine service)"
echo "          logs/frontend.log  (Next.js)"
echo "          logs/gateway.log   (gateway)"
echo ""
echo "  Stop:   ./stop.sh"
echo "============================================"

#!/bin/bash
# CHANGED WITH AI: Build script for the C++Browse web server package.
# Builds the C++ engine and the Next.js frontend, installs all Node.js dependencies.
set -e
cd "$(dirname "$0")"

echo "============================================"
echo "  C++Browse — Building all components"
echo "============================================"

echo ""
echo "[1/4] Building C++ engine..."
cd engine
make
cd ..
echo "  ✓ Engine binary built: engine/engine"

echo ""
echo "[2/4] Installing gateway dependencies..."
cd gateway
npm install --silent
cd ..
echo "  ✓ Gateway ready"

echo ""
echo "[3/4] Installing engine service dependencies..."
cd service
npm install --silent
cd ..
echo "  ✓ Service ready"

echo ""
echo "[4/4] Building Next.js frontend..."
cd frontend
npm install --silent
npm run build
cd ..
echo "  ✓ Frontend built"

echo ""
echo "============================================"
echo "  Build complete!"
echo "============================================"
echo ""
echo "Run ./run.sh to start the server on port 8080."

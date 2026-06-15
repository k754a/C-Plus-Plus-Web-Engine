// CHANGED WITH AI: Added Node.js wrapper to host the C++ engine on the web
const http = require('http');
const fs = require('fs');
const path = require('path');
const { spawn } = require('child_process');
const { Server } = require('ws');



const server = http.createServer((req, res) => {
    if (req.url === '/' || req.url === '/index.html') {
        fs.readFile(path.join(__dirname, 'index.html'), (err, data) => {
            if (err) {
                res.writeHead(500);
                res.end('Error loading index.html');
                return;
            }
            res.writeHead(200, { 'Content-Type': 'text/html' });
            res.end(data);
        });
    } else {
        res.writeHead(404);
        res.end();
    }
});

const wss = new Server({ server });

wss.on('error', (e) => {
    // Ignore wss errors during port scanning, server.on('error') handles it
});

wss.on('connection', (ws) => {
    console.log('Client connected');

    // Choose executable based on platform
    const exeName = process.platform === 'win32' ? 'x64/Release/ConsoleApplication1.exe' : './engine';
    const enginePath = path.join(__dirname, exeName);
    
    let engineProcess;
    try {
        engineProcess = spawn(enginePath);
    } catch(e) {
        ws.send('Error starting engine. Is it compiled? (' + enginePath + ')\n');
        return;
    }

    engineProcess.stdout.on('data', (data) => {
        if (ws.readyState === 1) { // 1 is OPEN
            ws.send(data.toString());
        }
    });

    engineProcess.stderr.on('data', (data) => {
        if (ws.readyState === 1) {
            ws.send(`ERROR: ${data.toString()}`);
        }
    });

    engineProcess.on('error', (err) => {
        if (ws.readyState === 1) {
            ws.send(`Failed to start subprocess: ${err}\n`);
        }
    });

    engineProcess.on('close', (code) => {
        if (ws.readyState === 1) {
            ws.send(`\nEngine exited with code ${code}`);
            ws.close();
        }
    });

    ws.on('message', (message) => {
        if (engineProcess) {
            engineProcess.stdin.write(message + '\n');
        }
    });

    ws.on('close', () => {
        console.log('Client disconnected');
        if (engineProcess) {
            engineProcess.kill();
        }
    });
});

let PORT = process.env.PORT || 8080;

server.on('error', (e) => {
    if (e.code === 'EADDRINUSE') {
        console.log(`Port ${PORT} is in use, trying ${PORT + 1}...`);
        PORT++;
        server.listen(PORT);
    } else {
        console.error(e);
    }
});

server.listen(PORT, () => {
    console.log(`Server listening on http://localhost:${PORT}`);
    console.log(`[Nest Info] If you are on a shared server, use the port above to connect!`);
});

// CHANGED WITH AI: Node.js + socket.io mini-service for the C++Browse web port.
// Spawns one SHORT-LIVED engine process per navigation, collects its JSON output,
// and sends it back. Scales for a shared server — no persistent process per user.
const { createServer } = require('http');
const fs = require('fs');
const path = require('path');
const { spawn } = require('child_process');
const { Server } = require('socket.io');

// CHANGED WITH AI: paths adjusted for the webserver/ package structure.
// engine/ is a sibling of service/, and starred_pages.STAR lives in engine/.
const ENGINE_DIR = path.join(__dirname, '..', 'engine');
const ENGINE_PATH = path.join(ENGINE_DIR, 'engine');
const STARRED_PATH = path.join(ENGINE_DIR, 'starred_pages.STAR');

const httpServer = createServer();

const io = new Server(httpServer, {
  path: '/',
  cors: {
    origin: '*',
    methods: ['GET', 'POST'],
  },
  pingTimeout: 60000,
  pingInterval: 25000,
});

// CHANGED WITH AI: read the shared starred_pages.STAR file.
function readBookmarks() {
  const out = [];
  try {
    const data = fs.readFileSync(STARRED_PATH, 'utf8');
    for (const line of data.split('\n')) {
      const trimmed = line.replace(/\r$/, '').trim();
      if (trimmed) out.push(trimmed);
    }
  } catch (e) {
    // file may not exist yet
  }
  return out;
}

// CHANGED WITH AI: write the shared starred_pages.STAR file.
function writeBookmarks(list) {
  fs.writeFileSync(STARRED_PATH, list.join('\n') + '\n', 'utf8');
}

// CHANGED WITH AI: spawn the engine for one URL, collect stdout, extract the JSON
// payload that follows the __BROWSE_JSON__ delimiter, and resolve with it.
function runEngine(url) {
  return new Promise((resolve) => {
    let stdout = '';
    let stderr = '';
    let timedOut = false;

    const child = spawn(ENGINE_PATH, [url], { cwd: ENGINE_DIR });

    const timer = setTimeout(() => {
      timedOut = true;
      try { child.kill('SIGKILL'); } catch (e) {}
    }, 45000);

    child.stdout.on('data', (d) => { stdout += d.toString(); });
    child.stderr.on('data', (d) => { stderr += d.toString(); });

    child.on('error', (err) => {
      clearTimeout(timer);
      resolve({ error: `Failed to start engine: ${err.message}` });
    });

    child.on('close', (code) => {
      clearTimeout(timer);
      if (timedOut) {
        resolve({ error: 'Engine timed out (45s).' });
        return;
      }

      const delim = '__BROWSE_JSON__';
      const idx = stdout.indexOf(delim);
      if (idx === -1) {
        resolve({ error: `Engine produced no layout. (code ${code})${stderr ? '\n' + stderr.slice(0, 500) : ''}` });
        return;
      }

      const after = stdout.slice(idx + delim.length).replace(/^\r?\n/, '');
      const nl = after.indexOf('\n');
      const jsonStr = nl === -1 ? after : after.slice(0, nl);

      try {
        const parsed = JSON.parse(jsonStr);
        resolve(parsed);
      } catch (e) {
        resolve({ error: `Failed to parse engine JSON: ${e.message}` });
      }
    });
  });
}

io.on('connection', (socket) => {
  console.log(`[browse-engine] client connected: ${socket.id}`);

  socket.on('navigate', async (data) => {
    const url = (typeof data === 'string' ? data : (data && data.url)) || 'home';
    console.log(`[browse-engine] navigate: ${url}`);
    const result = await runEngine(url);
    socket.emit('layout', result);
  });

  socket.on('bookmark', (data) => {
    const { url, add } = data || {};
    let list = readBookmarks();
    const i = list.indexOf(url);
    if (add) {
      if (i === -1) list.push(url);
    } else {
      if (i !== -1) list.splice(i, 1);
    }
    writeBookmarks(list);
    socket.emit('bookmarks', list);
  });

  socket.on('getBookmarks', () => {
    socket.emit('bookmarks', readBookmarks());
  });

  socket.on('disconnect', () => {
    console.log(`[browse-engine] client disconnected: ${socket.id}`);
  });
});

const PORT = 3003;
httpServer.listen(PORT, () => {
  console.log(`[browse-engine] socket.io server running on port ${PORT}`);
  console.log(`[browse-engine] engine binary: ${ENGINE_PATH}`);
});

process.on('SIGTERM', () => { httpServer.close(() => process.exit(0)); });
process.on('SIGINT', () => { httpServer.close(() => process.exit(0)); });

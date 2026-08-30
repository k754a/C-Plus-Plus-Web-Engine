const { createServer } = require('http');
const fs = require('fs');
const path = require('path');
const { spawn } = require('child_process');
const { Server } = require('socket.io');

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

//js version of the C++ bookmarks thing.
function readBookmarks() {
   const starredPages = [];
   try {
      const data = fs.readFileSync(STARRED_PATH, 'utf8'); //open the starred_pages.STAR file, and read it as a string
      for (const line of data.split('\n')) { //for each line
        if(!line.trim()) continue; //ignore blank lines
         starredPages.push(line.trim()); //add it to the array
      }
    } catch (e) {
      //if the file is not found. just to prevent a crazy amount of errors
    }
    return starredPages;
}

//write to the starred_pages.
function writeBookmarks(list) { //takes in a list
  fs.writeFileSync(STARRED_PATH, list.join('\n')); //adds each line of the list, with a new line \n
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




httpServer.listen(3003);
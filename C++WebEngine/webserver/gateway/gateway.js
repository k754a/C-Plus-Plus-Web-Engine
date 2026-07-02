// CHANGED WITH AI: Node.js reverse-proxy gateway for C++Browse.
// Replaces Caddy so the deployment only needs Node.js (no extra system packages).
//
// Listens on port 8080 (publicly accessible).
// - Requests with ?XTransformPort=<port> are proxied to localhost:<port>
//   (this is how the frontend's socket.io client reaches the engine service on 3003).
// - All other requests are proxied to the Next.js frontend on port 3000.
// - WebSocket upgrades are handled the same way (socket.io needs this).
const http = require('http');
const { createProxyServer } = require('http-proxy');

const FRONTEND_PORT = 3000;
const GATEWAY_PORT = 8080;

const proxy = createProxyServer({ ws: true });

proxy.on('error', (err, req, res) => {
  console.error('[gateway] proxy error:', err.message);
  if (res && !res.headersSent) {
    res.writeHead(502, { 'Content-Type': 'text/plain' });
    res.end('Bad Gateway');
  }
});

function getTargetPort(req) {
  const parsed = new URL(req.url, 'http://localhost');
  const port = parsed.searchParams.get('XTransformPort');
  return port || String(FRONTEND_PORT);
}

const server = http.createServer((req, res) => {
  const port = getTargetPort(req);
  proxy.web(req, res, { target: 'http://localhost:' + port });
});

// Handle WebSocket upgrades (socket.io)
server.on('upgrade', (req, socket, head) => {
  const port = getTargetPort(req);
  proxy.ws(req, socket, head, { target: 'ws://localhost:' + port });
});

server.listen(GATEWAY_PORT, () => {
  console.log(`[gateway] listening on port ${GATEWAY_PORT}`);
  console.log(`[gateway] frontend -> localhost:${FRONTEND_PORT}`);
  console.log(`[gateway] engine   -> localhost:3003 (via ?XTransformPort=3003)`);
});

process.on('SIGTERM', () => { server.close(() => process.exit(0)); });
process.on('SIGINT', () => { server.close(() => process.exit(0)); });

const http = require('http'); //load node's http stuff
const { createProxyServer } = require('http-proxy'); //load node's http-proxy stuff

const proxy = createProxyServer({ ws: true }); //make a proxy server, that handles websockets

function getPort(req) { //pull the port that we want to send too
  //check for the port, if it does not exist, use 3000
  return new URL(req.url, 'http://localhost').searchParams.get('XTransformPort') || 3000;
}

//create the server
const server = http.createServer((req, res) => {
  //send the stuff to the port
  proxy.web(req, res, { target: `http://localhost:${getPort(req)}` });
});

//runs only when a websocket req is made
server.on('upgrade', (req, socket, head) => {
  //send to the websocket
  proxy.ws(req, socket, head, { target: `ws://localhost:${getPort(req)}` });
});

//wait on port 8080
server.listen(8080);
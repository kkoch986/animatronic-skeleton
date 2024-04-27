#!/usr/bin/env node

import inquirer from 'inquirer';
import { input } from '@inquirer/prompts';
import WebSocket, { WebSocketServer } from 'ws';

const wss = new WebSocketServer({
  host: "192.168.1.171",
  port: 8888,
  path: "/",
  skipUTF8Validation: false,
  verifyClient: (info) => {
    console.log(`WebSocket server verifyClient: ${info}`);
    return true;
  },
});

console.log("starting");
wss.on('wsClientError', (err) => {
  console.error(`WebSocket server error: ${err}`);
});
wss.on('error', (err) => {
  console.error(`WebSocket server error: ${err}`);
});
wss.on('listening', () => {
  console.log(`WebSocket server listening on ws://${wss.options.host}:${wss.options.port}`);
});
wss.on('headers', (headers, req) => {
  console.log(`WebSocket server headers: ${headers}`);
});
wss.on('close', () => {
  console.log(`WebSocket server closed`);
});
wss.on('connection', (ws) => {
  console.log("new connection established");
  ws.on('error', console.error);
});


// enter a command loop to start issuing commands
while(true) {
  const answer = await input({ message: 'Enter a command' });
  console.log(answer);
  wss.clients.forEach(function each(client) {
    if (client.readyState === WebSocket.OPEN) {
      client.send(answer);
    }
  });
}

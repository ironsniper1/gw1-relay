const WebSocket = require('ws');
const http = require('http');

const PORT = process.env.PORT || 3000;

const server = http.createServer((req, res) => {
    res.writeHead(200);
    res.end('GW1 Map Tracker Relay');
});

const wss = new WebSocket.Server({ server });

// sessions: key -> { plugin: ws, viewers: [ws] }
const sessions = new Map();

wss.on('connection', (ws, req) => {
    const url = new URL(req.url, `http://localhost`);
    const key = url.searchParams.get('key');
    const role = url.searchParams.get('role');

    if (!key || !role) { ws.close(); return; }

    if (!sessions.has(key)) sessions.set(key, { plugin: null, viewers: [] });
    const session = sessions.get(key);

    if (role === 'plugin') {
        if (session.plugin) session.plugin.close();
        session.plugin = ws;
        console.log(`Plugin connected: ${key}`);

        ws.on('message', (data) => {
            for (const viewer of session.viewers) {
                if (viewer.readyState === WebSocket.OPEN) {
                    viewer.send(data);
                }
            }
        });

        ws.on('close', () => {
            session.plugin = null;
            for (const viewer of session.viewers) {
                if (viewer.readyState === WebSocket.OPEN) {
                    viewer.send(JSON.stringify({ type: 'disconnected' }));
                }
            }
            console.log(`Plugin disconnected: ${key}`);
        });

    } else if (role === 'viewer') {
        session.viewers.push(ws);
        console.log(`Viewer connected: ${key}`);

        ws.on('close', () => {
            session.viewers = session.viewers.filter(v => v !== ws);
        });
    } else {
        ws.close();
    }
});

server.listen(PORT, () => {
    console.log(`Relay server running on port ${PORT}`);
});const WebSocket = require('ws');
const http = require('http');

const PORT = process.env.PORT || 3000;

const server = http.createServer((req, res) => {
    res.writeHead(200);
    res.end('GW1 Map Tracker Relay');
});

const wss = new WebSocket.Server({ server });

// sessions: key -> { plugin: ws, viewers: [ws] }
const sessions = new Map();

wss.on('connection', (ws, req) => {
    const url = new URL(req.url, `http://localhost`);
    const key = url.searchParams.get('key');
    const role = url.searchParams.get('role');

    if (!key || !role) { ws.close(); return; }

    if (!sessions.has(key)) sessions.set(key, { plugin: null, viewers: [] });
    const session = sessions.get(key);

    if (role === 'plugin') {
        if (session.plugin) session.plugin.close();
        session.plugin = ws;
        console.log(`Plugin connected: ${key}`);

        ws.on('message', (data) => {
            for (const viewer of session.viewers) {
                if (viewer.readyState === WebSocket.OPEN) {
                    viewer.send(data);
                }
            }
        });

        ws.on('close', () => {
            session.plugin = null;
            for (const viewer of session.viewers) {
                if (viewer.readyState === WebSocket.OPEN) {
                    viewer.send(JSON.stringify({ type: 'disconnected' }));
                }
            }
            console.log(`Plugin disconnected: ${key}`);
        });

    } else if (role === 'viewer') {
        session.viewers.push(ws);
        console.log(`Viewer connected: ${key}`);

        ws.on('close', () => {
            session.viewers = session.viewers.filter(v => v !== ws);
        });
    } else {
        ws.close();
    }
});

server.listen(PORT, () => {
    console.log(`Relay server running on port ${PORT}`);
});

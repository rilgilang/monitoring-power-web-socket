const { WebSocketServer } = require('ws');

let wss;

module.exports = (req, res) => {
    if (!wss) {
        // Create a WebSocket server if it doesn't exist
        wss = new WebSocketServer({ noServer: true });

        console.log("WebSocket server initialized");

        wss.on('connection', (ws) => {
            console.log("Client connected");

            ws.send(JSON.stringify({ type: 'welcome', message: 'Connected to WebSocket server!' }));

            ws.on('message', (message) => {
                console.log(`Received: ${message}`);
                wss.clients.forEach((client) => {
                    if (client.readyState === ws.OPEN) {
                        client.send(JSON.stringify({ type: 'broadcast', message: message.toString() }));
                    }
                });
            });

            ws.on('close', () => {
                console.log("Client disconnected");
            });
        });
    }

    if (req.method === 'GET') {
        res.status(200).send('WebSocket server is running.');
    }
};

const { WebSocketServer } = require('ws');

// Create a WebSocket server
const wss = new WebSocketServer({ port: 8080 });

console.log("WebSocket server started on ws://localhost:8080");

// Handle connection
wss.on('connection', (ws) => {
    console.log("Client connected");

    // Send a welcome message
    ws.send(JSON.stringify({ type: 'welcome', message: 'Connected to WebSocket server!' }));

    // Handle incoming messages
    ws.on('message', (message) => {
        console.log(`Received: ${message}`);
        // Broadcast the message to all connected clients
        wss.clients.forEach((client) => {
            if (client.readyState === ws.OPEN) {
                client.send(JSON.stringify({ type: 'broadcast', message: message.toString() }));
            }
        });
    });

    // Handle disconnection
    ws.on('close', () => {
        console.log("Client disconnected");
    });
});

const express = require('express');
const http = require('http');
const { WebSocketServer } = require('ws');
const mqtt = require('mqtt');
const path = require('path');

const app = express();
app.use(express.static(path.join(__dirname, 'public')));
app.use(express.json()); // Middleware to parse JSON bodies

const server = http.createServer(app);
const wss = new WebSocketServer({ server });

const MQTT_BROKER_URL = 'mqtt://localhost:1883';
const mqttClient = mqtt.connect(MQTT_BROKER_URL);

// --- State Cache ---
let lastKnownSettings = null;

// --- MQTT Client Logic ---
mqttClient.on('connect', () => {
  console.log('Connected to MQTT broker');
  mqttClient.subscribe('TestResult', (err) => {
    if (err) console.error('Failed to subscribe to TestResult', err);
    else console.log('Subscribed to TestResult');
  });
  mqttClient.subscribe('Setting', (err) => {
    if (err) console.error('Failed to subscribe to Setting', err);
    else console.log('Subscribed to Setting');
  });
});

mqttClient.on('message', (topic, message) => {
  console.log(`MQTT message received on ${topic}`);
  let type;
  let payload;

  if (topic === 'TestResult') {
    type = 'data';
    payload = message.toString();
  } else if (topic === 'Setting') {
    type = 'setting';
    try {
        payload = JSON.parse(message.toString());
        lastKnownSettings = payload; // Cache the latest settings
    } catch(e) {
        console.error('Could not parse settings JSON', e);
        return;
    }
  }

  if (type) {
    const wsMessage = JSON.stringify({ type, payload });
    // Broadcast to all clients
    wss.clients.forEach(client => {
      if (client.readyState === require('ws').OPEN) {
        client.send(wsMessage);
      }
    });
  }
});

mqttClient.on('error', (err) => {
  console.error('MQTT client error:', err);
});

// --- WebSocket Server Logic ---
wss.on('connection', ws => {
  console.log('WebSocket client connected');

  // On new connection, send the last known settings immediately
  if (lastKnownSettings) {
    console.log('Sending cached settings to new client.');
    ws.send(JSON.stringify({ type: 'setting', payload: lastKnownSettings }));
  }

  ws.on('close', () => {
    console.log('WebSocket client disconnected');
  });
  ws.on('error', (error) => {
    console.error('WebSocket error:', error);
  });
});

// --- Express API Endpoint ---
app.post('/settingchange', (req, res) => {
  const { id, value } = req.body;
  if (id === undefined || value === undefined) {
    return res.status(400).json({ error: 'Missing id or value' });
  }

  const message = `${id},${value}`;
  mqttClient.publish('Change', message, (err) => {
    if (err) {
      console.error('Failed to publish to Change topic', err);
      return res.status(500).json({ error: 'Failed to publish MQTT message' });
    }
    console.log(`Published to Change: ${message}`);
    res.status(200).json({ success: true, message: 'Change request sent.' });
  });
});

// --- Start Server ---
const PORT = process.env.PORT || 3000;
server.listen(PORT, () => {
  console.log(`Web server is listening on port ${PORT}`);
});

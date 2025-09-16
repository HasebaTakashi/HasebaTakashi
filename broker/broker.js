const aedes = require('aedes')();
const server = require('net').createServer(aedes.handle);
const port = 1883;

server.listen(port, function () {
  console.log('MQTT broker started and listening on port', port);
});

// Log client connections
aedes.on('client', function (client) {
  console.log(`Client Connected: ${client.id}`);
});

// Log client disconnections
aedes.on('clientDisconnect', function (client) {
  console.log(`Client Disconnected: ${client.id}`);
});

// Log published messages
aedes.on('publish', function (packet, client) {
  if (client) {
    console.log(`Message from ${client.id}: ${packet.topic} - ${packet.payload.toString()}`);
  }
});

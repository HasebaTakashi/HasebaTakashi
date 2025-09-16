const mqtt = require('mqtt');
const fs = require('fs');
const path = 'thresholds.json';

let thresholds;

// Load thresholds from JSON file
try {
  const rawData = fs.readFileSync(path);
  thresholds = JSON.parse(rawData);
  console.log('Thresholds loaded:', thresholds);
} catch (err) {
  console.error('Error reading thresholds file!', err);
  // Default values in case file doesn't exist or is corrupted
  thresholds = [0.3, 0.6, 0.9];
  fs.writeFileSync(path, JSON.stringify(thresholds, null, 2));
  console.log('Created default thresholds file:', thresholds);
}


const client = mqtt.connect('mqtt://localhost:1883');

client.on('connect', () => {
  console.log('Connected to MQTT broker');
  client.subscribe('TestData', (err) => {
    if (err) console.error('Failed to subscribe to TestData', err);
    else console.log('Subscribed to TestData');
  });
  client.subscribe('Change', (err) => {
    if (err) console.error('Failed to subscribe to Change', err);
    else console.log('Subscribed to Change');
  });
  client.subscribe('RequestSetting', (err) => {
    if(err) console.error('Failed to subscribe to RequestSetting', err);
    else console.log('Subscribed to RequestSetting');
  });
});

client.on('message', (topic, message) => {
  const messageStr = message.toString();
  console.log(`Received message on topic ${topic}: ${messageStr}`);

  if (topic === 'TestData') {
    handleTestData(messageStr);
  } else if (topic === 'Change') {
    handleChange(messageStr);
  } else if (topic === 'RequestSetting') {
    handleRequestSetting();
  }
});

function handleTestData(messageStr) {
  const data = messageStr.split(',').map(Number);
  if (data.length !== 5 || data.some(isNaN)) {
    console.error('Invalid data format received on TestData:', messageStr);
    return;
  }

  const valueToCompare = data[4];
  let result = 0;
  // Thresholds are sorted, so we can check in order
  if (valueToCompare >= thresholds[0]) result = 1;
  if (valueToCompare >= thresholds[1]) result = 2;
  if (valueToCompare >= thresholds[2]) result = 3;

  const resultMessage = `${messageStr},${result}`;
  client.publish('TestResult', resultMessage, (err) => {
    if (err) console.error('Failed to publish to TestResult', err);
    else console.log(`Published to TestResult: ${resultMessage}`);
  });
}

function handleChange(messageStr) {
  const parts = messageStr.split(',');
  const index = parseInt(parts[0], 10) - 1; // Assuming 1-based index from message
  const value = parseFloat(parts[1]);

  if (isNaN(index) || isNaN(value) || index < 0 || index > 2) {
    console.error('Invalid change format received on Change:', messageStr);
    return;
  }

  thresholds[index] = value;
  // Re-sort thresholds to maintain order
  thresholds.sort((a, b) => a - b);

  console.log('Updated thresholds:', thresholds);

  fs.writeFile(path, JSON.stringify(thresholds, null, 2), (err) => {
    if (err) {
      console.error('Failed to write updated thresholds to file', err);
    } else {
      console.log('Thresholds file updated successfully.');
      // Publish the new settings after change
      client.publish('Setting', JSON.stringify(thresholds));
    }
  });
}

function handleRequestSetting() {
    client.publish('Setting', JSON.stringify(thresholds), (err) => {
        if(err) console.error('Failed to publish to Setting', err);
        else console.log(`Published current settings to Setting topic.`);
    });
}

client.on('error', (err) => {
  console.error('MQTT client error:', err);
});

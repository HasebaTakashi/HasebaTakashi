# PLC-style HMI with Sinatra and MQTT

This project is a web-based Human-Machine Interface (HMI) that resembles a Programmable Logic Controller (PLC) touch panel. It's built with a Ruby/Sinatra backend and a vanilla JavaScript frontend. Communication between the frontend and backend is handled via WebSockets, and the backend communicates with external systems (like a PLC or other applications) via MQTT.

## Features

- **PLC-like UI**: Dark-themed interface with control buttons and numerical displays.
- **Real-time Data Visualization**: Displays up to three data points on a trend graph.
- **WebSocket Communication**: For low-latency updates between the browser and the server.
- **MQTT Integration**: The server acts as a bridge between the web interface and an MQTT message bus.
    - Subscribes to topics to receive data for display (`/data/display`).
    - Subscribes to topics to receive configuration data (`/data/config`).
    - Publishes events (e.g., button presses) to a topic (`/event/button`).
- **Dynamic Graph Control**: Toggle the visibility of data series on the trend graph using checkboxes.

## Prerequisites

Before you begin, ensure you have the following installed:

- **Ruby**: [Installation guide](https://www.ruby-lang.org/en/documentation/installation/)
- **Bundler**: `gem install bundler`
- **An MQTT Broker**: Such as [Mosquitto](https://mosquitto.org/). The application assumes the broker is running on `localhost:1883`.

## Setup and Installation

1.  **Clone the repository:**
    ```bash
    git clone <repository-url>
    cd <repository-directory>
    ```

2.  **Install dependencies:**
    This project uses Bundler to manage gems. The gems will be installed locally in the `vendor/bundle` directory.
    ```bash
    bundle install
    ```

## How to Run

1.  **Start the Sinatra application:**
    ```bash
    bundle exec ruby app.rb
    ```
    You should see output indicating that the Thin web server is running and the MQTT client is connected.

2.  **Open the HMI:**
    Open your web browser and navigate to `http://localhost:4567`. You should see the HMI interface.

## How to Test (Using an MQTT Client)

You can use command-line tools like `mosquitto_pub` and `mosquitto_sub` to simulate an external system interacting with the HMI.

### 1. Sending Data to the HMI

Open a new terminal and use `mosquitto_pub` to send data. The HMI should update in real-time.

**Send numerical data for display:**
This will update the numeric displays and the trend graph.
```bash
mosquitto_pub -h localhost -p 1883 -t /data/display -m '{"data-1": 10.5, "data-2": 45.8, "data-3": 88.2}'
```
Try sending different values to see the graph change.
```bash
mosquitto_pub -h localhost -p 1883 -t /data/display -m '{"data-1": 15.2, "data-2": 42.1, "data-3": 91.5}'
```

**Send configuration data:**
This will update the "Configuration" display area.
```bash
mosquitto_pub -h localhost -p 1883 -t /data/config -m '{"param_a": "ON", "param_b": 1234, "threshold": 99.9}'
```

### 2. Receiving Events from the HMI

Open another terminal and use `mosquitto_sub` to listen for events published by the HMI.

**Subscribe to the button event topic:**
```bash
mosquitto_sub -h localhost -p 1883 -t /event/button
```
Now, go to the web interface and click on "Button 1", "Button 2", or "Button 3". You should see JSON messages appear in the terminal where `mosquitto_sub` is running, like this:
```json
{"button_id":"btn-1"}
{"button_id":"btn-2"}
```

This completes the basic setup and testing workflow.
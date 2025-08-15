# Waveform Viewer Frontend

This directory contains the frontend application for the Waveform Viewer. It's built with HTML, CSS, and vanilla JavaScript, and it uses Chart.js for plotting.

## Setup Instructions

Follow these steps to get the viewer running.

### 1. Download Chart.js

This frontend requires the **Chart.js** library.

-   **Download from:** [https://cdn.jsdelivr.net/npm/chart.js](https://cdn.jsdelivr.net/npm/chart.js)
-   Save the downloaded file with the name `chart.js`.

### 2. Place Files in Your Project

You need to place the files from this `wave_viewer` directory into your Ruby application's `public` folder. The `public` folder is the standard place for static assets in Sinatra applications.

1.  Place the entire `wave_viewer` directory inside your `public` directory.
2.  Place the downloaded `chart.js` file into the `public/wave_viewer/js/` directory.

The final file structure should look like this:

```
your_ruby_project/
|-- public/
|   `-- wave_viewer/
|       |-- index.html
|       |-- README.md      <-- This file
|       |-- css/
|       |   `-- style.css
|       `-- js/
|           |-- chart.js   <-- The file you downloaded
|           `-- main.js
|
|-- your_app.rb
|-- (other files...)
```

### 3. Adjust Ruby Backend

To allow Sinatra to serve the new `index.html` page correctly, you must **delete or comment out** the existing `/wave_viewer` route in your Ruby application. This prevents the old `erb` template from being rendered.

Find and remove this code block:

```ruby
# get '/wave_viewer' do
#   @title = 'WaveViewer'
#   erb :wave_viewer
# end
```

By removing this, Sinatra's static file handling will automatically serve `public/wave_viewer/index.html` when a request is made to that path.

### 4. Run the Application

1.  Start your Ruby server.
2.  Open your web browser and navigate to:
    **`http://<your_host>:<your_port>/wave_viewer/index.html`**

You should now see the waveform viewer interface. You can use the buttons to start/stop sampling and change parameters. The graph will update in real-time as data is received from the WebSocket.

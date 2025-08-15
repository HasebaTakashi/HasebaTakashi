document.addEventListener('DOMContentLoaded', () => {
    // --- DOM Elements ---
    const startBtn = document.getElementById('start-sampling');
    const stopBtn = document.getElementById('stop-sampling');
    const changeSecBtn = document.getElementById('change-sec');
    const changeChBtn = document.getElementById('change-ch');
    const secInput = document.getElementById('sec-input');
    const chInput = document.getElementById('ch-input');

    // Info panel elements
    const maxValueEl = document.getElementById('max-value');
    const minValueEl = document.getElementById('min-value');
    const aveValueEl = document.getElementById('ave-value');
    const rmsValueEl = document.getElementById('rms-value');
    const pulseValueEl = document.getElementById('pulse-value');
    const terminalVolValueEl = document.getElementById('terminal-vol-value');
    const datetimeValueEl = document.getElementById('datetime-value');

    // --- WebSocket Setup ---
    let ws;
    // Use wss:// if the main page is served over https://
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const socketUrl = `${protocol}//${window.location.host}/socket`;

    function connect() {
        ws = new WebSocket(socketUrl);

        ws.onopen = () => {
            console.log('WebSocket connection established.');
        };

        ws.onmessage = (event) => {
            try {
                const data = JSON.parse(event.data);
                updateChart(data);
                updateInfoPanel(data);
            } catch (error) {
                console.error('Error parsing received data:', error);
            }
        };

        ws.onclose = () => {
            console.log('WebSocket connection closed. Attempting to reconnect in 3 seconds...');
            setTimeout(connect, 3000); // Simple reconnect logic
        };

        ws.onerror = (error) => {
            console.error('WebSocket error:', error);
            ws.close(); // This will trigger the onclose handler for reconnection
        };
    }

    function sendMessage(message) {
        if (ws && ws.readyState === WebSocket.OPEN) {
            ws.send(message);
        } else {
            console.error('WebSocket is not connected.');
        }
    }

    // --- Chart.js Setup ---
    const ctx = document.getElementById('waveChart').getContext('2d');
    if (typeof Chart === 'undefined') {
        console.error('Chart.js is not loaded. Please download it and place it in js/chart.js');
        // Display an error message on the canvas
        ctx.font = '16px Arial';
        ctx.fillStyle = 'red';
        ctx.fillText('Error: Chart.js is not loaded.', 10, 50);
        return; // Stop execution if Chart.js is not available
    }

    const waveChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: [], // Time data
            datasets: [{
                label: 'Waveform',
                data: [], // Value data
                borderColor: 'rgba(0, 255, 0, 1)', // Green line
                backgroundColor: 'rgba(0, 255, 0, 0.1)', // Green area under the line
                borderWidth: 1.5,
                pointRadius: 0, // Hide points for a smoother line
                tension: 0.1 // Slight curve to the line
            }]
        },
        options: {
            maintainAspectRatio: false,
            animation: false, // Disable animation for real-time data
            scales: {
                x: {
                    type: 'linear', // Treat x-axis as a continuous series of numbers
                    ticks: {
                        color: 'rgba(255, 255, 255, 0.7)'
                    },
                    grid: {
                        color: 'rgba(255, 255, 255, 0.2)'
                    }
                },
                y: {
                    ticks: {
                        color: 'rgba(255, 255, 255, 0.7)'
                    },
                    grid: {
                        color: 'rgba(255, 255, 255, 0.2)'
                    }
                }
            },
            plugins: {
                legend: {
                    display: false // Hide the legend
                },
                tooltip: {
                    enabled: true // Enable tooltips on hover
                }
            }
        }
    });

    // --- Update Functions ---
    function updateChart(data) {
        if (data && data.time && data.value) {
            const timeArray = data.time.split(',').map(Number);
            const valueArray = data.value.split(',').map(Number);

            waveChart.data.labels = timeArray;
            waveChart.data.datasets[0].data = valueArray;
            waveChart.update();
        }
    }

    function updateInfoPanel(data) {
        maxValueEl.textContent = data.max ?? '-';
        minValueEl.textContent = data.min ?? '-';
        aveValueEl.textContent = data.ave ?? '-';
        rmsValueEl.textContent = data.rms ?? '-';
        pulseValueEl.textContent = data.pulse ?? '-';
        terminalVolValueEl.textContent = data.terminal_vol ?? '-';
        datetimeValueEl.textContent = data.datetime ? new Date(data.datetime).toLocaleString() : '-';
    }

    // --- Event Listeners ---
    startBtn.addEventListener('click', () => sendMessage('startsampling'));
    stopBtn.addEventListener('click', () => sendMessage('stopsampling'));

    changeSecBtn.addEventListener('click', () => {
        const sec = secInput.value;
        sendMessage(`changesec,${sec}`);
    });

    changeChBtn.addEventListener('click', () => {
        const ch = chInput.value;
        sendMessage(`changechannel,${ch}`);
    });

    // --- Initial Connection ---
    connect();
});

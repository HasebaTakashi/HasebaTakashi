document.addEventListener('DOMContentLoaded', () => {
    // --- DOM Elements ---
    const startBtn = document.getElementById('start-sampling');
    const stopBtn = document.getElementById('stop-sampling');
    const changeSecBtn = document.getElementById('change-sec');
    const changeChBtn = document.getElementById('change-ch');
    const secInput = document.getElementById('sec-input');
    const chInput = document.getElementById('ch-input');

    // Info panel elements
    const statusLightEl = document.getElementById('status-light');
    const statusTextEl = document.getElementById('status-text');
    const maxValueEl = document.getElementById('max-value');
    const minValueEl = document.getElementById('min-value');
    const aveValueEl = document.getElementById('ave-value');
    const rmsValueEl = document.getElementById('rms-value');
    const pulseValueEl = document.getElementById('pulse-value');
    const terminalVolValueEl = document.getElementById('terminal-vol-value');
    const datetimeValueEl = document.getElementById('datetime-value');

    // --- Status Update Function ---
    function updateStatus(status, message) {
        statusLightEl.className = 'light'; // Reset classes
        statusLightEl.classList.add(status); // 'connecting', 'connected', or 'disconnected'
        statusTextEl.textContent = message;

        // Also update text color for consistency
        const colorMap = {
            connected: 'var(--accent-color)',
            disconnected: '#dc3545', // Red
            connecting: '#ffc107'     // Yellow
        };
        statusTextEl.style.color = colorMap[status] || 'var(--primary-text-color)';
    }

    // --- WebSocket Setup ---
    let ws;
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const socketUrl = `${protocol}//${window.location.host}/socket`;

    function connect() {
        updateStatus('connecting', 'Connecting...');
        ws = new WebSocket(socketUrl);

        ws.onopen = () => {
            console.log('WebSocket connection established.');
            updateStatus('connected', 'Connected');
        };

        ws.onmessage = (event) => {
            try {
                if (!statusLightEl.classList.contains('connected')) {
                    updateStatus('connected', 'Connected');
                }
                const data = JSON.parse(event.data);
                updateChart(data);
                updateInfoPanel(data);
            } catch (error) {
                console.error('Error parsing received data:', error);
            }
        };

        ws.onclose = () => {
            console.log('WebSocket connection closed. Attempting to reconnect in 3 seconds...');
            updateStatus('disconnected', 'Disconnected');
            setTimeout(connect, 3000);
        };

        ws.onerror = (error) => {
            console.error('WebSocket error:', error);
            updateStatus('disconnected', 'Error');
            ws.close();
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
        ctx.font = '16px Arial';
        ctx.fillStyle = 'red';
        ctx.fillText('Error: Chart.js is not loaded.', 10, 50);
        return;
    }

    const waveChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: 'Waveform',
                data: [],
                borderColor: 'rgba(0, 255, 0, 1)',
                backgroundColor: 'rgba(0, 255, 0, 0.1)',
                borderWidth: 1.5,
                pointRadius: 0,
                tension: 0.1
            }]
        },
        options: {
            maintainAspectRatio: false,
            animation: false,
            scales: {
                x: {
                    type: 'linear',
                    ticks: { color: 'rgba(255, 255, 255, 0.7)' },
                    grid: { color: 'rgba(255, 255, 255, 0.2)' }
                },
                y: {
                    ticks: { color: 'rgba(255, 255, 255, 0.7)' },
                    grid: { color: 'rgba(255, 255, 255, 0.2)' }
                }
            },
            plugins: {
                legend: { display: false },
                tooltip: { enabled: true }
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

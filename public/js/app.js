document.addEventListener('DOMContentLoaded', () => {
    const domElements = {
        status: document.getElementById('status'),
        diagIdSelector: document.getElementById('diag-id-selector'),
        diagId: document.getElementById('diag-id'),
        mode: document.getElementById('mode'),
        learnedCount: document.getElementById('learned-count'),
        targetCount: document.getElementById('target-count'),
        diagLevel: document.getElementById('diag-level'),
        result: document.getElementById('result'),
        threshold1: document.getElementById('threshold-1'),
        threshold2: document.getElementById('threshold-2'),
        threshold3: document.getElementById('threshold-3'),
    };

    const dataStore = new Map();

    const updateView = (data) => {
        if (!data) return;

        domElements.diagId.textContent = data[0];

        const modeNum = data[1];
        domElements.mode.textContent = modeNum === '1' ? `学習 (${modeNum})` : `診断 (${modeNum})`;

        domElements.learnedCount.textContent = data[2];
        domElements.targetCount.textContent = data[3];

        const levelNum = data[4];
        domElements.diagLevel.textContent = `レベル ${levelNum}`;
        domElements.diagLevel.className = `value level-${levelNum}`;

        domElements.result.textContent = parseFloat(data[5]).toFixed(6);
        domElements.threshold1.textContent = data[6];
        domElements.threshold2.textContent = data[7];
        domElements.threshold3.textContent = data[8];
    };

    const wsProtocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${wsProtocol}//${window.location.host}/socket`;
    const ws = new WebSocket(wsUrl);

    ws.onopen = () => {
        console.log('Connected to WebSocket server');
        domElements.status.textContent = 'Connected';
        domElements.status.style.color = 'green';
    };

    ws.onmessage = (event) => {
        console.log('Received data:', event.data);
        const data = event.data.split(',');

        if (data.length < 9) {
            console.error('Invalid data format received');
            return;
        }

        const currentDiagId = data[0];
        dataStore.set(currentDiagId, data);

        // Add new ID to selector if it doesn't exist
        if (!domElements.diagIdSelector.querySelector(`option[value="${currentDiagId}"]`)) {
            const option = document.createElement('option');
            option.value = currentDiagId;
            option.textContent = currentDiagId;
            domElements.diagIdSelector.appendChild(option);
        }

        // Update view only if the received data is for the currently selected ID
        if (currentDiagId === domElements.diagIdSelector.value) {
            updateView(data);
        }
    };

    domElements.diagIdSelector.addEventListener('change', (event) => {
        const selectedId = event.target.value;
        const data = dataStore.get(selectedId);
        updateView(data);
    });

    ws.onclose = () => {
        console.log('Disconnected from WebSocket server');
        domElements.status.textContent = 'Disconnected. Please refresh the page to reconnect.';
        domElements.status.style.color = 'red';
    };

    ws.onerror = (error) => {
        console.error('WebSocket Error:', error);
        domElements.status.textContent = 'Connection Error.';
        domElements.status.style.color = 'red';
    };
});

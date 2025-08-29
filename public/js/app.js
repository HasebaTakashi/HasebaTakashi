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
    const fieldsPerDataset = 9;

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

    const processDataset = (data) => {
        const currentDiagId = data[0];
        dataStore.set(currentDiagId, data);

        if (!domElements.diagIdSelector.querySelector(`option[value="${currentDiagId}"]`)) {
            const option = document.createElement('option');
            option.value = currentDiagId;
            option.textContent = currentDiagId;
            domElements.diagIdSelector.appendChild(option);
        }
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
        const allValues = event.data.split(',');

        if (allValues.length === 0 || allValues[0] === '') {
            return; // Ignore empty messages
        }
        if (allValues.length % fieldsPerDataset !== 0) {
            console.error('Invalid data format: length is not a multiple of 9.', allValues.length);
            return;
        }

        const wasSelectorEmpty = domElements.diagIdSelector.options.length === 0;

        const numDatasets = allValues.length / fieldsPerDataset;
        for (let i = 0; i < numDatasets; i++) {
            const dataset = allValues.slice(i * fieldsPerDataset, (i + 1) * fieldsPerDataset);
            processDataset(dataset);
        }

        if (wasSelectorEmpty) {
            const firstId = allValues[0];
            domElements.diagIdSelector.value = firstId;
            updateView(dataStore.get(firstId));
        } else {
            const selectedId = domElements.diagIdSelector.value;
            updateView(dataStore.get(selectedId));
        }
    };

    domElements.diagIdSelector.addEventListener('change', (event) => {
        const selectedId = event.target.value;
        updateView(dataStore.get(selectedId));
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

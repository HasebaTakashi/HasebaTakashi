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
        chartCanvas: document.getElementById('result-chart'),
    };

    let dataStore = new Map();
    const fieldsPerDataset = 9;
    const maxHistory = 100;

    const saveState = () => {
        try {
            sessionStorage.setItem('dataStore', JSON.stringify(Array.from(dataStore.entries())));
            const selectedId = domElements.diagIdSelector.value;
            if (selectedId) {
                sessionStorage.setItem('selectedDiagId', selectedId);
            }
        } catch (e) {
            console.error("Failed to save state to sessionStorage", e);
        }
    };

    const loadState = () => {
        try {
            const savedStore = sessionStorage.getItem('dataStore');
            if (savedStore) {
                dataStore = new Map(JSON.parse(savedStore));
                domElements.diagIdSelector.innerHTML = '';
                for (const key of dataStore.keys()) {
                    const option = document.createElement('option');
                    option.value = key;
                    option.textContent = key;
                    domElements.diagIdSelector.appendChild(option);
                }
            }

            const savedId = sessionStorage.getItem('selectedDiagId');
            if (savedId && dataStore.has(savedId)) {
                domElements.diagIdSelector.value = savedId;
            }

            updateDisplay(domElements.diagIdSelector.value);
        } catch (e) {
            console.error("Failed to load state from sessionStorage", e);
            dataStore = new Map(); // Reset on error
        }
    };

    // --- Chart.js Initialization ---
    const chartCtx = domElements.chartCanvas.getContext('2d');
    const resultChart = new Chart(chartCtx, {
        type: 'line',
        data: {
            labels: Array.from({ length: maxHistory }, (_, i) => i + 1),
            datasets: [
                { label: '計算結果', data: [], borderColor: 'rgba(75, 192, 192, 1)', borderWidth: 2, fill: false, tension: 0.1 },
                { label: 'レベル1しきい値', data: [], borderColor: 'rgba(255, 206, 86, 1)', borderWidth: 1, borderDash: [5, 5], fill: false, pointRadius: 0 },
                { label: 'レベル2しきい値', data: [], borderColor: 'rgba(255, 159, 64, 1)', borderWidth: 1, borderDash: [5, 5], fill: false, pointRadius: 0 },
                { label: 'レベル3しきい値', data: [], borderColor: 'rgba(255, 99, 132, 1)', borderWidth: 1, borderDash: [5, 5], fill: false, pointRadius: 0 },
            ],
        },
        options: { scales: { y: { beginAtZero: true } }, animation: { duration: 0 } }
    });

    const updateDisplay = (diagId) => {
        const stored = dataStore.get(diagId);
        if (!stored) return;
        const data = stored.latestData;
        domElements.diagId.textContent = data[0];
        domElements.mode.textContent = data[1] === '1' ? `学習 (${data[1]})` : `診断 (${data[1]})`;
        domElements.learnedCount.textContent = data[2];
        domElements.targetCount.textContent = data[3];
        domElements.diagLevel.textContent = `レベル ${data[4]}`;
        domElements.diagLevel.className = `value level-${data[4]}`;
        domElements.result.textContent = parseFloat(data[5]).toFixed(6);
        domElements.threshold1.textContent = data[6];
        domElements.threshold2.textContent = data[7];
        domElements.threshold3.textContent = data[8];
        resultChart.data.datasets[0].data = stored.resultsHistory;
        const fill = (v) => Array(maxHistory).fill(v);
        resultChart.data.datasets[1].data = fill(stored.thresholds.t1);
        resultChart.data.datasets[2].data = fill(stored.thresholds.t2);
        resultChart.data.datasets[3].data = fill(stored.thresholds.t3);
        resultChart.update('none');
    };

    const processDataset = (data) => {
        const diagId = data[0];
        if (!dataStore.has(diagId)) {
            dataStore.set(diagId, {
                latestData: [], resultsHistory: [], thresholds: { t1: 0, t2: 0, t3: 0 },
            });
            const option = document.createElement('option');
            option.value = diagId;
            option.textContent = diagId;
            domElements.diagIdSelector.appendChild(option);
        }
        const storeEntry = dataStore.get(diagId);
        storeEntry.latestData = data;
        storeEntry.thresholds = { t1: parseFloat(data[6]), t2: parseFloat(data[7]), t3: parseFloat(data[8]) };
        storeEntry.resultsHistory.push(parseFloat(data[5]));
        if (storeEntry.resultsHistory.length > maxHistory) {
            storeEntry.resultsHistory.shift();
        }
    };

    // --- Initial Load from Storage ---
    loadState();

    // --- WebSocket Connection ---
    const wsProtocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${wsProtocol}//${window.location.host}/socket`;
    const ws = new WebSocket(wsUrl);

    ws.onopen = () => domElements.status.textContent = 'Connected';

    ws.onmessage = (event) => {
        const allValues = event.data.split(',');
        if (allValues.length === 0 || allValues[0] === '' || allValues.length % fieldsPerDataset !== 0) {
            console.error('Invalid data format received:', event.data);
            return;
        }
        const numDatasets = allValues.length / fieldsPerDataset;
        for (let i = 0; i < numDatasets; i++) {
            const dataset = allValues.slice(i * fieldsPerDataset, (i + 1) * fieldsPerDataset);
            processDataset(dataset);
        }
        updateDisplay(domElements.diagIdSelector.value);
        saveState();
    };

    domElements.diagIdSelector.addEventListener('change', (event) => {
        updateDisplay(event.target.value);
        saveState();
    });

    ws.onclose = () => domElements.status.textContent = 'Disconnected';
    ws.onerror = () => domElements.status.textContent = 'Connection Error';
});

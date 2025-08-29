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

    const dataStore = new Map();
    const fieldsPerDataset = 9;
    const maxHistory = 100;

    // --- Chart.js Initialization ---
    const chartCtx = domElements.chartCanvas.getContext('2d');
    const resultChart = new Chart(chartCtx, {
        type: 'line',
        data: {
            labels: Array.from({ length: maxHistory }, (_, i) => i + 1),
            datasets: [
                {
                    label: '計算結果',
                    data: [],
                    borderColor: 'rgba(75, 192, 192, 1)',
                    backgroundColor: 'rgba(75, 192, 192, 0.2)',
                    borderWidth: 2,
                    fill: false,
                    tension: 0.1,
                },
                {
                    label: 'レベル1しきい値',
                    data: [],
                    borderColor: 'rgba(255, 206, 86, 1)',
                    borderWidth: 1,
                    borderDash: [5, 5],
                    fill: false,
                    pointRadius: 0,
                },
                {
                    label: 'レベル2しきい値',
                    data: [],
                    borderColor: 'rgba(255, 159, 64, 1)',
                    borderWidth: 1,
                    borderDash: [5, 5],
                    fill: false,
                    pointRadius: 0,
                },
                {
                    label: 'レベル3しきい値',
                    data: [],
                    borderColor: 'rgba(255, 99, 132, 1)',
                    borderWidth: 1,
                    borderDash: [5, 5],
                    fill: false,
                    pointRadius: 0,
                },
            ],
        },
        options: {
            scales: {
                y: {
                    beginAtZero: true,
                },
            },
            animation: {
                duration: 200, // Faster animation
            },
        },
    });

    const updateDisplay = (diagId) => {
        const stored = dataStore.get(diagId);
        if (!stored) return;

        const data = stored.latestData;
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

        // Update chart
        resultChart.data.datasets[0].data = stored.resultsHistory;
        const fillThreshold = (v) => Array(maxHistory).fill(v);
        resultChart.data.datasets[1].data = fillThreshold(stored.thresholds.t1);
        resultChart.data.datasets[2].data = fillThreshold(stored.thresholds.t2);
        resultChart.data.datasets[3].data = fillThreshold(stored.thresholds.t3);
        resultChart.update('none'); // 'none' for no animation on update
    };

    const processDataset = (data) => {
        const diagId = data[0];
        const result = parseFloat(data[5]);
        const thresholds = {
            t1: parseFloat(data[6]),
            t2: parseFloat(data[7]),
            t3: parseFloat(data[8]),
        };

        if (!dataStore.has(diagId)) {
            dataStore.set(diagId, {
                latestData: data,
                resultsHistory: [],
                thresholds: { t1: 0, t2: 0, t3: 0 },
            });
            const option = document.createElement('option');
            option.value = diagId;
            option.textContent = diagId;
            domElements.diagIdSelector.appendChild(option);
        }

        const storeEntry = dataStore.get(diagId);
        storeEntry.latestData = data;
        storeEntry.thresholds = thresholds;
        storeEntry.resultsHistory.push(result);
        if (storeEntry.resultsHistory.length > maxHistory) {
            storeEntry.resultsHistory.shift();
        }
    };

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

        const wasSelectorEmpty = domElements.diagIdSelector.options.length === 0;
        const numDatasets = allValues.length / fieldsPerDataset;
        for (let i = 0; i < numDatasets; i++) {
            const dataset = allValues.slice(i * fieldsPerDataset, (i + 1) * fieldsPerDataset);
            processDataset(dataset);
        }

        let idToDisplay = domElements.diagIdSelector.value;
        if (wasSelectorEmpty) {
            idToDisplay = allValues[0];
            domElements.diagIdSelector.value = idToDisplay;
        }
        updateDisplay(idToDisplay);
    };

    domElements.diagIdSelector.addEventListener('change', (event) => {
        updateDisplay(event.target.value);
    });

    ws.onclose = () => domElements.status.textContent = 'Disconnected';
    ws.onerror = () => domElements.status.textContent = 'Connection Error';
});

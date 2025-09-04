document.addEventListener('DOMContentLoaded', () => {
    // --- DOM Elements ---
    const domElements = {
        status: document.getElementById('status'),
        diagIdSelector: document.getElementById('diag-id-selector'),
        summaryGrid: document.getElementById('summary-grid'),
        summaryPrevBtn: document.getElementById('summary-prev-btn'),
        summaryNextBtn: document.getElementById('summary-next-btn'),
        diagId: document.getElementById('diag-id'),
        mode: document.getElementById('mode'),
        learnedCount: document.getElementById('learned-count'),
        targetCount: document.getElementById('target-count'),
        diagLevelLamp: document.getElementById('diag-level-lamp'),
        diagLevelText: document.getElementById('diag-level-text'),
        result: document.getElementById('result'),
        threshold1: document.getElementById('threshold-1'),
        threshold2: document.getElementById('threshold-2'),
        threshold3: document.getElementById('threshold-3'),
        chartCanvas: document.getElementById('result-chart'),
    };

    // --- State Variables ---
    let dataStore = new Map();
    let diagIdToNameMap = new Map();
    let summaryCurrentPage = 0;
    const summaryItemsPerPage = 4;
    const fieldsPerDataset = 9;
    const maxHistory = 100;

    // --- Chart.js Initialization ---
    const resultChart = new Chart(domElements.chartCanvas.getContext('2d'), {
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

    // --- Functions ---
    const saveState = () => {
        try {
            sessionStorage.setItem('dataStore', JSON.stringify(Array.from(dataStore.entries())));
            const selectedId = domElements.diagIdSelector.value;
            if (selectedId) sessionStorage.setItem('selectedDiagId', selectedId);
            sessionStorage.setItem('summaryCurrentPage', summaryCurrentPage);
        } catch (e) { console.error("Failed to save state", e); }
    };

    const renderSummaryGrid = () => {
        domElements.summaryGrid.innerHTML = '';
        const allIds = Array.from(dataStore.keys()).sort((a, b) => a - b);
        const startIndex = summaryCurrentPage * summaryItemsPerPage;
        const endIndex = startIndex + summaryItemsPerPage;
        const idsToShow = allIds.slice(startIndex, endIndex);

        for (const id of idsToShow) {
            const stored = dataStore.get(id);
            if (!stored || !stored.latestData) continue;
            const data = stored.latestData;
            const name = diagIdToNameMap.get(id) || `ID: ${id}`;
            const level = data[4];
            const result = parseFloat(data[5]).toFixed(4);

            const cardHTML = `
                <div class="summary-card" data-id="${id}" style="cursor: pointer;">
                    <div class="summary-card-header">
                        <span class="summary-card-id">${id}</span>
                        <span class="lamp level-${level}"></span>
                    </div>
                    <div class="summary-card-name">${name}</div>
                    <div class="summary-card-result">${result}</div>
                </div>`;
            domElements.summaryGrid.insertAdjacentHTML('beforeend', cardHTML);
        }
        domElements.summaryPrevBtn.disabled = summaryCurrentPage === 0;
        domElements.summaryNextBtn.disabled = endIndex >= allIds.length;
    };

    const updateDisplay = (diagId) => {
        const stored = dataStore.get(diagId);
        if (!stored || !stored.latestData) return;
        const data = stored.latestData;
        domElements.diagId.textContent = data[0];
        domElements.mode.textContent = data[1] === '1' ? `学習 (${data[1]})` : `診断 (${data[1]})`;
        domElements.learnedCount.textContent = data[2];
        domElements.targetCount.textContent = data[3];
        const levelNum = data[4];
        domElements.diagLevelLamp.className = `lamp level-${levelNum}`;
        domElements.diagLevelText.textContent = `レベル ${levelNum}`;
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

    const init = async () => {
        // Load state from session storage
        try {
            const savedStore = sessionStorage.getItem('dataStore');
            if (savedStore) dataStore = new Map(JSON.parse(savedStore));
            summaryCurrentPage = parseInt(sessionStorage.getItem('summaryCurrentPage') || '0', 10);
        } catch (e) { console.error("Failed to load state", e); dataStore = new Map(); }

        // Fetch settings for names
        try {
            const response = await fetch('/api/settings');
            if (response.ok) {
                const settings = await response.json();
                for (const key in settings) {
                    diagIdToNameMap.set(settings[key].DetectID.toString(), settings[key].DetectName);
                }
            }
        } catch (e) { console.error("Failed to fetch settings", e); }

        // Initial UI Render
        domElements.diagIdSelector.innerHTML = '';
        const sortedIds = Array.from(dataStore.keys()).sort((a, b) => a - b);
        for (const key of sortedIds) {
            const option = document.createElement('option');
            option.value = key;
            option.textContent = key;
            domElements.diagIdSelector.appendChild(option);
        }
        const savedId = sessionStorage.getItem('selectedDiagId');
        if (savedId && dataStore.has(savedId)) domElements.diagIdSelector.value = savedId;

        renderSummaryGrid();
        updateDisplay(domElements.diagIdSelector.value);

        // --- Event Listeners & WebSocket ---
        domElements.summaryPrevBtn.addEventListener('click', () => {
            if (summaryCurrentPage > 0) {
                summaryCurrentPage--;
                renderSummaryGrid();
                saveState();
            }
        });
        domElements.summaryNextBtn.addEventListener('click', () => {
            const totalPages = Math.ceil(dataStore.size / summaryItemsPerPage);
            if (summaryCurrentPage < totalPages - 1) {
                summaryCurrentPage++;
                renderSummaryGrid();
                saveState();
            }
        });
        domElements.diagIdSelector.addEventListener('change', (event) => {
            updateDisplay(event.target.value);
            saveState();
        });

        domElements.summaryGrid.addEventListener('click', (event) => {
            const card = event.target.closest('.summary-card');
            if (card && card.dataset.id) {
                const clickedId = card.dataset.id;
                domElements.diagIdSelector.value = clickedId;
                updateDisplay(clickedId);
                saveState();
            }
        });

        const ws = new WebSocket(`${window.location.protocol === 'https:' ? 'wss:' : 'ws:'}//${window.location.host}/socket`);
        ws.onopen = () => domElements.status.textContent = 'Connected';
        ws.onclose = () => domElements.status.textContent = 'Disconnected';
        ws.onerror = () => domElements.status.textContent = 'Connection Error';
        ws.onmessage = (event) => {
            const allValues = event.data.split(',');
            if (allValues.length === 0 || allValues[0] === '' || allValues.length % fieldsPerDataset !== 0) return;
            const numDatasets = allValues.length / fieldsPerDataset;
            for (let i = 0; i < numDatasets; i++) {
                processDataset(allValues.slice(i * fieldsPerDataset, (i + 1) * fieldsPerDataset));
            }
            renderSummaryGrid();
            updateDisplay(domElements.diagIdSelector.value);
            saveState();
        };
    };

    init();
});

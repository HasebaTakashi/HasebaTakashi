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
        dataDisplay: document.getElementById('data-display'),
    };

    // --- State Variables ---
    let dataStore = new Map();
    let summaryCurrentPage = 0;
    const summaryItemsPerPage = 4;
    const maxHistory = 100;
    const settingFieldNames = ["DetectID", "DetectName", "AnalyzeID", "OperationMode", "LearningNo", "MoveAveNo", "Coefficient1", "Coefficient2", "Coefficient3", "Threshold1", "Threshold2", "Threshold3", "Enable"];
    const realtimeFieldNames = ["DetectID", "OperationMode", "CurrentLearningNo", "DetectLevel", "CalculationResult"];

    // --- Chart.js Initialization ---
    const resultChart = new Chart(domElements.chartCanvas.getContext('2d'), { type: 'line', data: { labels: Array.from({ length: maxHistory }, (_, i) => i + 1), datasets: [ { label: '計算結果', data: [], borderColor: 'rgba(75, 192, 192, 1)', borderWidth: 2, fill: false, tension: 0.1 }, { label: 'レベル1しきい値', data: [], borderColor: 'rgba(255, 206, 86, 1)', borderWidth: 1, borderDash: [5, 5], fill: false, pointRadius: 0 }, { label: 'レベル2しきい値', data: [], borderColor: 'rgba(255, 159, 64, 1)', borderWidth: 1, borderDash: [5, 5], fill: false, pointRadius: 0 }, { label: 'レベル3しきい値', data: [], borderColor: 'rgba(255, 99, 132, 1)', borderWidth: 1, borderDash: [5, 5], fill: false, pointRadius: 0 }, ]}, options: { scales: { y: { beginAtZero: true } }, animation: { duration: 0 } } });

    // --- Main Functions ---
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
            if (!stored || !stored.settings) continue;
            const s = stored.settings;
            const resultText = (s.CalculationResult !== undefined && s.CalculationResult !== null)
                ? s.CalculationResult.toFixed(4)
                : 'N/A';
            const cardHTML = `<div class="summary-card" data-id="${s.DetectID}"><div class="summary-card-header"><span class="summary-card-id">${s.DetectID}</span><span class="lamp level-${s.DetectLevel}"></span></div><div class="summary-card-name editable-name">${s.DetectName}</div><div class="summary-card-result">${resultText}</div></div>`;
            domElements.summaryGrid.insertAdjacentHTML('beforeend', cardHTML);
        }
        domElements.summaryPrevBtn.disabled = summaryCurrentPage === 0;
        domElements.summaryNextBtn.disabled = endIndex >= allIds.length;
    };

    const updateDisplay = (diagId) => {
        const stored = dataStore.get(diagId);
        if (!stored || !stored.settings) return;
        const s = stored.settings;
        domElements.diagId.textContent = s.DetectID ?? 'N/A';
        domElements.mode.textContent = (s.OperationMode !== undefined) ? (s.OperationMode === 1 ? `学習 (${s.OperationMode})` : `診断 (${s.OperationMode})`) : 'N/A';
        domElements.learnedCount.textContent = s.CurrentLearningNo ?? 'N/A';
        domElements.targetCount.textContent = s.LearningNo ?? 'N/A';
        const levelNum = s.DetectLevel ?? 0;
        domElements.diagLevelLamp.className = `lamp level-${levelNum}`;
        domElements.diagLevelText.textContent = `レベル ${levelNum}`;
        domElements.result.textContent = (s.CalculationResult !== undefined && s.CalculationResult !== null) ? s.CalculationResult.toFixed(6) : 'N/A';
        domElements.threshold1.textContent = s.Threshold1 ?? 'N/A';
        domElements.threshold2.textContent = s.Threshold2 ?? 'N/A';
        domElements.threshold3.textContent = s.Threshold3 ?? 'N/A';
        resultChart.data.datasets[0].data = stored.resultsHistory;
        const fill = (v) => Array(maxHistory).fill(v);
        resultChart.data.datasets[1].data = fill(s.Threshold1);
        resultChart.data.datasets[2].data = fill(s.Threshold2);
        resultChart.data.datasets[3].data = fill(s.Threshold3);
        resultChart.update('none');
    };

    const processSettingsData = (csvString) => {
        const allValues = csvString.split(',');
        const numDatasets = allValues.length / settingFieldNames.length;
        for (let i = 0; i < numDatasets; i++) {
            const settingsArray = allValues.slice(i * settingFieldNames.length, (i + 1) * settingFieldNames.length);
            const settings = {};
            settingFieldNames.forEach((name, j) => {
                const val = settingsArray[j];
                settings[name] = (name === "DetectName") ? val : parseFloat(val) || 0;
            });
            const diagId = settings.DetectID.toString();
            if (!dataStore.has(diagId)) dataStore.set(diagId, { settings: null, resultsHistory: [] });
            dataStore.get(diagId).settings = settings;
        }
    };

    const processRealtimeData = (csvString) => {
        const allValues = csvString.split(',');
        const numDatasets = allValues.length / realtimeFieldNames.length;
        for (let i = 0; i < numDatasets; i++) {
            const realtimeArray = allValues.slice(i * realtimeFieldNames.length, (i + 1) * realtimeFieldNames.length);
            const realtimeData = {};
            realtimeFieldNames.forEach((name, j) => { realtimeData[name] = parseFloat(realtimeArray[j]) || 0; });
            const diagId = realtimeData.DetectID.toString();
            const stored = dataStore.get(diagId);
            if (stored && stored.settings) {
                Object.assign(stored.settings, realtimeData);
                stored.resultsHistory.push(realtimeData.CalculationResult);
                if (stored.resultsHistory.length > maxHistory) stored.resultsHistory.shift();
            }
        }
    };

    const saveSettingsToMqtt = async () => {
        const settingsToSave = {};
        const sortedIds = Array.from(dataStore.keys()).sort((a, b) => a - b);
        for(const id of sortedIds) {
            const data = dataStore.get(id);
            if (data && data.settings) {
                const key = data.settings.DetectName || `Detect${id}`;
                settingsToSave[key] = data.settings;
            }
        }
        try {
            const res = await fetch('/api/publish_settings', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(settingsToSave) });
            if (!res.ok) throw new Error('Failed to publish settings');
        } catch (e) { console.error("Failed to publish settings", e); }
    };

    const init = () => {
        // Load state from session storage
        try {
            const savedStore = sessionStorage.getItem('dataStore');
            if (savedStore) dataStore = new Map(JSON.parse(savedStore));
            summaryCurrentPage = parseInt(sessionStorage.getItem('summaryCurrentPage') || '0', 10);
        } catch (e) { dataStore = new Map(); }

        const sortedIds = Array.from(dataStore.keys()).sort((a, b) => a - b);
        domElements.diagIdSelector.innerHTML = '';
        sortedIds.forEach(key => {
            const option = document.createElement('option');
            option.value = key;
            option.textContent = key;
            domElements.diagIdSelector.appendChild(option);
        });
        const savedId = sessionStorage.getItem('selectedDiagId');
        if (savedId && dataStore.has(savedId)) domElements.diagIdSelector.value = savedId;

        renderSummaryGrid();
        updateDisplay(domElements.diagIdSelector.value);

        // --- Event Listeners ---
        addEventListeners();

        // --- WebSocket Connection ---
        const ws = new WebSocket(`${window.location.protocol === 'https:' ? 'wss:' : 'ws:'}//${window.location.host}/socket`);
        ws.onopen = () => { domElements.status.textContent = 'Connected'; fetch('/api/request_settings', { method: 'POST' }); };
        ws.onclose = () => domElements.status.textContent = 'Disconnected';
        ws.onerror = () => domElements.status.textContent = 'Connection Error';
        ws.onmessage = (event) => {
            const message = JSON.parse(event.data);
            if (message.type === 'settings_data') {
                processSettingsData(message.data);
                const sortedIds = Array.from(dataStore.keys()).sort((a,b)=>a-b);
                const currentVal = domElements.diagIdSelector.value;
                domElements.diagIdSelector.innerHTML = '';
                sortedIds.forEach(id => {
                    const option = document.createElement('option');
                    option.value = id;
                    option.textContent = id;
                    domElements.diagIdSelector.appendChild(option);
                });
                if(dataStore.has(currentVal)) domElements.diagIdSelector.value = currentVal;
            } else if (message.type === 'detect_result') {
                processRealtimeData(message.data);
            }
            renderSummaryGrid();
            updateDisplay(domElements.diagIdSelector.value);
            saveState();
        };
    };

    const addEventListeners = () => {
        domElements.summaryPrevBtn.addEventListener('click', () => { if (summaryCurrentPage > 0) { summaryCurrentPage--; renderSummaryGrid(); saveState(); } });
        domElements.summaryNextBtn.addEventListener('click', () => { const totalPages = Math.ceil(dataStore.size / summaryItemsPerPage); if (summaryCurrentPage < totalPages - 1) { summaryCurrentPage++; renderSummaryGrid(); saveState(); } });
        domElements.diagIdSelector.addEventListener('change', (event) => { updateDisplay(event.target.value); saveState(); });

        domElements.summaryGrid.addEventListener('click', (event) => {
            const card = event.target.closest('.summary-card');
            if (!card) return;

            const diagId = card.dataset.id;
            const nameEl = event.target.closest('.editable-name');

            if (nameEl) {
                // Click was on the name, start editing
                enterNameEditMode(nameEl, diagId);
            } else {
                // Click was on the card but not the name, switch the view
                if (diagId) {
                    domElements.diagIdSelector.value = diagId;
                    updateDisplay(diagId);
                    saveState();
                }
            }
        });

        domElements.dataDisplay.addEventListener('click', (event) => {
            const card = event.target.closest('.info-card');
            if (!card) return;
            if (card.classList.contains('toggle-mode-card')) {
                toggleOperationMode();
            } else if (card.classList.contains('editable-card')) {
                if (card.querySelector('.edit-container')) return;
                const valueEl = card.querySelector('.value');
                const propertyToEdit = card.dataset.property;
                enterThresholdEditMode(card, valueEl, propertyToEdit);
            }
        });
    };

    const enterNameEditMode = (nameEl, diagId) => {
        const originalValue = nameEl.textContent;
        nameEl.style.display = 'none';
        const input = document.createElement('input');
        input.type = 'text';
        input.value = originalValue;
        nameEl.parentNode.insertBefore(input, nameEl.nextSibling);
        input.focus();
        const exit = (save) => {
            nameEl.parentNode.removeChild(input);
            nameEl.style.display = 'block';
            if (save) {
                const newValue = input.value.trim();
                if (newValue && newValue !== originalValue) {
                    const stored = dataStore.get(diagId);
                    if (stored) {
                        stored.settings.DetectName = newValue;
                        saveSettingsToMqtt();
                        renderSummaryGrid();
                        saveState();
                    }
                }
            }
        };
        input.addEventListener('blur', () => exit(false));
        input.addEventListener('keydown', (e) => {
            if (e.key === 'Enter') { e.preventDefault(); exit(true); }
            else if (e.key === 'Escape') { exit(false); }
        });
    };

    const enterThresholdEditMode = (card, valueEl, propertyToEdit) => {
        const originalValue = valueEl.textContent;
        valueEl.style.display = 'none';
        const editContainer = document.createElement('div');
        editContainer.className = 'edit-container';
        const input = document.createElement('input');
        input.type = 'number';
        input.value = originalValue;
        input.step = 'any';
        const saveBtn = document.createElement('button');
        saveBtn.textContent = '✔';
        saveBtn.className = 'edit-btn save';
        const cancelBtn = document.createElement('button');
        cancelBtn.textContent = '✖';
        cancelBtn.className = 'edit-btn cancel';
        editContainer.append(input, saveBtn, cancelBtn);
        card.appendChild(editContainer);
        input.focus();
        input.select();
        const exit = (save) => {
            card.removeChild(editContainer);
            valueEl.style.display = 'block';
            if (save) {
                const newValue = parseFloat(input.value);
                if (isNaN(newValue) || newValue === parseFloat(originalValue)) return;
                const diagId = domElements.diagIdSelector.value;
                const stored = dataStore.get(diagId);
                if (stored) {
                    stored.settings[propertyToEdit] = newValue;
                    saveSettingsToMqtt();
                    updateDisplay(diagId);
                    saveState();
                }
            }
        };
        saveBtn.addEventListener('click', () => exit(true));
        cancelBtn.addEventListener('click', () => exit(false));
        input.addEventListener('keydown', (e) => {
            if (e.key === 'Enter') { e.preventDefault(); exit(true); }
            else if (e.key === 'Escape') { exit(false); }
        });
    };

    const toggleOperationMode = () => {
        const diagId = domElements.diagIdSelector.value;
        const stored = dataStore.get(diagId);
        if (!stored || stored.settings.OperationMode === undefined) return;
        const currentMode = stored.settings.OperationMode;
        const newMode = currentMode === 0 ? 1 : 0;
        if (newMode === 1) {
            if (!confirm("学習モードに切り替えると、現在の学習データはリセットされます。よろしいですか？")) {
                return;
            }
        }
        stored.settings.OperationMode = newMode;
        saveSettingsToMqtt();
        updateDisplay(diagId);
        saveState();
    };

    init();
});

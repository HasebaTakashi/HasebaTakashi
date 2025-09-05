document.addEventListener('DOMContentLoaded', () => {
    const formContainer = document.getElementById('settings-form-container');
    const addBtn = document.getElementById('add-setting-btn');
    const saveBtn = document.getElementById('save-settings-btn');
    const notification = document.getElementById('notification');

    const settingFieldNames = [ "DetectID", "DetectName", "AnalyzeID", "OperationMode", "LearningNo", "MoveAveNo", "Coefficient1", "Coefficient2", "Coefficient3", "Threshold1", "Threshold2", "Threshold3", "Enable" ];

    const createSettingCard = (settingData = {}) => {
        const key = settingData.DetectName || `Detect${settingData.DetectID}`;
        const card = document.createElement('div');
        card.className = 'setting-card';
        card.dataset.id = settingData.DetectID;

        let formContent = `<h3>${key}</h3><div class="card-actions"><button class="btn-delete">この設定を削除</button></div><div class="form-grid">`;
        settingFieldNames.forEach(prop => {
            const value = settingData[prop] !== undefined ? settingData[prop] : '';
            const inputType = (prop === 'DetectName') ? 'text' : 'number';
            const step = (prop.includes('Threshold') || prop.includes('Coefficient')) ? 'any' : '1';
            formContent += `<div class="form-item"><label>${prop}</label><input type="${inputType}" value="${value}" step="${step}" data-prop="${prop}"></div>`;
        });
        formContent += `</div>`;
        card.innerHTML = formContent;
        return card;
    };

    const renderSettings = (settingsData) => {
        formContainer.innerHTML = '';
        const sortedIds = Object.keys(settingsData).sort((a, b) => settingsData[a].DetectID - settingsData[b].DetectID);

        sortedIds.forEach(key => {
            formContainer.appendChild(createSettingCard(settingsData[key]));
        });
    };

    const showNotification = (message, type) => {
        notification.textContent = message;
        notification.className = `notification ${type}`;
        notification.style.display = 'block';
        setTimeout(() => { notification.style.display = 'none'; }, 3000);
    };

    const collectSettingsFromUI = () => {
        const settingsToSave = {};
        formContainer.querySelectorAll('.setting-card').forEach(card => {
            const setting = {};
            card.querySelectorAll('input').forEach(input => {
                const prop = input.dataset.prop;
                const value = input.type === 'number' ? parseFloat(input.value) : input.value;
                setting[prop] = isNaN(value) ? input.value : value;
            });
            const key = setting.DetectName || `Detect${setting.DetectID}`;
            settingsToSave[key] = setting;
        });
        return settingsToSave;
    };

    addBtn.addEventListener('click', () => {
        const cards = formContainer.querySelectorAll('.setting-card');
        const nextId = cards.length > 0 ? Math.max(...Array.from(cards).map(c => parseInt(c.querySelector('[data-prop=DetectID]').value, 10))) + 1 : 1;
        const defaultData = { "DetectID": nextId, "DetectName": `Detect${nextId}`, "AnalyzeID": 1, "OperationMode": 0, "LearningNo": 20, "MoveAveNo": 1, "Coefficient1": 1.5, "Coefficient2": 3.0, "Coefficient3": 6.0, "Threshold1": 0, "Threshold2": 0, "Threshold3": 0, "Enable": 1 };
        formContainer.appendChild(createSettingCard(defaultData));
    });

    saveBtn.addEventListener('click', async () => {
        const settingsToSave = collectSettingsFromUI();
        try {
            const response = await fetch('/api/publish_settings', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(settingsToSave) });
            const result = await response.json();
            if (!response.ok) throw new Error(result.error || 'Failed to save');
            showNotification(result.success, 'success');
            fetch('/api/request_settings', { method: 'POST' });
        } catch (error) { showNotification(error.message, 'error'); }
    });

    formContainer.addEventListener('click', async (event) => {
        if (event.target.classList.contains('btn-delete')) {
            if (!confirm('この設定を本当に削除しますか？')) return;
            const card = event.target.closest('.setting-card');
            const settingToDelete = {};
            card.querySelectorAll('input').forEach(input => {
                const prop = input.dataset.prop;
                const value = input.type === 'number' ? parseFloat(input.value) : input.value;
                settingToDelete[prop] = isNaN(value) ? input.value : value;
            });
            try {
                const response = await fetch('/api/delete_setting', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(settingToDelete) });
                const result = await response.json();
                if (!response.ok) throw new Error(result.error || 'Failed to delete');
                showNotification(result.success, 'success');
                fetch('/api/request_settings', { method: 'POST' });
            } catch (error) { showNotification(error.message, 'error'); }
        }
    });

    const ws = new WebSocket(`${window.location.protocol === 'https:' ? 'wss:' : 'ws:'}//${window.location.host}/socket`);
    ws.onopen = () => fetch('/api/request_settings', { method: 'POST' });
    ws.onmessage = (event) => {
        const message = JSON.parse(event.data);
        if (message.type === 'settings_data') {
            const allValues = message.data.split(',');
            const settings = {};
            const numDatasets = allValues.length / settingFieldNames.length;
            for (let i = 0; i < numDatasets; i++) {
                const settingsArray = allValues.slice(i * settingFieldNames.length, (i + 1) * settingFieldNames.length);
                const settingData = {};
                settingFieldNames.forEach((name, j) => {
                    const val = settingsArray[j];
                    settingData[name] = (name === "DetectName") ? val : parseFloat(val);
                });
                const key = settingData.DetectName || `Detect${settingData.DetectID}`;
                settings[key] = settingData;
            }
            renderSettings(settings);
        }
    };
});

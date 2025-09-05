document.addEventListener('DOMContentLoaded', () => {
    const formContainer = document.getElementById('settings-form-container');
    const addBtn = document.getElementById('add-setting-btn');
    const saveBtn = document.getElementById('save-settings-btn');
    const notification = document.getElementById('notification');

    const settingKeys = [
        "DetectID", "DetectName", "AnalyzeID", "OperationMode",
        "LearningNo", "MoveAveNo", "Coefficient1", "Coefficient2",
        "Coefficient3", "Threshold1", "Threshold2", "Threshold3", "Enable"
    ];

    const createSettingCard = (key, settingData = {}) => {
        const card = document.createElement('div');
        card.className = 'setting-card';
        // Use the DetectID for the dataset key to ensure consistency
        card.dataset.id = settingData.DetectID || key;

        let formContent = `<h3>${settingData.DetectName || key}</h3><div class="form-grid">`;
        settingKeys.forEach(prop => {
            const value = settingData[prop] !== undefined ? settingData[prop] : '';
            const inputType = (prop === 'DetectName') ? 'text' : 'number';
            const step = (prop.includes('Threshold') || prop.includes('Coefficient')) ? 'any' : '1';

            formContent += `
                <div class="form-item">
                    <label for="${key}-${prop}">${prop}</label>
                    <input type="${inputType}" id="${key}-${prop}" name="${prop}" value="${value}" step="${step}" data-prop="${prop}">
                </div>
            `;
        });
        formContent += `</div>`;
        card.innerHTML = formContent;
        return card;
    };

    const loadSettingsFromStorage = () => {
        try {
            const savedStore = sessionStorage.getItem('dataStore');
            if (savedStore) {
                const dataStore = new Map(JSON.parse(savedStore));
                formContainer.innerHTML = '';
                const sortedIds = Array.from(dataStore.keys()).sort((a, b) => a - b);
                for (const id of sortedIds) {
                    const data = dataStore.get(id);
                    if (data && data.settings) {
                        const card = createSettingCard(data.settings.DetectName, data.settings);
                        formContainer.appendChild(card);
                    }
                }
            } else {
                showNotification('メインページでデータを一度受信してから、このページを開いてください。', 'error');
            }
        } catch (error) {
            showNotification('設定の読み込みに失敗しました: ' + error.message, 'error');
        }
    };

    const showNotification = (message, type) => {
        notification.textContent = message;
        notification.className = `notification ${type}`;
        notification.style.display = 'block';
        setTimeout(() => {
            notification.style.display = 'none';
        }, 3000);
    };

    addBtn.addEventListener('click', () => {
        const nextId = formContainer.children.length > 0
            ? Math.max(...Array.from(formContainer.children).map(c => parseInt(c.dataset.id, 10))) + 1
            : 1;

        const newKey = `Detect${nextId}`;
        const defaultData = {
            "DetectID": nextId, "DetectName": newKey, "AnalyzeID": 1, "OperationMode": 0,
            "LearningNo": 20, "MoveAveNo": 1, "Coefficient1": 1.5, "Coefficient2": 3.0, "Coefficient3": 6.0,
            "Threshold1": 0, "Threshold2": 0, "Threshold3": 0, "Enable": 1
        };
        const card = createSettingCard(newKey, defaultData);
        formContainer.appendChild(card);
    });

    saveBtn.addEventListener('click', async () => {
        const settingsToSave = {};
        const cards = formContainer.querySelectorAll('.setting-card');

        cards.forEach(card => {
            const setting = {};
            const inputs = card.querySelectorAll('input');
            inputs.forEach(input => {
                const prop = input.dataset.prop;
                const value = input.type === 'number' ? parseFloat(input.value) : input.value;
                setting[prop] = isNaN(value) ? input.value : value;
            });
            // Use DetectName as the key for the JSON object
            const key = setting.DetectName || `Detect${setting.DetectID}`;
            settingsToSave[key] = setting;
        });

        try {
            const response = await fetch('/api/publish_settings', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(settingsToSave)
            });
            const result = await response.json();
            if (!response.ok) throw new Error(result.error || 'Failed to save settings');
            showNotification(result.success, 'success');
        } catch (error) {
            showNotification(error.message, 'error');
        }
    });

    loadSettingsFromStorage();
});

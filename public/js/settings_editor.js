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
        card.dataset.key = key;

        let formContent = `<h3>${key}</h3><div class="form-grid">`;
        settingKeys.forEach(prop => {
            const value = settingData[prop] !== undefined ? settingData[prop] : '';
            const inputType = (typeof value === 'number') ? 'number' : 'text';
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

    const loadSettings = async () => {
        try {
            const response = await fetch('/api/settings');
            if (!response.ok) throw new Error('Failed to fetch settings');
            const settings = await response.json();
            formContainer.innerHTML = '';
            for (const key in settings) {
                const card = createSettingCard(key, settings[key]);
                formContainer.appendChild(card);
            }
        } catch (error) {
            showNotification(error.message, 'error');
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
        const newKey = `Detect${formContainer.children.length + 1}`;
        const defaultData = {
            "DetectID": formContainer.children.length + 1, "DetectName": newKey, "AnalyzeID": 1, "OperationMode": 0,
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
            const key = card.dataset.key;
            const setting = {};
            const inputs = card.querySelectorAll('input');
            inputs.forEach(input => {
                const prop = input.dataset.prop;
                const value = input.type === 'number' ? parseFloat(input.value) : input.value;
                setting[prop] = isNaN(value) ? input.value : value;
            });
            settingsToSave[key] = setting;
        });

        try {
            const response = await fetch('/api/settings', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(settingsToSave, null, 2)
            });
            const result = await response.json();
            if (!response.ok) throw new Error(result.error || 'Failed to save settings');
            showNotification(result.success, 'success');
        } catch (error) {
            showNotification(error.message, 'error');
        }
    });

    // Initial load
    loadSettings();
});

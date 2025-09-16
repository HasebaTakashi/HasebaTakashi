document.addEventListener('DOMContentLoaded', () => {
    const dataDisplay = document.getElementById('data-display');
    const thresholdsList = document.getElementById('thresholds-list').getElementsByTagName('span');
    const updateForm = document.getElementById('update-form');
    const updateStatus = document.getElementById('update-status');

    const protocol = window.location.protocol === 'https:' ? 'wss' : 'ws';
    const ws = new WebSocket(`${protocol}://${window.location.host}`);

    ws.onopen = () => {
        console.log('Connected to WebSocket server');
    };

    ws.onmessage = (event) => {
        try {
            const message = JSON.parse(event.data);
            console.log('Received message:', message);

            if (message.type === 'data') {
                const p = document.createElement('p');
                p.textContent = `[${new Date().toLocaleTimeString()}] ${message.payload}`;
                // Prepend new data to keep the list updated, and limit the list size
                dataDisplay.prepend(p);
                if (dataDisplay.children.length > 20) {
                    dataDisplay.removeChild(dataDisplay.lastChild);
                }
                 if(dataDisplay.firstChild.textContent.includes("Waiting for data...")){
                    dataDisplay.removeChild(dataDisplay.lastChild);
                }
            } else if (message.type === 'setting') {
                const thresholds = message.payload;
                if (Array.isArray(thresholds) && thresholds.length === 3) {
                    thresholdsList[0].textContent = thresholds[0];
                    thresholdsList[1].textContent = thresholds[1];
                    thresholdsList[2].textContent = thresholds[2];
                }
            }
        } catch (error) {
            console.error('Error parsing WebSocket message or updating UI:', error);
        }
    };

    ws.onclose = () => {
        console.log('Disconnected from WebSocket server');
        dataDisplay.innerHTML = '<p>Connection lost. Please refresh.</p>';
    };

    ws.onerror = (error) => {
        console.error('WebSocket error:', error);
        dataDisplay.innerHTML = '<p>Connection error. Please refresh.</p>';
    };

    updateForm.addEventListener('submit', async (e) => {
        e.preventDefault();
        const id = e.target.id.value;
        const value = e.target.value.value;

        updateStatus.textContent = 'Sending update...';

        try {
            const response = await fetch('/settingchange', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ id, value }),
            });

            const result = await response.json();

            if (response.ok) {
                updateStatus.textContent = 'Update request sent successfully!';
                updateForm.reset();
            } else {
                updateStatus.textContent = `Error: ${result.error}`;
            }
        } catch (error) {
            console.error('Failed to send update request:', error);
            updateStatus.textContent = 'Failed to send request. Check console.';
        }

        setTimeout(() => {
            updateStatus.textContent = '';
        }, 3000);
    });
});

document.addEventListener('DOMContentLoaded', () => {
    const status = document.getElementById('status');
    const diagId = document.getElementById('diag-id');
    const mode = document.getElementById('mode');
    const learnedCount = document.getElementById('learned-count');
    const targetCount = document.getElementById('target-count');
    const diagLevel = document.getElementById('diag-level');
    const result = document.getElementById('result');
    const threshold1 = document.getElementById('threshold-1');
    const threshold2 = document.getElementById('threshold-2');
    const threshold3 = document.getElementById('threshold-3');

    const wsProtocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${wsProtocol}//${window.location.host}/socket`;

    const ws = new WebSocket(wsUrl);

    ws.onopen = () => {
        console.log('Connected to WebSocket server');
        status.textContent = 'Connected';
        status.style.color = 'green';
    };

    ws.onmessage = (event) => {
        console.log('Received data:', event.data);
        const data = event.data.split(',');

        if (data.length < 9) {
            console.error('Invalid data format received');
            return;
        }

        // データフィールドの更新
        diagId.textContent = data[0];

        // モード番号 (1:学習, 0:診断)
        const modeNum = data[1];
        mode.textContent = modeNum === '1' ? `学習 (${modeNum})` : `診断 (${modeNum})`;

        learnedCount.textContent = data[2];
        targetCount.textContent = data[3];

        // 診断レベル (0-3)
        const levelNum = data[4];
        diagLevel.textContent = `レベル ${levelNum}`;
        diagLevel.className = `value level-${levelNum}`; // スタイルを動的に変更

        result.textContent = parseFloat(data[5]).toFixed(6); // 小数点以下6桁にフォーマット
        threshold1.textContent = data[6];
        threshold2.textContent = data[7];
        threshold3.textContent = data[8];
    };

    ws.onclose = () => {
        console.log('Disconnected from WebSocket server');
        status.textContent = 'Disconnected. Please refresh the page to reconnect.';
        status.style.color = 'red';
    };

    ws.onerror = (error) => {
        console.error('WebSocket Error:', error);
        status.textContent = 'Connection Error.';
        status.style.color = 'red';
    };
});

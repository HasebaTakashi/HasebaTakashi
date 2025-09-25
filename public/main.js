document.addEventListener('DOMContentLoaded', () => {
    // WebSocketのセットアップ
    const wsProtocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${wsProtocol}//${window.location.host}/`;
    const socket = new WebSocket(wsUrl);

    socket.onopen = () => {
        console.log('WebSocket connection established');
    };

    socket.onclose = () => {
        console.log('WebSocket connection closed');
    };

    socket.onerror = (error) => {
        console.error('WebSocket error:', error);
    };

    // UI要素の取得
    const dataDisplays = {
        'data-1': document.getElementById('data-1'),
        'data-2': document.getElementById('data-2'),
        'data-3': document.getElementById('data-3'),
    };
    const configOutput = document.getElementById('config-output');
    const buttons = document.querySelectorAll('.btn');
    const checkboxes = {
        'data-1': document.getElementById('check-data-1'),
        'data-2': document.getElementById('check-data-2'),
        'data-3': document.getElementById('check-data-3'),
    };

    // Chart.jsのセットアップ
    const ctx = document.getElementById('trendChart').getContext('2d');
    const trendChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: [], // X軸のラベル（時刻など）
            datasets: [
                {
                    label: 'Data 1',
                    data: [],
                    borderColor: 'rgba(255, 99, 132, 1)',
                    backgroundColor: 'rgba(255, 99, 132, 0.2)',
                    hidden: false,
                },
                {
                    label: 'Data 2',
                    data: [],
                    borderColor: 'rgba(54, 162, 235, 1)',
                    backgroundColor: 'rgba(54, 162, 235, 0.2)',
                    hidden: false,
                },
                {
                    label: 'Data 3',
                    data: [],
                    borderColor: 'rgba(75, 192, 192, 1)',
                    backgroundColor: 'rgba(75, 192, 192, 0.2)',
                    hidden: false,
                }
            ]
        },
        options: {
            scales: {
                x: {
                    type: 'time',
                    time: {
                        unit: 'second',
                        displayFormats: {
                            second: 'HH:mm:ss'
                        }
                    },
                    ticks: {
                        color: '#e0e0e0'
                    },
                    grid: {
                        color: '#444'
                    }
                },
                y: {
                    beginAtZero: true,
                    ticks: {
                        color: '#e0e0e0'
                    },
                    grid: {
                        color: '#444'
                    }
                }
            },
            plugins: {
                legend: {
                    labels: {
                        color: '#e0e0e0'
                    }
                }
            },
            maintainAspectRatio: false
        }
    });

    // サーバーからのメッセージ受信処理
    socket.onmessage = (event) => {
        console.log('Received from server:', event.data);
        const data = JSON.parse(event.data);
        const message = JSON.parse(data.message);

        if (data.topic === '/data/display') {
            updateDisplayData(message);
        } else if (data.topic === '/data/config') {
            updateConfigDisplay(message);
        }
    };

    function updateDisplayData(displayData) {
        const timestamp = new Date();

        Object.keys(displayData).forEach((key, index) => {
            if (dataDisplays[key]) {
                const value = parseFloat(displayData[key]);
                dataDisplays[key].textContent = value.toFixed(2);

                // グラフにデータを追加
                if (trendChart.data.datasets[index]) {
                    trendChart.data.datasets[index].data.push({ x: timestamp, y: value });
                }
            }
        });

        // X軸にタイムスタンプを追加
        // trendChart.data.labels.push(timestamp.toLocaleTimeString());

        // 古いデータをグラフから削除（例: 50件以上保持しない）
        const maxDataPoints = 50;
        trendChart.data.datasets.forEach(dataset => {
            if (dataset.data.length > maxDataPoints) {
                dataset.data.shift();
            }
        });
        // if (trendChart.data.labels.length > maxDataPoints) {
        //     trendChart.data.labels.shift();
        // }

        trendChart.update();
    }

    function updateConfigDisplay(configData) {
        configOutput.textContent = JSON.stringify(configData, null, 2);
    }

    // ボタンのクリックイベント
    buttons.forEach(button => {
        button.addEventListener('click', () => {
            const message = {
                event: 'button_press',
                id: button.id
            };
            socket.send(JSON.stringify(message));
            console.log('Sent to server:', message);
        });
    });

    // チェックボックスの変更イベント
    Object.keys(checkboxes).forEach((key, index) => {
        checkboxes[key].addEventListener('change', (e) => {
            if (trendChart.data.datasets[index]) {
                trendChart.data.datasets[index].hidden = !e.target.checked;
                trendChart.update();
            }
        });
    });
});
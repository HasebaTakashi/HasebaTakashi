#include <stdio.h>
#include "sampling_manager.h"
#include "sampling_queue.h"
#include "command_server.h"

#define SERVER_PORT 8888

int main(void) {
    printf("--- C-based Data Acquisition Server ---\n");

    // 1. デバイスとチャンネルの設定を定義
    DeviceSetting device_settings[] = {
        { .id = VMONITOR2_BOARD_ID, .name = "VMonitor2-Device", .channel_no = 17, .enable = true }
    };
    int num_device_settings = sizeof(device_settings) / sizeof(device_settings[0]);

    ChannelSetting channel_settings[17];
    for (int i = 0; i < 16; ++i) {
        channel_settings[i] = (ChannelSetting){ .channel_id = i + 1, .device_id = VMONITOR2_BOARD_ID, .ch_index = i + 1, .sampling_freq = 25600, .gain = 1.0, .input_type = 0, };
        sprintf(channel_settings[i].channel_name, "AD Channel %d", i + 1);
    }
    channel_settings[16] = (ChannelSetting){ .channel_id = 17, .device_id = VMONITOR2_BOARD_ID, .ch_index = 17, .sampling_freq = 1, .gain = 1.0, .input_type = 0, .channel_name = "Pulse Counter" };
    int num_channel_settings = 17;

    // 2. データキューを作成
    printf("Creating data queue...\n");
    SamplingQueue* data_queue = queue_create("MainQueue", 100);
    if (!data_queue) {
        fprintf(stderr, "Error: Failed to create data queue.\n");
        return 1;
    }

    // 3. サンプリングマネージャーを作成
    printf("Creating sampling manager...\n");
    SamplingManager* manager = sampling_manager_create(
        device_settings, num_device_settings,
        channel_settings, num_channel_settings,
        &data_queue, 1
    );
    if (!manager) {
        fprintf(stderr, "Error: Failed to create sampling manager.\n");
        queue_destroy(data_queue);
        return 1;
    }

    // 4. コマンドサーバーを作成
    printf("Creating command server on port %d...\n", SERVER_PORT);
    CommandServer* server = command_server_create(SERVER_PORT, manager);
    if (!server) {
        fprintf(stderr, "Error: Failed to create command server.\n");
        sampling_manager_destroy(manager);
        queue_destroy(data_queue);
        return 1;
    }

    // 5. 各コンポーネントを開始
    printf("Starting sampling manager...\n");
    if (!sampling_manager_start(manager)) {
        // ... エラー処理 ...
        return 1;
    }
    printf("Starting command server...\n");
    if (!command_server_start(server)) {
        // ... エラー処理 ...
        return 1;
    }

    // 6. ユーザーの入力を待つ
    printf("\n>>> Server is running. Press Enter to stop.\n");
    getchar();

    // 7. 各コンポーネントを停止し、リソースを解放する
    printf("\nStopping server and manager...\n");
    command_server_stop(server);
    sampling_manager_stop(manager);

    printf("Cleaning up resources...\n");
    command_server_destroy(server);
    sampling_manager_destroy(manager);
    queue_destroy(data_queue);

    printf("Application finished cleanly.\n");
    return 0;
}

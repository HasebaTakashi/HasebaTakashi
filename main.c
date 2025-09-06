#include <stdio.h>
#include "sampling_manager.h"
#include "sampling_queue.h"
#include "command_server.h"
#include "logger.h"

#define SERVER_PORT 8888
#define LOG_DIR "../AppData/log"
#define LOG_FILENAME "driver.log"
#define LOG_MAX_FILES 10
#define LOG_MAX_SIZE_MB 3

int main(void) {
    // 1. ロガーを初期化
    if (!logger_init(LOG_DIR, LOG_FILENAME, LOG_MAX_SIZE_MB * 1024 * 1024, LOG_MAX_FILES)) {
        fprintf(stderr, "Failed to initialize logger. Exiting.\n");
        return 1;
    }

    LOG_INFO("MAIN", "--- C-based Data Acquisition Server ---");

    // 2. デバイスとチャンネルの設定を定義
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

    // 3. データキューを作成
    LOG_INFO("MAIN", "Creating data queue...");
    SamplingQueue* data_queue = queue_create("MainQueue", 100);
    if (!data_queue) {
        LOG_FATAL("MAIN", "Failed to create data queue.");
        logger_close();
        return 1;
    }

    // 4. サンプリングマネージャーを作成
    LOG_INFO("MAIN", "Creating sampling manager...");
    SamplingManager* manager = sampling_manager_create(
        device_settings, num_device_settings,
        channel_settings, num_channel_settings,
        &data_queue, 1
    );
    if (!manager) {
        LOG_FATAL("MAIN", "Failed to create sampling manager.");
        queue_destroy(data_queue);
        logger_close();
        return 1;
    }

    // 5. コマンドサーバーを作成
    LOG_INFO("MAIN", "Creating command server on port %d...", SERVER_PORT);
    CommandServer* server = command_server_create(SERVER_PORT, manager);
    if (!server) {
        LOG_FATAL("MAIN", "Failed to create command server.");
        sampling_manager_destroy(manager);
        queue_destroy(data_queue);
        logger_close();
        return 1;
    }

    // 6. 各コンポーネントを開始
    LOG_INFO("MAIN", "Starting sampling manager...");
    if (!sampling_manager_start(manager)) {
        LOG_FATAL("MAIN", "Failed to start sampling manager.");
        command_server_destroy(server);
        sampling_manager_destroy(manager);
        queue_destroy(data_queue);
        logger_close();
        return 1;
    }
    LOG_INFO("MAIN", "Starting command server...");
    if (!command_server_start(server)) {
        LOG_FATAL("MAIN", "Failed to start command server.");
        sampling_manager_stop(manager);
        command_server_destroy(server);
        sampling_manager_destroy(manager);
        queue_destroy(data_queue);
        logger_close();
        return 1;
    }

    // 7. ユーザーの入力を待つ
    LOG_INFO("MAIN", "Server is running. Press Enter to stop.");
    getchar();

    // 8. 各コンポーネントを停止し、リソースを解放する
    LOG_INFO("MAIN", "Stopping server and manager...");
    command_server_stop(server);
    sampling_manager_stop(manager);

    LOG_INFO("MAIN", "Cleaning up resources...");
    command_server_destroy(server);
    sampling_manager_destroy(manager);
    queue_destroy(data_queue);

    LOG_INFO("MAIN", "Application finished cleanly.");
    logger_close();
    return 0;
}

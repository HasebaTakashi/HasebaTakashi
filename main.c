#include <stdio.h>
#include "sampling_manager.h"
#include "sampling_queue.h"

int main(void) {
    printf("--- C-based VMonitor Data Acquisition (Refactored) ---\n");

    // 1. 使用するデバイスとチャンネルの設定を定義する
    // 本来は設定ファイルから読み込むが、ここではハードコードする
    DeviceSetting device_settings[] = {
        { .id = VMONITOR2_BOARD_ID, .name = "VMonitor2-Device", .channel_no = 17, .enable = true }
    };
    int num_device_settings = sizeof(device_settings) / sizeof(device_settings[0]);

    ChannelSetting channel_settings[17];
    // アナログ16chぶん設定
    for (int i = 0; i < 16; ++i) {
        channel_settings[i] = (ChannelSetting){
            .channel_id = i + 1,
            .device_id = VMONITOR2_BOARD_ID,
            .ch_index = i + 1,
            .sampling_freq = 25600,
            .gain = 1.0,
            .input_type = 0,
        };
        sprintf(channel_settings[i].channel_name, "AD Channel %d", i + 1);
    }
    // パルス1chぶん設定
    channel_settings[16] = (ChannelSetting){
        .channel_id = 17,
        .device_id = VMONITOR2_BOARD_ID,
        .ch_index = 17,
        .sampling_freq = 1,
        .gain = 1.0,
        .input_type = 0,
        .channel_name = "Pulse Counter"
    };
    int num_channel_settings = 17;


    // 2. データキューを作成する
    printf("Creating data queue...\n");
    SamplingQueue* data_queue = queue_create("MainQueue", 100);
    if (!data_queue) {
        fprintf(stderr, "Error: Failed to create data queue.\n");
        return 1;
    }


    // 3. 設定を渡してサンプリングマネージャーを作成する
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


    // 4. マネージャーを開始する
    printf("Starting sampling manager...\n");
    if (!sampling_manager_start(manager)) {
        fprintf(stderr, "Error: Failed to start sampling manager.\n");
        sampling_manager_destroy(manager);
        queue_destroy(data_queue);
        return 1;
    }


    // 5. ユーザーの入力を待つ
    printf("\n>>> Data acquisition is running in the background.\n");
    printf(">>> Press Enter to stop and exit.\n");
    getchar();


    // 6. マネージャーを停止し、リソースを解放する
    printf("\nStopping sampling manager...\n");
    sampling_manager_stop(manager);
    printf("Cleaning up resources...\n");
    sampling_manager_destroy(manager);
    queue_destroy(data_queue);

    printf("Application finished cleanly.\n");
    return 0;
}

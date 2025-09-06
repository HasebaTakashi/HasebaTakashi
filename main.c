#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "logger.h"
#include "config_manager.h"
#include "sampling_manager.h"
#include "command_server.h"

#define SETTINGS_DIR "./setting"

// シグナルハンドラ用のグローバルなシャットダウンフラグ
static volatile bool g_shutdown_request = false;

// シグナルハンドラ関数
static void handle_signal(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        g_shutdown_request = true;
    }
}

int main(void) {
    // 1. ロガーを初期化
    if (!logger_init("../AppData/log", "driver.log", 3 * 1024 * 1024, 10)) {
        fprintf(stderr, "Failed to initialize logger. Exiting.\n");
        return 1;
    }

    LOG_INFO("MAIN", "--- C-based Data Acquisition Server Starting ---");

    // 2. シグナルハンドラを設定
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // 3. ファイルから全設定を読み込み
    LOG_INFO("MAIN", "Loading all configurations from '%s'...", SETTINGS_DIR);
    AllConfigs* configs = config_load_all(SETTINGS_DIR);
    if (!configs) {
        LOG_FATAL("MAIN", "Failed to load configuration. Shutting down.");
        logger_close();
        return 1;
    }

    // 4. データキューを作成 (app_configに基づいて作成)
    LOG_INFO("MAIN", "Creating %d data queues...", configs->app_config.buffer_no);
    SamplingQueue** queues = (SamplingQueue**)calloc(configs->app_config.buffer_no, sizeof(SamplingQueue*));
    for(int i = 0; i < configs->app_config.buffer_no; ++i) {
        char q_name[32];
        sprintf(q_name, "Queue%d", i + 1);
        queues[i] = queue_create(q_name, configs->app_config.buffer_sizes[i]);
    }

    // 5. サンプリングマネージャーを作成
    LOG_INFO("MAIN", "Creating sampling manager...");
    SamplingManager* manager = sampling_manager_create(
        &configs->app_config,
        configs->device_settings, configs->num_device_settings,
        configs->channel_settings, configs->num_channel_settings,
        queues, configs->app_config.buffer_no
    );
    if (!manager) {
        LOG_FATAL("MAIN", "Failed to create sampling manager.");
        // cleanup...
        config_destroy(configs);
        logger_close();
        return 1;
    }

    // TODO: app_configの他の設定(PulseCountThresholdなど)をmanager経由でデバイスに渡す

    // 6. コマンドサーバーを作成
    LOG_INFO("MAIN", "Creating command server on port %d...", configs->app_config.server_port);
    CommandServer* server = command_server_create(configs->app_config.server_port, manager);
    if (!server) {
        LOG_FATAL("MAIN", "Failed to create command server.");
        // cleanup...
        config_destroy(configs);
        sampling_manager_destroy(manager);
        logger_close();
        return 1;
    }

    // 7. 各コンポーネントを開始
    LOG_INFO("MAIN", "Starting sampling manager...");
    if (!sampling_manager_start(manager)) {
        LOG_FATAL("MAIN", "Failed to start sampling manager.");
        // cleanup...
        return 1;
    }
    LOG_INFO("MAIN", "Starting command server...");
    if (!command_server_start(server)) {
        LOG_FATAL("MAIN", "Failed to start command server.");
        // cleanup...
        return 1;
    }

    // 8. シャットダウンシグナルを待つ
    LOG_INFO("MAIN", "Server is running. Waiting for shutdown signal (Ctrl+C)...");
    while (!g_shutdown_request) {
        sleep(1); // 1秒ごとにチェック
    }

    // 9. 各コンポーネントを停止し、リソースを解放する
    LOG_INFO("MAIN", "Shutdown signal received. Stopping server and manager...");
    command_server_stop(server);
    sampling_manager_stop(manager);

    LOG_INFO("MAIN", "Cleaning up resources...");
    command_server_destroy(server);
    sampling_manager_destroy(manager);
    for(int i = 0; i < configs->app_config.buffer_no; ++i) {
        queue_destroy(queues[i]);
    }
    free(queues);
    config_destroy(configs);

    LOG_INFO("MAIN", "Application finished cleanly.");
    logger_close();
    return 0;
}

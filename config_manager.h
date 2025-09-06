#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "sampling_manager.h" // For DeviceSetting and ChannelSetting
#include <stdbool.h>

// app_setting.json の内容を保持する構造体
typedef struct AppConfig {
    int buffer_no;
    int buffer_sizes[10]; // バッファは最大10個と仮定
    char server_ip[64];
    int server_port;
    int pulse_count_threshold;
    int idle_reboot_type;
    int idle_reboot_timeout;
} AppConfig;

// 3つの設定すべてを保持するコンテナ構造体
typedef struct {
    AppConfig app_config;
    DeviceSetting* device_settings;
    int num_device_settings;
    ChannelSetting* channel_settings;
    int num_channel_settings;
} AllConfigs;

/**
 * @brief 指定されたディレクトリからすべての設定JSONファイルを読み込む
 *
 * @param settings_dir 設定ファイル (app_setting.jsonなど) が含まれるディレクトリのパス
 * @return AllConfigs* 読み込まれた全設定を含む構造体へのポインタ。
 *                     失敗した場合はNULLを返す。
 *                     呼び出し元は、使用後に config_destroy() でメモリを解放する責任を負う。
 */
AllConfigs* config_load_all(const char* settings_dir);

/**
 * @brief config_load_allで確保されたメモリを解放する
 *
 * @param configs 解放する設定コンテナへのポインタ
 */
void config_destroy(AllConfigs* configs);

#endif // CONFIG_MANAGER_H

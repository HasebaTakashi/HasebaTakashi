#include "sampling_manager.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h> // for usleep
#include <string.h>

#define LOG_INFO(msg, ...) printf("[MGR_INFO] " msg "\n", ##__VA_ARGS__)
#define LOG_ERROR(msg, ...) fprintf(stderr, "[MGR_ERROR] " msg "\n", ##__VA_ARGS__)

// データ収集を実行するスレッド関数
static void* run_loop(void* arg) {
    SamplingManager* manager = (SamplingManager*)arg;
    long no_data_times = 0;
    const long timeout_threshold = (1000000 / CHECK_DATA_INTERVAL_USEC) * NO_DATA_TIME_OUT_SEC;

    LOG_INFO("Data acquisition thread started.");

    while (manager->running) {
        if (vmonitor2_board_check_data()) {
            // データ取得
            if (vmonitor2_board_get_data(manager->board, manager->channels, manager->num_channels)) {
                // 取得したデータをキューに入れる
                SamplingData* data = sampling_data_create(manager->channels, manager->num_channels);
                if (data) {
                    for (int i = 0; i < manager->num_queues; ++i) {
                        queue_enqueue(manager->data_queues[i], data);
                        LOG_INFO("Enqueued data to queue '%s'. Count: %d",
                                 manager->data_queues[i]->name,
                                 queue_data_count(manager->data_queues[i]));
                    }
                }
            }
            no_data_times = 0;
        } else {
            no_data_times++;
            if (no_data_times > timeout_threshold) {
                LOG_ERROR("No data timeout (%ld counts). Restarting device.", no_data_times);
                vmonitor2_board_restart();
                no_data_times = 0;
            }
            struct timespec req = {0, CHECK_DATA_INTERVAL_USEC * 1000L};
            nanosleep(&req, NULL);
        }
    }

    LOG_INFO("Data acquisition thread finished.");
    return NULL;
}

// サンプリング前の準備 (ゲインや入力種別の設定)
static bool prepare_sampling(SamplingManager* manager) {
    char* version = vmonitor2_board_get_version();
    if (version) {
        LOG_INFO("Device Version: %s", version);
        free(version);
    }

    for (int i = 0; i < manager->num_channels; ++i) {
        SamplingChannel* ch = &manager->channels[i];

        // 入力種別の確認と設定
        int current_input = vmonitor2_board_get_input(ch->ch_index);
        if (current_input != ch->input_type) {
            LOG_INFO("CH:%d Index:%d Input mismatch. Current:%d, Target:%d. Setting...",
                     ch->id, ch->ch_index, current_input, ch->input_type);
            if (!vmonitor2_board_set_input(ch->ch_index, ch->input_type)) {
                 LOG_ERROR("Failed to set input for CH:%d", ch->id);
            }
        }

        // ゲインの確認と設定
        double current_gain = vmonitor2_board_get_gain(ch->ch_index);
        if (current_gain != ch->gain) {
            LOG_INFO("CH:%d Index:%d Gain mismatch. Current:%.1f, Target:%.1f. Setting...",
                     ch->id, ch->ch_index, current_gain, ch->gain);
            if (!vmonitor2_board_set_gain(ch->ch_index, ch->gain)) {
                LOG_ERROR("Failed to set gain for CH:%d", ch->id);
            }
        }
    }
    return true;
}

// --- ライフサイクル管理 ---

SamplingManager* sampling_manager_create(int num_queues, SamplingQueue** queues) {
    SamplingManager* manager = (SamplingManager*)malloc(sizeof(SamplingManager));
    if (!manager) return NULL;

    manager->board = vmonitor2_board_create();
    if (!manager->board) {
        free(manager);
        return NULL;
    }

    manager->num_queues = num_queues;
    manager->data_queues = queues;
    manager->running = false;
    manager->num_channels = VMONITOR2_CH_NO;
    manager->channels = (SamplingChannel*)malloc(sizeof(SamplingChannel) * manager->num_channels);
    if (!manager->channels) {
        vmonitor2_board_destroy(manager->board);
        free(manager);
        return NULL;
    }

    // チャンネル情報を初期化 (本来は設定ファイルから読み込む)
    for (int i = 0; i < manager->num_channels; ++i) {
        SamplingChannel* ch = &manager->channels[i];
        ch->id = i + 1;
        ch->ch_index = i + 1;
        ch->device_id = VMONITOR2_BOARD; // 仮のデバイスID
        sprintf(ch->name, "Channel %d", i + 1);
        ch->gain = 1.0; // デフォルトゲイン
        ch->input_type = 0; // デフォルト入力タイプ

        if (ch->ch_index <= VMONITOR2_AD_CH_NO) { // ADチャンネル
            ch->sampling_no = VMONITOR2_SAMPLING_FREQUENCY;
            ch->buffer.ad = (short*)malloc(sizeof(short) * ch->sampling_no);
        } else { // パルスチャンネル
            ch->sampling_no = 1;
            ch->buffer.pulse = (int*)malloc(sizeof(int) * ch->sampling_no);
            ch->gain = 1.0; // パルスチャンネルのゲインは1.0固定
        }
    }
    return manager;
}

void sampling_manager_destroy(SamplingManager* manager) {
    if (!manager) return;
    if (manager->running) {
        sampling_manager_stop(manager);
    }
    vmonitor2_board_destroy(manager->board);
    for (int i = 0; i < manager->num_channels; ++i) {
        if (manager->channels[i].ch_index <= VMONITOR2_AD_CH_NO) {
            free(manager->channels[i].buffer.ad);
        } else {
            free(manager->channels[i].buffer.pulse);
        }
    }
    free(manager->channels);
    free(manager);
}

// --- 制御 ---

bool sampling_manager_start(SamplingManager* manager) {
    if (!vmonitor2_board_open(manager->board)) {
        LOG_ERROR("Failed to open board.");
        return false;
    }
    LOG_INFO("Board opened successfully.");

    if (!prepare_sampling(manager)) {
        LOG_ERROR("Failed to prepare sampling.");
        vmonitor2_board_close();
        return false;
    }
    LOG_INFO("Sampling prepared.");

    if (!vmonitor2_board_start_sampling()) {
        LOG_ERROR("Failed to start sampling.");
        vmonitor2_board_close();
        return false;
    }
    LOG_INFO("Sampling started on board.");

    manager->running = true;
    if (pthread_create(&manager->run_thread, NULL, run_loop, manager) != 0) {
        LOG_ERROR("Failed to create data acquisition thread.");
        manager->running = false;
        vmonitor2_board_stop_sampling();
        vmonitor2_board_close();
        return false;
    }
    return true;
}

void sampling_manager_stop(SamplingManager* manager) {
    if (manager->running) {
        manager->running = false;
        pthread_join(manager->run_thread, NULL);
    }
    vmonitor2_board_stop_sampling();
    vmonitor2_board_close();
    LOG_INFO("Manager stopped.");
}

// --- デバイスへの問い合わせ関数 ---
// 内部でチャンネルIDからch_indexを検索するヘルパー
static SamplingChannel* find_channel_by_id(SamplingManager* manager, int channel_id) {
    for (int i = 0; i < manager->num_channels; ++i) {
        if (manager->channels[i].id == channel_id) {
            return &manager->channels[i];
        }
    }
    return NULL;
}

double sampling_manager_get_terminal_voltage(SamplingManager* manager, int channel_id) {
    SamplingChannel* ch = find_channel_by_id(manager, channel_id);
    if (ch) {
        return vmonitor2_board_get_terminal_voltage(ch->ch_index);
    }
    return -1.0; // Error
}

double sampling_manager_get_gain(SamplingManager* manager, int channel_id) {
    SamplingChannel* ch = find_channel_by_id(manager, channel_id);
    if (ch) {
        return vmonitor2_board_get_gain(ch->ch_index);
    }
    return -1.0;
}

bool sampling_manager_set_gain(SamplingManager* manager, int channel_id, double gain) {
    SamplingChannel* ch = find_channel_by_id(manager, channel_id);
    if (ch) {
        if (vmonitor2_board_set_gain(ch->ch_index, gain)) {
            ch->gain = gain; // 設定が成功したらローカルも更新
            return true;
        }
    }
    return false;
}

// device_id は現時点では1つしかないので未使用だが、将来の拡張のために残す
int sampling_manager_get_digital_in(SamplingManager* manager, int device_id, int ch_index) {
    (void)manager;
    (void)device_id;
    return vmonitor2_board_get_digital_in(ch_index);
}

int sampling_manager_get_digital_out(SamplingManager* manager, int device_id, int ch_index) {
    (void)manager;
    (void)device_id;
    return vmonitor2_board_get_digital_out(ch_index);
}

bool sampling_manager_set_digital_out(SamplingManager* manager, int device_id, int ch_index, int status) {
    (void)manager;
    (void)device_id;
    return vmonitor2_board_set_digital_out(ch_index, status);
}

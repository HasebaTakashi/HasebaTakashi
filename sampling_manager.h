#ifndef SAMPLING_MANAGER_H
#define SAMPLING_MANAGER_H

#include "sampling_device.h"
#include "sampling_queue.h"
#include <pthread.h>
#include <stdbool.h>

#define MAX_DEVICES 8           // マネージャーが扱えるデバイスの最大数
#define MAX_CHANNELS_TOTAL 64   // マネージャーが扱えるチャンネルの総数

// デバイスの設定情報をマネージャーに渡すための構造体
typedef struct {
    DeviceType id;
    char name[64];
    int channel_no;
    bool enable;
} DeviceSetting;

// チャンネルの設定情報をマネージャーに渡すための構造体
typedef struct {
    int channel_id;
    char channel_name[64];
    DeviceType device_id;
    int ch_index;
    int sampling_freq;
    double gain;
    double max_range;
    double min_range;
    int resolution;
    int input_type;
} ChannelSetting;

// データ収集全体を管理する構造体
typedef struct {
    SamplingDevice* devices[MAX_DEVICES];
    int num_devices;

    SamplingChannel* channels[MAX_CHANNELS_TOTAL];
    int num_channels;

    SamplingQueue** data_queues;
    int num_queues;

    pthread_t run_thread;   // データ収集スレッドの識別子
    volatile bool running;  // スレッドの実行状態を制御するフラグ
} SamplingManager;

// --- ライフサイクル管理 ---
SamplingManager* sampling_manager_create(
    DeviceSetting* device_settings, int num_device_settings,
    ChannelSetting* channel_settings, int num_channel_settings,
    SamplingQueue** queues, int num_queues
);
void sampling_manager_destroy(SamplingManager* manager);

// --- 制御 ---
bool sampling_manager_start(SamplingManager* manager);
void sampling_manager_stop(SamplingManager* manager);

// --- デバイスへの問い合わせ関数 ---
double sampling_manager_get_terminal_voltage(SamplingManager* manager, int channel_id);
double sampling_manager_get_gain(SamplingManager* manager, int channel_id);
bool sampling_manager_set_gain(SamplingManager* manager, int channel_id, double gain);
int sampling_manager_get_digital_in(SamplingManager* manager, DeviceType device_id, int ch_index);
int sampling_manager_get_digital_out(SamplingManager* manager, DeviceType device_id, int ch_index);
bool sampling_manager_set_digital_out(SamplingManager* manager, DeviceType device_id, int ch_index, int status);

#endif // SAMPLING_MANAGER_H

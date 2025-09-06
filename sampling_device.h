#ifndef SAMPLING_DEVICE_H
#define SAMPLING_DEVICE_H

#include "device_interface.h"
#include "sampling_channel.h"
#include <stdbool.h>

// 1つのデバイスが持てるチャンネルの最大数
#define MAX_CHANNELS_PER_DEVICE 32

// 抽象デバイスを表す構造体
typedef struct SamplingDevice {
    DeviceType id;
    char name[64];
    int max_channels;

    // このデバイスに割り当てられたチャンネル
    SamplingChannel* attached_channels[MAX_CHANNELS_PER_DEVICE];
    int num_attached_channels;

    // 現在のサイクルでデータを取得したかどうかのフラグ
    bool is_get_data;

    // --- ポリモーフィズム（多態性）を実現する部分 ---
    // 具象デバイスオブジェクトへのポインタ (例: VMonitor2Board*)
    void* handle;
    // デバイスの実装（vtable）へのポインタ
    const DeviceVTable* vtable;
} SamplingDevice;

// --- ライフサイクル管理 ---
SamplingDevice* sampling_device_create(DeviceType id, const char* name, int channel_no);
void sampling_device_destroy(SamplingDevice* device);

// --- チャンネル管理 ---
bool sampling_device_attach_channel(SamplingDevice* device, SamplingChannel* channel);

// --- vtableへのラッパー関数群 ---
bool sampling_device_open(SamplingDevice* device);
void sampling_device_close(SamplingDevice* device);
bool sampling_device_prepare_sampling(SamplingDevice* device);
char* sampling_device_get_version(SamplingDevice* device);
int sampling_device_get_ad_ch_no(SamplingDevice* device);

bool sampling_device_start_sampling(SamplingDevice* device);
void sampling_device_stop_sampling(SamplingDevice* device);
bool sampling_device_check_data(SamplingDevice* device);
bool sampling_device_get_data(SamplingDevice* device);
void sampling_device_get_data_clear(SamplingDevice* device);

double sampling_device_get_terminal_voltage(SamplingDevice* device, int ch_index);
double sampling_device_get_gain(SamplingDevice* device, int ch_index);
bool sampling_device_set_gain(SamplingDevice* device, int ch_index, double gain);

void sampling_device_reset(SamplingDevice* device);
void sampling_device_restart(SamplingDevice* device);

#endif // SAMPLING_DEVICE_H

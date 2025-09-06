#include "sampling_device.h"
#include "vmonitor2_board.h"
#include "dummy_device.h"
#include "logger.h"
#include "config_manager.h" // For AppConfig struct definition
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// --- ライフサイクル管理 ---

SamplingDevice* sampling_device_create(DeviceType id, const char* name, int channel_no, const struct AppConfig* app_config) {
    SamplingDevice* device = (SamplingDevice*)calloc(1, sizeof(SamplingDevice));
    if (!device) {
        LOG_ERROR("DEVICE", "Failed to allocate memory for SamplingDevice");
        return NULL;
    }

    device->id = id;
    strncpy(device->name, name, sizeof(device->name) - 1);
    device->max_channels = channel_no;
    device->is_get_data = false;

    // デバイスIDに基づいて具象オブジェクトとvtableを割り当てる
    switch (id) {
        case VMONITOR2_BOARD_ID:
            device->handle = vmonitor2_board_new(app_config);
            device->vtable = vmonitor2_board_vtable();
            break;
        case DUMMY_DEVICE_ID:
            device->handle = dummy_device_new();
            device->vtable = dummy_device_vtable();
            break;
        default:
            LOG_ERROR("DEVICE", "Unknown device ID: %d", id);
            free(device);
            return NULL;
    }

    if (!device->handle || !device->vtable) {
        LOG_ERROR("DEVICE", "Failed to create concrete device or get vtable for ID: %d", id);
        if (device->handle) {
            // handleは作られたがvtableがなかった場合、handleを解放
            device->vtable->destroy(device->handle);
        }
        free(device);
        return NULL;
    }

    return device;
}

void sampling_device_destroy(SamplingDevice* device) {
    if (!device) return;
    // vtableを通じて具象デバイスのdestroy関数を呼び出す
    if (device->vtable && device->vtable->destroy) {
        device->vtable->destroy(device->handle);
    }
    free(device);
}

// --- チャンネル管理 ---

bool sampling_device_attach_channel(SamplingDevice* device, SamplingChannel* channel) {
    if (device->num_attached_channels >= device->max_channels) {
        LOG_ERROR("DEVICE", "Device '%s' cannot attach more channels (max: %d)", device->name, device->max_channels);
        return false;
    }

    // デバイスがこのチャンネルのサンプリング周波数をサポートしているか確認
    if (!device->vtable->check_sampling_freq(device->handle, channel->ch_index, channel->sampling_no)) {
        LOG_ERROR("DEVICE", "Channel %d sampling freq %d not supported by device '%s'",
                  channel->id, channel->sampling_no, device->name);
        return false;
    }

    device->attached_channels[device->num_attached_channels++] = channel;
    LOG_INFO("DEVICE", "Attached channel %d to device '%s'", channel->id, device->name);
    return true;
}

// --- vtableへのラッパー関数群 ---

bool sampling_device_open(SamplingDevice* device) {
    return device->vtable->open(device->handle);
}

void sampling_device_close(SamplingDevice* device) {
    device->vtable->close(device->handle);
}

bool sampling_device_prepare_sampling(SamplingDevice* device) {
    LOG_INFO("DEVICE", "Preparing sampling for device '%s'...", device->name);
    char* version = device->vtable->get_version(device->handle);
    if (version) {
        LOG_INFO("DEVICE", "Device '%s' version: %s", device->name, version);
        free(version);
    }

    for (int i = 0; i < device->num_attached_channels; ++i) {
        SamplingChannel* ch = device->attached_channels[i];

        // 入力種別の確認と設定
        int current_input = device->vtable->get_input(device->handle, ch->ch_index);
        if (current_input != ch->input_type) {
            LOG_INFO("DEVICE", "CH:%d Input mismatch on '%s'. Current:%d, Target:%d. Setting...",
                     ch->id, device->name, current_input, ch->input_type);
            device->vtable->set_input(device->handle, ch->ch_index, ch->input_type);
        }

        // ゲインの確認と設定
        double current_gain = device->vtable->get_gain(device->handle, ch->ch_index);
        if (current_gain != ch->gain) {
            LOG_INFO("DEVICE", "CH:%d Gain mismatch on '%s'. Current:%.1f, Target:%.1f. Setting...",
                     ch->id, device->name, current_gain, ch->gain);
            device->vtable->set_gain(device->handle, ch->ch_index, ch->gain);
        }
    }
    return true;
}

char* sampling_device_get_version(SamplingDevice* device) {
    return device->vtable->get_version(device->handle);
}

int sampling_device_get_ad_ch_no(SamplingDevice* device) {
    return device->vtable->get_ad_ch_no(device->handle);
}

bool sampling_device_start_sampling(SamplingDevice* device) {
    return device->vtable->start_sampling(device->handle);
}

void sampling_device_stop_sampling(SamplingDevice* device) {
    device->vtable->stop_sampling(device->handle);
}

bool sampling_device_check_data(SamplingDevice* device) {
    return device->vtable->check_data(device->handle);
}

bool sampling_device_get_data(SamplingDevice* device) {
    if (device->vtable->get_data(device->handle, device->attached_channels, device->num_attached_channels)) {
        device->is_get_data = true;
        return true;
    }
    return false;
}

void sampling_device_get_data_clear(SamplingDevice* device) {
    device->is_get_data = false;
}

double sampling_device_get_terminal_voltage(SamplingDevice* device, int ch_index) {
    return device->vtable->get_terminal_voltage(device->handle, ch_index);
}

double sampling_device_get_gain(SamplingDevice* device, int ch_index) {
    return device->vtable->get_gain(device->handle, ch_index);
}

bool sampling_device_set_gain(SamplingDevice* device, int ch_index, double gain) {
    return device->vtable->set_gain(device->handle, ch_index, gain);
}

void sampling_device_reset(SamplingDevice* device) {
    device->vtable->reset(device->handle);
}

void sampling_device_restart(SamplingDevice* device) {
    device->vtable->restart(device->handle);
}

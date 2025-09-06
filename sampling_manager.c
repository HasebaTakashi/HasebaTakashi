#include "sampling_manager.h"
#include "app_config.h"
#include "logger.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

// データ収集を実行するスレッド関数
static void* run_loop(void* arg) {
    SamplingManager* manager = (SamplingManager*)arg;
    long no_data_times = 0;
    const long timeout_threshold = (1000000 / CHECK_DATA_INTERVAL_USEC) * NO_DATA_TIME_OUT_SEC;

    LOG_INFO("MANAGER", "Data acquisition thread started.");

    while (manager->running) {
        for (int i = 0; i < manager->num_devices; ++i) {
            SamplingDevice* device = manager->devices[i];
            if (!device->is_get_data && sampling_device_check_data(device)) {
                sampling_device_get_data(device);
            }
        }

        bool all_data_ready = true;
        if (manager->num_devices == 0) {
            all_data_ready = false;
        }
        for (int i = 0; i < manager->num_devices; ++i) {
            if (!manager->devices[i]->is_get_data) {
                all_data_ready = false;
                break;
            }
        }

        if (all_data_ready) {
            SamplingData* data = sampling_data_create(manager->channels, manager->num_channels);
            if (data) {
                for (int i = 0; i < manager->num_queues; ++i) {
                    queue_enqueue(manager->data_queues[i], data);
                }
            }
            for (int i = 0; i < manager->num_devices; ++i) {
                sampling_device_get_data_clear(manager->devices[i]);
            }
            no_data_times = 0;
        } else {
            no_data_times++;
            if (no_data_times > timeout_threshold) {
                LOG_ERROR("MANAGER", "No data timeout (%ld counts). Restarting all devices.", no_data_times);
                for (int i = 0; i < manager->num_devices; ++i) {
                    sampling_device_restart(manager->devices[i]);
                }
                no_data_times = 0;
            }
            struct timespec req = {0, CHECK_DATA_INTERVAL_USEC * 1000L};
            nanosleep(&req, NULL);
        }
    }
    LOG_INFO("MANAGER", "Data acquisition thread finished.");
    return NULL;
}

#include "config_manager.h" // For AppConfig struct definition

// --- ライフサイクル管理 ---

SamplingManager* sampling_manager_create(
    const struct AppConfig* app_config,
    DeviceSetting* device_settings, int num_device_settings,
    ChannelSetting* channel_settings, int num_channel_settings,
    SamplingQueue** queues, int num_queues
) {
    SamplingManager* manager = (SamplingManager*)calloc(1, sizeof(SamplingManager));
    if (!manager) return NULL;

    manager->data_queues = queues;
    manager->num_queues = num_queues;

    // 有効なデバイスをインスタンス化
    for (int i = 0; i < num_device_settings; ++i) {
        if (device_settings[i].enable) {
            manager->devices[manager->num_devices++] = sampling_device_create(
                device_settings[i].id, device_settings[i].name, device_settings[i].channel_no, app_config);
        }
    }

    // チャンネルをインスタンス化
    for (int i = 0; i < num_channel_settings; ++i) {
        ChannelSetting* cs = &channel_settings[i];
        SamplingChannel* ch = (SamplingChannel*)calloc(1, sizeof(SamplingChannel));
        // ... (チャンネル設定をコピー)
        ch->id = cs->channel_id;
        strncpy(ch->name, cs->channel_name, sizeof(ch->name) - 1);
        ch->device_id = cs->device_id;
        ch->ch_index = cs->ch_index;
        ch->sampling_no = cs->sampling_freq;
        ch->gain = cs->gain;
        ch->input_type = cs->input_type;

        // バッファ確保
        if (ch->sampling_no > 0) {
            if (ch->ch_index == VMONITOR2_CH_NO) { // パルスチャンネルの判定（より汎用的にすべき）
                 ch->buffer.pulse = (int*)malloc(sizeof(int) * ch->sampling_no);
            } else {
                 ch->buffer.ad = (short*)malloc(sizeof(short) * ch->sampling_no);
            }
        }
        manager->channels[manager->num_channels++] = ch;

        // 対応するデバイスにチャンネルをアタッチ
        for (int j = 0; j < manager->num_devices; ++j) {
            if (manager->devices[j]->id == ch->device_id) {
                sampling_device_attach_channel(manager->devices[j], ch);
                break;
            }
        }
    }
    return manager;
}

void sampling_manager_destroy(SamplingManager* manager) {
    if (!manager) return;
    if (manager->running) {
        sampling_manager_stop(manager);
    }
    for (int i = 0; i < manager->num_devices; ++i) {
        sampling_device_destroy(manager->devices[i]);
    }
    for (int i = 0; i < manager->num_channels; ++i) {
        // バッファを解放
        if (manager->channels[i]->sampling_no > 0) {
            if (manager->channels[i]->ch_index == VMONITOR2_CH_NO) {
                free(manager->channels[i]->buffer.pulse);
            } else {
                free(manager->channels[i]->buffer.ad);
            }
        }
        free(manager->channels[i]);
    }
    free(manager);
}

// --- 制御 ---

bool sampling_manager_start(SamplingManager* manager) {
    for (int i = 0; i < manager->num_devices; ++i) {
        SamplingDevice* dev = manager->devices[i];
        if (!sampling_device_open(dev)) {
            LOG_ERROR("Failed to open device '%s'", dev->name);
            return false;
        }
        if (!sampling_device_prepare_sampling(dev)) {
            LOG_ERROR("Failed to prepare sampling for '%s'", dev->name);
            return false;
        }
        if (!sampling_device_start_sampling(dev)) {
            LOG_ERROR("Failed to start sampling for '%s'", dev->name);
            return false;
        }
    }

    manager->running = true;
    if (pthread_create(&manager->run_thread, NULL, run_loop, manager) != 0) {
        LOG_ERROR("MANAGER", "Failed to create data acquisition thread.");
        manager->running = false;
        return false;
    }
    return true;
}

void sampling_manager_stop(SamplingManager* manager) {
    if (manager->running) {
        manager->running = false;
        pthread_join(manager->run_thread, NULL);
    }
    for (int i = 0; i < manager->num_devices; ++i) {
        sampling_device_stop_sampling(manager->devices[i]);
        sampling_device_close(manager->devices[i]);
    }
    LOG_INFO("MANAGER", "Manager stopped.");
}

// --- デバイスへの問い合わせ関数 ---
static SamplingChannel* find_channel_by_id(SamplingManager* manager, int channel_id) {
    for (int i = 0; i < manager->num_channels; ++i) {
        if (manager->channels[i]->id == channel_id) {
            return manager->channels[i];
        }
    }
    return NULL;
}

static SamplingDevice* find_device_by_id(SamplingManager* manager, DeviceType device_id) {
    for (int i = 0; i < manager->num_devices; ++i) {
        if (manager->devices[i]->id == device_id) {
            return manager->devices[i];
        }
    }
    return NULL;
}

double sampling_manager_get_terminal_voltage(SamplingManager* manager, int channel_id) {
    SamplingChannel* ch = find_channel_by_id(manager, channel_id);
    if (!ch) return -1.0;
    SamplingDevice* dev = find_device_by_id(manager, ch->device_id);
    if (!dev) return -1.0;
    return dev->vtable->get_terminal_voltage(dev->handle, ch->ch_index);
}

double sampling_manager_get_gain(SamplingManager* manager, int channel_id) {
    SamplingChannel* ch = find_channel_by_id(manager, channel_id);
    if (!ch) return -1.0;
    SamplingDevice* dev = find_device_by_id(manager, ch->device_id);
    if (!dev) return -1.0;
    return dev->vtable->get_gain(dev->handle, ch->ch_index);
}

bool sampling_manager_set_gain(SamplingManager* manager, int channel_id, double gain) {
    SamplingChannel* ch = find_channel_by_id(manager, channel_id);
    if (!ch) return false;
    SamplingDevice* dev = find_device_by_id(manager, ch->device_id);
    if (!dev) return false;
    if (dev->vtable->set_gain(dev->handle, ch->ch_index, gain)) {
        ch->gain = gain;
        return true;
    }
    return false;
}

int sampling_manager_get_digital_in(SamplingManager* manager, DeviceType device_id, int ch_index) {
    SamplingDevice* dev = find_device_by_id(manager, device_id);
    if (!dev) return -1;
    return dev->vtable->get_digital_in(dev->handle, ch_index);
}

int sampling_manager_get_digital_out(SamplingManager* manager, DeviceType device_id, int ch_index) {
    SamplingDevice* dev = find_device_by_id(manager, device_id);
    if (!dev) return -1;
    return dev->vtable->get_digital_out(dev->handle, ch_index);
}

bool sampling_manager_set_digital_out(SamplingManager* manager, DeviceType device_id, int ch_index, int status) {
    SamplingDevice* dev = find_device_by_id(manager, device_id);
    if (!dev) return false;
    return dev->vtable->set_digital_out(dev->handle, ch_index, status);
}

#include "dummy_device.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // For sleep
#include <math.h>   // For sin

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DUMMY_CH_NO 17
#define DUMMY_SAMPLING_FREQUENCY 25600
#define DUMMY_DI_NO 4
#define DUMMY_DO_NO 8

// DummyDeviceの具象構造体
typedef struct {
    short* ad_data[DUMMY_CH_NO - 1];
    int pulse_data;
    double gains[DUMMY_CH_NO - 1];
    int input_types[DUMMY_CH_NO - 1];
    int digital_in[DUMMY_DI_NO];
    int digital_out[DUMMY_DO_NO];
} DummyDevice;

// --- インターフェースを実装する静的関数 ---

static void destroy_impl(void* device_handle) {
    DummyDevice* dev = (DummyDevice*)device_handle;
    if (!dev) return;
    for (int i = 0; i < DUMMY_CH_NO - 1; ++i) {
        free(dev->ad_data[i]);
    }
    free(dev);
    LOG_INFO("DUMMY", "Device destroyed.");
}

static bool open_impl(void* device_handle) {
    (void)device_handle;
    LOG_INFO("DUMMY", "Device Open");
    return true;
}

static void close_impl(void* device_handle) {
    (void)device_handle;
    LOG_INFO("DUMMY", "Device Close");
}

static bool start_sampling_impl(void* device_handle) {
    (void)device_handle;
    LOG_INFO("DUMMY", "Sampling Start");
    return true;
}

static void stop_sampling_impl(void* device_handle) {
    (void)device_handle;
    LOG_INFO("DUMMY", "Sampling Stop");
}

static bool check_data_impl(void* device_handle) {
    (void)device_handle;
    sleep(1); // 1秒待機
    return true;
}

static bool get_data_impl(void* device_handle, SamplingChannel** channels, int num_channels) {
    DummyDevice* dev = (DummyDevice*)device_handle;

    // サイン波データを生成
    for (int i = 0; i < DUMMY_CH_NO - 1; ++i) {
        for (int j = 0; j < DUMMY_SAMPLING_FREQUENCY; ++j) {
            double tmp = (double)j / DUMMY_SAMPLING_FREQUENCY;
            dev->ad_data[i][j] = (short)(100.0 * sin(2.0 * M_PI * 10.0 * tmp));
        }
    }
    dev->pulse_data = 200;

    // チャンネルにデータをコピー
    for (int i = 0; i < num_channels; ++i) {
        SamplingChannel* ch = channels[i];
        if (ch->ch_index > 0 && ch->ch_index < DUMMY_CH_NO) {
            memcpy(ch->buffer.ad, dev->ad_data[ch->ch_index - 1], sizeof(short) * ch->sampling_no);
        } else if (ch->ch_index == DUMMY_CH_NO) {
            ch->buffer.pulse[0] = dev->pulse_data;
        }
    }
    return true;
}

static char* get_version_impl(void* device_handle) {
    (void)device_handle;
    char* version_str = (char*)malloc(9);
    sprintf(version_str, "%08x", 1000);
    return version_str;
}

static double get_gain_impl(void* device_handle, int ch_index) {
    DummyDevice* dev = (DummyDevice*)device_handle;
    if (ch_index < 1 || ch_index >= DUMMY_CH_NO) return -1.0;
    return dev->gains[ch_index - 1];
}

static bool set_gain_impl(void* device_handle, int ch_index, double gain) {
    DummyDevice* dev = (DummyDevice*)device_handle;
    if (ch_index < 1 || ch_index >= DUMMY_CH_NO) return false;
    dev->gains[ch_index - 1] = gain;
    LOG_INFO("DUMMY", "Set CH %d Gain to %f", ch_index, gain);
    return true;
}

static int get_input_impl(void* device_handle, int ch_index) {
    DummyDevice* dev = (DummyDevice*)device_handle;
    if (ch_index < 1 || ch_index >= DUMMY_CH_NO) return -1;
    return dev->input_types[ch_index - 1];
}

static bool set_input_impl(void* device_handle, int ch_index, int input) {
    DummyDevice* dev = (DummyDevice*)device_handle;
    if (ch_index < 1 || ch_index >= DUMMY_CH_NO) return false;
    dev->input_types[ch_index - 1] = input;
    LOG_INFO("DUMMY", "Set CH %d InputType to %d", ch_index, input);
    return true;
}

static bool check_sampling_freq_impl(void* device_handle, int ch_index, int sampling_freq) {
    (void)device_handle;
    if (ch_index > 0 && ch_index < DUMMY_CH_NO) {
        return sampling_freq == DUMMY_SAMPLING_FREQUENCY;
    } else if (ch_index == DUMMY_CH_NO) {
        return sampling_freq == 1;
    }
    return false;
}
static int get_ad_ch_no_impl(void* device_handle) {
    (void)device_handle;
    return DUMMY_CH_NO - 1;
}
static double get_terminal_voltage_impl(void* device_handle, int ch_index) {
    (void)device_handle;
    return (double)(ch_index) * 1.5; // Return some dummy value
}
static int get_digital_in_impl(void* device_handle, int di_no) {
    DummyDevice* dev = (DummyDevice*)device_handle;
    if (di_no < 1 || di_no > DUMMY_DI_NO) return -1;
    return dev->digital_in[di_no - 1];
}
static int get_digital_out_impl(void* device_handle, int do_no) {
    DummyDevice* dev = (DummyDevice*)device_handle;
    if (do_no < 1 || do_no > DUMMY_DO_NO) return -1;
    return dev->digital_out[do_no - 1];
}
static bool set_digital_out_impl(void* device_handle, int do_no, int on_off) {
    DummyDevice* dev = (DummyDevice*)device_handle;
    if (do_no < 1 || do_no > DUMMY_DO_NO) return false;
    dev->digital_out[do_no - 1] = on_off;
    LOG_INFO("DUMMY", "Set DO %d to %d", do_no, on_off);
    return true;
}
static void reset_impl(void* device_handle) {
    (void)device_handle;
    LOG_INFO("DUMMY", "Reset Device Called");
}
static void restart_impl(void* device_handle) {
    (void)device_handle;
    LOG_INFO("DUMMY", "Restart System Called");
}

// --- vtableの定義 ---
static const DeviceVTable dummy_device_vtable_instance = {
    .open = open_impl,
    .close = close_impl,
    .destroy = destroy_impl,
    .get_version = get_version_impl,
    .get_ad_ch_no = get_ad_ch_no_impl,
    .check_sampling_freq = check_sampling_freq_impl,
    .start_sampling = start_sampling_impl,
    .stop_sampling = stop_sampling_impl,
    .check_data = check_data_impl,
    .get_data = get_data_impl,
    .get_gain = get_gain_impl,
    .set_gain = set_gain_impl,
    .get_input = get_input_impl,
    .set_input = set_input_impl,
    .get_terminal_voltage = get_terminal_voltage_impl,
    .get_digital_in = get_digital_in_impl,
    .get_digital_out = get_digital_out_impl,
    .set_digital_out = set_digital_out_impl,
    .reset = reset_impl,
    .restart = restart_impl,
};


// --- 公開関数 ---
void* dummy_device_new(void) {
    DummyDevice* dev = (DummyDevice*)calloc(1, sizeof(DummyDevice));
    if (!dev) {
        LOG_ERROR("DUMMY", "Failed to allocate memory for DummyDevice");
        return NULL;
    }
    for (int i = 0; i < DUMMY_CH_NO - 1; ++i) {
        dev->ad_data[i] = (short*)malloc(sizeof(short) * DUMMY_SAMPLING_FREQUENCY);
        dev->gains[i] = 1.0;
        dev->input_types[i] = 0;
    }
    LOG_INFO("DUMMY", "New device created.");
    return dev;
}

const DeviceVTable* dummy_device_vtable(void) {
    return &dummy_device_vtable_instance;
}

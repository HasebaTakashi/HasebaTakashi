#include "vmonitor2_board.h"
#include "vmonitor2_driver.h"
#include "app_config.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// VMonitor2Boardの具象構造体
typedef struct {
    unsigned int pulse_count_threshold;
    unsigned int idle_reboot_type;
    unsigned int idle_reboot_timeout;
    short* ad_data[VMONITOR2_AD_CH_NO];
} VMonitor2Board;


// --- インターフェースを実装する静的関数 ---

static void destroy_impl(void* device_handle) {
    VMonitor2Board* board = (VMonitor2Board*)device_handle;
    if (!board) return;
    for (int i = 0; i < VMONITOR2_AD_CH_NO; ++i) {
        free(board->ad_data[i]);
    }
    free(board);
}

static bool open_impl(void* device_handle) {
    VMonitor2Board* board = (VMonitor2Board*)device_handle;
    if (VM2_Open() != 0) {
        LOG_ERROR("VMB2", "VM2_Open failed");
        return false;
    }
    if (VM2_Initialize() != 0) {
        LOG_ERROR("VMB2", "VM2_Initialize failed");
        VM2_Close();
        return false;
    }

    unsigned int old_threshold;
    VM2_GetPulseVoltage(&old_threshold);
    if (old_threshold != board->pulse_count_threshold) {
        LOG_INFO("VMB2", "Pulse threshold mismatch. Old: %u, New: %u. Setting...", old_threshold, board->pulse_count_threshold);
        if (VM2_SetPulseVoltage(board->pulse_count_threshold) != 0) {
            LOG_ERROR("VMB2", "Write pulse threshold failed.");
            return false;
        }
    }

    unsigned int old_type, old_timeout;
    VM2_GetIdleReboot(&old_type, &old_timeout);
    if (old_type != board->idle_reboot_type || old_timeout != board->idle_reboot_timeout) {
        LOG_INFO("VMB2", "Idle reboot mismatch. Old: %u/%u, New: %u/%u. Setting...", old_type, old_timeout, board->idle_reboot_type, board->idle_reboot_timeout);
        if (VM2_SetIdleReboot(board->idle_reboot_type, board->idle_reboot_timeout) != 0) {
            LOG_ERROR("VMB2", "Write idle reboot setting failed.");
            return false;
        }
    }
    return true;
}

static void close_impl(void* device_handle) {
    (void)device_handle; // この実装では未使用
    VM2_Close();
}

static void reset_impl(void* device_handle) {
    (void)device_handle;
    VM2_Reset(1);
}

static void restart_impl(void* device_handle) {
    (void)device_handle;
    VM2_Restart(5);
}

static bool start_sampling_impl(void* device_handle) {
    (void)device_handle;
    return VM2_StartSampling() == 0;
}

static void stop_sampling_impl(void* device_handle) {
    (void)device_handle;
    VM2_StopSampling();
}

static bool check_data_impl(void* device_handle) {
    (void)device_handle;
    int exist = 0;
    VM2_CheckBuffer(&exist);
    return exist == 1;
}

static bool get_data_impl(void* device_handle, SamplingChannel** channels, int num_channels) {
    VMonitor2Board* board = (VMonitor2Board*)device_handle;
    int pulse_count = 0;
    if (VM2_GetData(
        board->ad_data[0], board->ad_data[1], board->ad_data[2], board->ad_data[3],
        board->ad_data[4], board->ad_data[5], board->ad_data[6], board->ad_data[7],
        board->ad_data[8], board->ad_data[9], board->ad_data[10], board->ad_data[11],
        board->ad_data[12], board->ad_data[13], board->ad_data[14], board->ad_data[15],
        &pulse_count) != 0) {
        return false;
    }

    for (int i = 0; i < num_channels; ++i) {
        SamplingChannel* ch = channels[i];
        if (ch->ch_index > 0 && ch->ch_index <= VMONITOR2_AD_CH_NO) {
            memcpy(ch->buffer.ad,
                   board->ad_data[ch->ch_index - 1],
                   sizeof(short) * ch->sampling_no);
        } else if (ch->ch_index == VMONITOR2_CH_NO) {
            ch->buffer.pulse[0] = pulse_count;
        }
    }
    return true;
}

static char* get_version_impl(void* device_handle) {
    (void)device_handle;
    unsigned int version = 0;
    VM2_GetVersion(&version);
    char* version_str = (char*)malloc(9);
    if (version_str) {
        sprintf(version_str, "%08x", version);
    }
    return version_str;
}

static double get_gain_impl(void* device_handle, int ch_index) {
    (void)device_handle;
    if (ch_index < 1 || ch_index >= VMONITOR2_CH_NO) return 1.0;
    unsigned int gain_no = 0;
    VM2_GetGain(ch_index, &gain_no);
    switch (gain_no) {
        case 1: return 1.0;
        case 2: return 2.4;
        case 3: return 10.0;
        case 4: return 24.0;
        default: return -1.0;
    }
}

static bool set_gain_impl(void* device_handle, int ch_index, double gain) {
    (void)device_handle;
    if (ch_index < 1 || ch_index >= VMONITOR2_CH_NO) return false;
    unsigned int gain_no;
    if (gain == 1.0) gain_no = 1;
    else if (gain == 2.4) gain_no = 2;
    else if (gain == 10.0) gain_no = 3;
    else if (gain == 24.0) gain_no = 4;
    else return false;
    return VM2_SetGain(ch_index, gain_no) == 0;
}

static int get_input_impl(void* device_handle, int ch_index) {
    (void)device_handle;
    if (ch_index < 1 || ch_index >= VMONITOR2_CH_NO) return 0;
    unsigned int select = 0;
    VM2_GetInputSelect(ch_index, &select);
    return select;
}

static bool set_input_impl(void* device_handle, int ch_index, int input) {
    (void)device_handle;
    if (ch_index < 1 || ch_index >= VMONITOR2_CH_NO) return false;
    return VM2_SetInputSelect(ch_index, input) == 0;
}

static double get_terminal_voltage_impl(void* device_handle, int ch_index) {
    (void)device_handle;
    unsigned int voltage_mv = 0;
    VM2_GetTerminalAd(ch_index, &voltage_mv);
    return (double)voltage_mv / 1000.0;
}

static int get_digital_in_impl(void* device_handle, int di_no) {
    (void)device_handle;
    unsigned int status = 0;
    VM2_GetDigitalIn(di_no, &status);
    return status;
}

static int get_digital_out_impl(void* device_handle, int do_no) {
    (void)device_handle;
    unsigned int status = 0;
    VM2_GetDigitalOut(do_no, &status);
    return status;
}

static bool set_digital_out_impl(void* device_handle, int do_no, int on_off) {
    (void)device_handle;
    return VM2_SetDigitalOut(do_no, on_off) == 0;
}

static int get_ad_ch_no_impl(void* device_handle) {
    (void)device_handle;
    return VMONITOR2_AD_CH_NO;
}

static bool check_sampling_freq_impl(void* device_handle, int ch_index, int sampling_freq) {
    (void)device_handle;
    if (ch_index > 0 && ch_index <= VMONITOR2_AD_CH_NO) {
        return sampling_freq == VMONITOR2_SAMPLING_FREQUENCY;
    } else if (ch_index == VMONITOR2_CH_NO) {
        return sampling_freq == 1;
    }
    return false;
}

// --- vtableの定義 ---
static const DeviceVTable vmonitor2_board_vtable_instance = {
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

void* vmonitor2_board_new(void) {
    VMonitor2Board* board = (VMonitor2Board*)calloc(1, sizeof(VMonitor2Board));
    if (!board) {
        LOG_ERROR("VMB2", "Failed to allocate memory for VMonitor2Board");
        return NULL;
    }

    board->pulse_count_threshold = PULSE_COUNT_THRESHOLD;
    board->idle_reboot_type = IDLE_REBOOT_TYPE;
    board->idle_reboot_timeout = IDLE_REBOOT_TIMEOUT;

    for (int i = 0; i < VMONITOR2_AD_CH_NO; ++i) {
        board->ad_data[i] = (short*)malloc(sizeof(short) * VMONITOR2_SAMPLING_FREQUENCY);
        if (!board->ad_data[i]) {
            LOG_ERROR("VMB2", "Failed to allocate memory for AD data buffer #%d", i + 1);
            destroy_impl(board); // 確保済みのメモリを解放
            return NULL;
        }
    }
    return board;
}

const DeviceVTable* vmonitor2_board_vtable(void) {
    return &vmonitor2_board_vtable_instance;
}

#include "vmonitor2_board.h"
#include "vmonitor2_driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_INFO(msg, ...) printf("[INFO] " msg "\n", ##__VA_ARGS__)
#define LOG_ERROR(msg, ...) fprintf(stderr, "[ERROR] " msg "\n", ##__VA_ARGS__)

// --- ライフサイクル管理 ---

VMonitor2Board* vmonitor2_board_create(void) {
    VMonitor2Board* board = (VMonitor2Board*)malloc(sizeof(VMonitor2Board));
    if (!board) {
        LOG_ERROR("Failed to allocate memory for VMonitor2Board");
        return NULL;
    }

    // 設定値をconfigから読み込む
    board->pulse_count_threshold = PULSE_COUNT_THRESHOLD;
    board->idle_reboot_type = IDLE_REBOOT_TYPE;
    board->idle_reboot_timeout = IDLE_REBOOT_TIMEOUT;

    // ADデータバッファのメモリ確保
    for (int i = 0; i < VMONITOR2_AD_CH_NO; ++i) {
        board->ad_data[i] = (short*)malloc(sizeof(short) * VMONITOR2_SAMPLING_FREQUENCY);
        if (!board->ad_data[i]) {
            LOG_ERROR("Failed to allocate memory for AD data buffer #%d", i + 1);
            // 確保済みのメモリを解放
            for (int j = 0; j < i; ++j) {
                free(board->ad_data[j]);
            }
            free(board);
            return NULL;
        }
    }
    return board;
}

void vmonitor2_board_destroy(VMonitor2Board* board) {
    if (!board) return;
    for (int i = 0; i < VMONITOR2_AD_CH_NO; ++i) {
        free(board->ad_data[i]);
    }
    free(board);
}

// --- デバイス制御 ---

bool vmonitor2_board_open(VMonitor2Board* board) {
    if (VM2_Open() != 0) {
        LOG_ERROR("VM2_Open failed");
        return false;
    }
    if (VM2_Initialize() != 0) {
        LOG_ERROR("VM2_Initialize failed");
        VM2_Close();
        return false;
    }

    // パルスしきい値の設定
    unsigned int old_threshold;
    VM2_GetPulseVoltage(&old_threshold);
    LOG_INFO("Old PulseThreshold:%u, New PulseThreshold:%u", old_threshold, board->pulse_count_threshold);
    if (old_threshold != board->pulse_count_threshold) {
        LOG_INFO("Executing write pulse threshold...");
        if (VM2_SetPulseVoltage(board->pulse_count_threshold) == 0) {
            LOG_INFO("Write pulse threshold success.");
        } else {
            LOG_ERROR("Write pulse threshold failed.");
            return false;
        }
    }

    // コマンド未受信リブートの設定
    unsigned int old_type, old_timeout;
    VM2_GetIdleReboot(&old_type, &old_timeout);
    LOG_INFO("Old IdleReboot Type:%u Timeout:%u, New Type:%u Timeout:%u",
             old_type, old_timeout, board->idle_reboot_type, board->idle_reboot_timeout);
    if (old_type != board->idle_reboot_type || old_timeout != board->idle_reboot_timeout) {
        LOG_INFO("Executing write idle reboot setting...");
        if (VM2_SetIdleReboot(board->idle_reboot_type, board->idle_reboot_timeout) == 0) {
            LOG_INFO("Write idle reboot setting success.");
        } else {
            LOG_ERROR("Write idle reboot setting failed.");
            return false;
        }
    }
    return true;
}

void vmonitor2_board_close(void) {
    VM2_Close();
}

void vmonitor2_board_reset(void) {
    VM2_Reset(1);
}

void vmonitor2_board_restart(void) {
    VM2_Restart(5);
}

// --- サンプリング制御 ---

bool vmonitor2_board_start_sampling(void) {
    return VM2_StartSampling() == 0;
}

void vmonitor2_board_stop_sampling(void) {
    VM2_StopSampling();
}

bool vmonitor2_board_check_data(void) {
    int exist = 0;
    VM2_CheckBuffer(&exist);
    return exist == 1;
}

bool vmonitor2_board_get_data(VMonitor2Board* board, SamplingChannel* channels, int num_channels) {
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
        if (channels[i].ch_index > 0 && channels[i].ch_index <= VMONITOR2_AD_CH_NO) {
            // ADチャンネル
            memcpy(channels[i].buffer.ad,
                   board->ad_data[channels[i].ch_index - 1],
                   sizeof(short) * channels[i].sampling_no);
        } else if (channels[i].ch_index == VMONITOR2_CH_NO) {
            // パルスチャンネル
            channels[i].buffer.pulse[0] = pulse_count;
        }
    }
    return true;
}

// --- 状態および情報取得 ---

unsigned int vmonitor2_board_get_status(void) {
    unsigned int status = 0;
    VM2_GetStatus(&status);
    return status;
}

unsigned int vmonitor2_board_get_error_code(void) {
    unsigned int code = 0;
    VM2_GetDetailErrorCode(&code);
    return code;
}

char* vmonitor2_board_get_version(void) {
    unsigned int version = 0;
    VM2_GetVersion(&version);
    char* version_str = (char*)malloc(9); // 8 hex chars + null terminator
    if (version_str) {
        sprintf(version_str, "%08x", version);
    }
    return version_str;
}

double vmonitor2_board_get_terminal_voltage(int ch_index) {
    unsigned int voltage_mv = 0;
    VM2_GetTerminalAd(ch_index, &voltage_mv);
    return (double)voltage_mv / 1000.0;
}

// --- チャンネル設定 ---

double vmonitor2_board_get_gain(int ch_index) {
    if (ch_index < 1 || ch_index >= VMONITOR2_CH_NO) return 1.0;

    unsigned int gain_no = 0;
    VM2_GetGain(ch_index, &gain_no);
    switch (gain_no) {
        case 1: return 1.0;
        case 2: return 2.4;
        case 3: return 10.0;
        case 4: return 24.0;
        default: return -1.0; // Error
    }
}

bool vmonitor2_board_set_gain(int ch_index, double gain) {
    if (ch_index < 1 || ch_index >= VMONITOR2_CH_NO) return false;

    unsigned int gain_no;
    if (gain == 1.0) gain_no = 1;
    else if (gain == 2.4) gain_no = 2;
    else if (gain == 10.0) gain_no = 3;
    else if (gain == 24.0) gain_no = 4;
    else return false; // Invalid gain value

    return VM2_SetGain(ch_index, gain_no) == 0;
}

int vmonitor2_board_get_input(int ch_index) {
    if (ch_index < 1 || ch_index >= VMONITOR2_CH_NO) return 0;
    unsigned int select = 0;
    VM2_GetInputSelect(ch_index, &select);
    return select;
}

bool vmonitor2_board_set_input(int ch_index, int input) {
    if (ch_index < 1 || ch_index >= VMONITOR2_CH_NO) return false;
    return VM2_SetInputSelect(ch_index, input) == 0;
}

// --- デジタルI/O ---

int vmonitor2_board_get_digital_in(int ch_index) {
    unsigned int status = 0;
    VM2_GetDigitalIn(ch_index, &status);
    return status;
}

int vmonitor2_board_get_digital_out(int ch_index) {
    unsigned int status = 0;
    VM2_GetDigitalOut(ch_index, &status);
    return status;
}

bool vmonitor2_board_set_digital_out(int ch_index, int status) {
    return VM2_SetDigitalOut(ch_index, status) == 0;
}

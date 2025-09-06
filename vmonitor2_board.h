#ifndef VMONITOR2_BOARD_H
#define VMONITOR2_BOARD_H

#include "app_config.h"
#include "sampling_channel.h"
#include <stdbool.h>

// VMonitor2Boardの状態を保持する構造体
typedef struct {
    // app_config.hから読み込む設定値
    unsigned int pulse_count_threshold;
    unsigned int idle_reboot_type;
    unsigned int idle_reboot_timeout;

    // VM2_GetData関数で使用するデータバッファ
    // 16個のアナログチャンネルそれぞれに対するポインタ
    short* ad_data[VMONITOR2_AD_CH_NO];

} VMonitor2Board;

// --- ライフサイクル管理 ---
VMonitor2Board* vmonitor2_board_create(void);
void vmonitor2_board_destroy(VMonitor2Board* board);

// --- デバイス制御 ---
bool vmonitor2_board_open(VMonitor2Board* board);
void vmonitor2_board_close(void);
void vmonitor2_board_reset(void);
void vmonitor2_board_restart(void);

// --- サンプリング制御 ---
bool vmonitor2_board_start_sampling(void);
void vmonitor2_board_stop_sampling(void);
bool vmonitor2_board_check_data(void);
bool vmonitor2_board_get_data(VMonitor2Board* board, SamplingChannel* channels, int num_channels);

// --- 状態および情報取得 ---
unsigned int vmonitor2_board_get_status(void);
unsigned int vmonitor2_board_get_error_code(void);
// バージョンを16進文字列として返す。呼び出し元は返された文字列をfree()する必要がある。
char* vmonitor2_board_get_version(void);
double vmonitor2_board_get_terminal_voltage(int ch_index);

// --- チャンネル設定 ---
double vmonitor2_board_get_gain(int ch_index);
bool vmonitor2_board_set_gain(int ch_index, double gain);
int vmonitor2_board_get_input(int ch_index);
bool vmonitor2_board_set_input(int ch_index, int input);

// --- デジタルI/O ---
int vmonitor2_board_get_digital_in(int ch_index);
int vmonitor2_board_get_digital_out(int ch_index);
bool vmonitor2_board_set_digital_out(int ch_index, int status);

#endif // VMONITOR2_BOARD_H

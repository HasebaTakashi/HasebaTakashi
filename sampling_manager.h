#ifndef SAMPLING_MANAGER_H
#define SAMPLING_MANAGER_H

#include "vmonitor2_board.h"
#include "sampling_channel.h"
#include "sampling_queue.h"
#include <pthread.h>
#include <stdbool.h>

// データ収集全体を管理する構造体
typedef struct {
    VMonitor2Board* board;
    SamplingChannel* channels;
    int num_channels;

    // データキューのリスト (ポインタの配列)
    SamplingQueue** data_queues;
    int num_queues;

    pthread_t run_thread;   // データ収集スレッドの識別子
    volatile bool running;  // スレッドの実行状態を制御するフラグ
} SamplingManager;

// --- ライフサイクル管理 ---
// 設定に基づきサンプリングマネージャを作成する
// (現段階では、設定は内部でハードコードする)
SamplingManager* sampling_manager_create(int num_queues, SamplingQueue** queues);
void sampling_manager_destroy(SamplingManager* manager);

// --- 制御 ---
// デバイスを準備し、データ収集スレッドを開始する
bool sampling_manager_start(SamplingManager* manager);
// データ収集スレッドを停止し、デバイスを閉じる
void sampling_manager_stop(SamplingManager* manager);

// --- デバイスへの問い合わせ関数 ---
// チャンネルIDを元に、適切なデバイス関数を呼び出す
double sampling_manager_get_terminal_voltage(SamplingManager* manager, int channel_id);
double sampling_manager_get_gain(SamplingManager* manager, int channel_id);
bool sampling_manager_set_gain(SamplingManager* manager, int channel_id, double gain);
int sampling_manager_get_digital_in(SamplingManager* manager, int device_id, int ch_index);
int sampling_manager_get_digital_out(SamplingManager* manager, int device_id, int ch_index);
bool sampling_manager_set_digital_out(SamplingManager* manager, int device_id, int ch_index, int status);

#endif // SAMPLING_MANAGER_H

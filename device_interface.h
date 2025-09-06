#ifndef DEVICE_INTERFACE_H
#define DEVICE_INTERFACE_H

#include "sampling_channel.h"
#include "device_types.h"
#include <stdbool.h>


// DeviceVTableは、すべてのサンプリングデバイスの共通インターフェースを定義します。
// これは関数ポインタの構造体で、C言語における仮想メソッドテーブルを模倣しています。
// 各関数の第一引数 void* device_handle は、具象デバイス(vmonitor2_boardなど)のインスタンスを指します。
typedef struct {
    // --- ライフサイクルとセットアップ ---
    bool (*open)(void* device_handle);
    void (*close)(void* device_handle);
    void (*destroy)(void* device_handle); // 具象デバイスオブジェクトを破棄する
    char* (*get_version)(void* device_handle);
    int (*get_ad_ch_no)(void* device_handle);
    bool (*check_sampling_freq)(void* device_handle, int ch_index, int sampling_freq);

    // --- サンプリング制御 ---
    bool (*start_sampling)(void* device_handle);
    void (*stop_sampling)(void* device_handle);
    bool (*check_data)(void* device_handle);
    bool (*get_data)(void* device_handle, SamplingChannel** channels, int num_channels);

    // --- チャンネル設定 ---
    double (*get_gain)(void* device_handle, int ch_index);
    bool (*set_gain)(void* device_handle, int ch_index, double gain);
    int (*get_input)(void* device_handle, int ch_index);
    bool (*set_input)(void* device_handle, int ch_index, int input);

    // --- 状態と情報取得 ---
    double (*get_terminal_voltage)(void* device_handle, int ch_index);
    int (*get_digital_in)(void* device_handle, int di_no);
    int (*get_digital_out)(void* device_handle, int do_no);
    bool (*set_digital_out)(void* device_handle, int do_no, int on_off);

    // --- システム制御 ---
    void (*reset)(void* device_handle);
    void (*restart)(void* device_handle);

} DeviceVTable;

#endif // DEVICE_INTERFACE_H

#ifndef SAMPLING_CHANNEL_H
#define SAMPLING_CHANNEL_H

#include "device_types.h"

// RubyのSamplingChannelクラスに対応する構造体
// 各チャンネルの設定情報を保持します。
typedef struct {
    int id;                 // チャンネルID
    char name[64];          // チャンネル名 (最大64文字と仮定)
    DeviceType device_id;   // 属するデバイスのID
    int ch_index;           // デバイス内でのチャンネルインデックス
    int sampling_no;        // サンプリング周波数 (1秒間のデータ数)
    double gain;            // ゲイン
    double max_range;       // 最大レンジ
    double min_range;       // 最小レンジ
    int resolution;         // 分解能
    int input_type;         // 入力種別

    // チャンネルのデータバッファ。初期化時に動的にメモリ確保されます。
    // ch_indexを使ってADチャンネルかパルスチャンネルかを判断し、
    // unionのどちらのメンバを使用するかを決定します。
    union {
        short* ad;      // ADチャンネル用バッファ (short[sampling_no])
        int* pulse;     // パルスチャンネル用バッファ (int[sampling_no], sampling_no=1)
    } buffer;

} SamplingChannel;

#endif // SAMPLING_CHANNEL_H

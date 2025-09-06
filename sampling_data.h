#ifndef SAMPLING_DATA_H
#define SAMPLING_DATA_H

#include <time.h>
#include "sampling_channel.h" // SamplingChannelの定義をインクルード

// 1秒分のサンプリングデータを表現する構造体
// RubyのSamplingDataクラスに対応します。
typedef struct {
    // データ取得時刻
    struct timespec data_time;

    // このデータセットに含まれるチャンネル数
    int num_channels;

    // 各チャンネルのデータ配列。動的にメモリが確保されます。
    struct DataPerChannel {
        int channel_id;     // チャンネルID
        int ch_index;       // デバイス内インデックス (AD/パルスの区別に利用)
        int sampling_no;    // このチャンネルのデータ数

        // データバッファのポインタ。
        // このデータは、各チャンネルのバッファのコピーです。
        union {
            short* ad;
            int* pulse;
        } buffer;
    }* data_per_channel;

} SamplingData;

// SamplingDataオブジェクトを作成/破棄するための関数プロトタイプ
SamplingData* sampling_data_create(SamplingChannel* channels, int num_channels);
void sampling_data_destroy(SamplingData* data);

#endif // SAMPLING_DATA_H

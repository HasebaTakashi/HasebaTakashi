#ifndef SAMPLING_QUEUE_H
#define SAMPLING_QUEUE_H

#include <pthread.h>
#include "sampling_data.h"

// スレッドセーフなデータキュー
// RubyのSamplingQueueクラスに対応します。
typedef struct {
    char name[64];                  // キューの名前
    SamplingData** buffer;          // SamplingDataポインタを格納するリングバッファ
    int capacity;                   // キューの最大容量
    int count;                      // 現在の要素数
    int head;                       // 次にデキューする要素のインデックス
    int tail;                       // 次にエンキューする位置のインデックス
    pthread_mutex_t mutex;          // スレッドセーフのためのミューテックス
} SamplingQueue;

// キュー操作のための関数プロトタイプ
// 新しいキューを作成する
SamplingQueue* queue_create(const char* name, int capacity);
// キューを破棄し、中のデータも含めてメモリを解放する
void queue_destroy(SamplingQueue* queue);
// データをキューに入れる (キューが満杯の場合は古いものから削除される)
void queue_enqueue(SamplingQueue* queue, SamplingData* data);
// キューからデータを出す (呼び出し元が解放の責任を持つ)
SamplingData* queue_dequeue(SamplingQueue* queue);
// キューの現在のデータ数を取得する
int queue_data_count(SamplingQueue* queue);
// キューを空にする
void queue_clear(SamplingQueue* queue);
// 指定した時刻より古いデータを削除する
void queue_remove_older_than(SamplingQueue* queue, const struct timespec* trigger_time);

#endif // SAMPLING_QUEUE_H

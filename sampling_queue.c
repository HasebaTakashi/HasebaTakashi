#include "sampling_queue.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// timespec比較関数 (t1 < t2 -> -1, t1 == t2 -> 0, t1 > t2 -> 1)
static int timespec_cmp(const struct timespec* t1, const struct timespec* t2) {
    if (t1->tv_sec < t2->tv_sec) return -1;
    if (t1->tv_sec > t2->tv_sec) return 1;
    if (t1->tv_nsec < t2->tv_nsec) return -1;
    if (t1->tv_nsec > t2->tv_nsec) return 1;
    return 0;
}

SamplingQueue* queue_create(const char* name, int capacity) {
    if (capacity <= 0) return NULL;

    SamplingQueue* queue = (SamplingQueue*)malloc(sizeof(SamplingQueue));
    if (!queue) {
        perror("Failed to allocate SamplingQueue");
        return NULL;
    }

    strncpy(queue->name, name, sizeof(queue->name) - 1);
    queue->name[sizeof(queue->name) - 1] = '\0';

    queue->buffer = (SamplingData**)malloc(sizeof(SamplingData*) * capacity);
    if (!queue->buffer) {
        perror("Failed to allocate queue buffer");
        free(queue);
        return NULL;
    }

    queue->capacity = capacity;
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;

    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        perror("Failed to initialize mutex");
        free(queue->buffer);
        free(queue);
        return NULL;
    }
    return queue;
}

void queue_destroy(SamplingQueue* queue) {
    if (!queue) return;

    // まずキューの中身をクリア
    queue_clear(queue);

    // その後、ミューテックスとキュー自体のリソースを解放
    pthread_mutex_destroy(&queue->mutex);
    free(queue->buffer);
    free(queue);
}

void queue_enqueue(SamplingQueue* queue, SamplingData* data) {
    if (!queue || !data) return;

    pthread_mutex_lock(&queue->mutex);

    if (queue->count == queue->capacity) {
        // キューが満杯なら、最も古い要素(head)を破棄
        sampling_data_destroy(queue->buffer[queue->head]);
        queue->head = (queue->head + 1) % queue->capacity;
        queue->count--;
    }

    queue->buffer[queue->tail] = data;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;

    pthread_mutex_unlock(&queue->mutex);
}

SamplingData* queue_dequeue(SamplingQueue* queue) {
    if (!queue) return NULL;

    pthread_mutex_lock(&queue->mutex);

    if (queue->count == 0) {
        pthread_mutex_unlock(&queue->mutex);
        return NULL; // キューは空
    }

    SamplingData* data = queue->buffer[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;

    pthread_mutex_unlock(&queue->mutex);
    return data;
}

int queue_data_count(SamplingQueue* queue) {
    if (!queue) return 0;
    int count;
    pthread_mutex_lock(&queue->mutex);
    count = queue->count;
    pthread_mutex_unlock(&queue->mutex);
    return count;
}

void queue_clear(SamplingQueue* queue) {
    if (!queue) return;
    pthread_mutex_lock(&queue->mutex);

    while(queue->count > 0) {
        SamplingData* data = queue->buffer[queue->head];
        sampling_data_destroy(data);
        queue->head = (queue->head + 1) % queue->capacity;
        queue->count--;
    }

    // ポインタをリセット
    queue->head = 0;
    queue->tail = 0;

    pthread_mutex_unlock(&queue->mutex);
}

void queue_remove_older_than(SamplingQueue* queue, const struct timespec* trigger_time) {
    if (!queue || !trigger_time) return;

    pthread_mutex_lock(&queue->mutex);

    while (queue->count > 0) {
        // 先頭のデータを確認
        SamplingData* data = queue->buffer[queue->head];
        if (timespec_cmp(&data->data_time, trigger_time) < 0) { // data_time < trigger_time
            // データが指定時刻より古いので、デキューして破棄
            sampling_data_destroy(data);
            queue->head = (queue->head + 1) % queue->capacity;
            queue->count--;
        } else {
            // これ以降のデータは新しいので、ループを抜ける
            break;
        }
    }

    pthread_mutex_unlock(&queue->mutex);
}

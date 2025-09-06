#include "sampling_data.h"
#include "app_config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

SamplingData* sampling_data_create(SamplingChannel* channels, int num_channels) {
    if (!channels || num_channels <= 0) {
        return NULL;
    }

    SamplingData* data = (SamplingData*)malloc(sizeof(SamplingData));
    if (!data) {
        perror("Failed to allocate SamplingData");
        return NULL;
    }

    clock_gettime(CLOCK_REALTIME, &data->data_time);
    data->num_channels = num_channels;

    size_t dpc_size = sizeof(struct DataPerChannel) * num_channels;
    data->data_per_channel = (struct DataPerChannel*)malloc(dpc_size);
    if (!data->data_per_channel) {
        perror("Failed to allocate data_per_channel");
        free(data);
        return NULL;
    }

    for (int i = 0; i < num_channels; ++i) {
        data->data_per_channel[i].channel_id = channels[i].id;
        data->data_per_channel[i].ch_index = channels[i].ch_index;
        data->data_per_channel[i].sampling_no = channels[i].sampling_no;

        // ch_index を使ってADチャンネルかパルスチャンネルかを判断
        if (channels[i].ch_index < VMONITOR2_CH_NO) { // AD Channel
            size_t buffer_size = sizeof(short) * channels[i].sampling_no;
            data->data_per_channel[i].buffer.ad = (short*)malloc(buffer_size);
            if (!data->data_per_channel[i].buffer.ad) {
                perror("Failed to allocate ad buffer");
                // エラー発生時、それまでに確保したメモリをすべて解放する
                for (int j = 0; j < i; ++j) {
                    if (data->data_per_channel[j].ch_index < VMONITOR2_CH_NO) {
                        free(data->data_per_channel[j].buffer.ad);
                    } else {
                        free(data->data_per_channel[j].buffer.pulse);
                    }
                }
                free(data->data_per_channel);
                free(data);
                return NULL;
            }
            memcpy(data->data_per_channel[i].buffer.ad, channels[i].buffer.ad, buffer_size);
        } else { // Pulse Channel
            size_t buffer_size = sizeof(int) * channels[i].sampling_no;
            data->data_per_channel[i].buffer.pulse = (int*)malloc(buffer_size);
             if (!data->data_per_channel[i].buffer.pulse) {
                perror("Failed to allocate pulse buffer");
                // エラー発生時、それまでに確保したメモリをすべて解放する
                 for (int j = 0; j < i; ++j) {
                    if (data->data_per_channel[j].ch_index < VMONITOR2_CH_NO) {
                        free(data->data_per_channel[j].buffer.ad);
                    } else {
                        free(data->data_per_channel[j].buffer.pulse);
                    }
                }
                free(data->data_per_channel);
                free(data);
                return NULL;
            }
            memcpy(data->data_per_channel[i].buffer.pulse, channels[i].buffer.pulse, buffer_size);
        }
    }

    return data;
}

void sampling_data_destroy(SamplingData* data) {
    if (!data) {
        return;
    }

    for (int i = 0; i < data->num_channels; ++i) {
        if (data->data_per_channel[i].ch_index < VMONITOR2_CH_NO) { // AD Channel
            free(data->data_per_channel[i].buffer.ad);
        } else { // Pulse Channel
            free(data->data_per_channel[i].buffer.pulse);
        }
    }
    free(data->data_per_channel);
    free(data);
}

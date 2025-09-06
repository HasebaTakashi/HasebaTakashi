#include <stdio.h>
#include "sampling_manager.h"
#include "sampling_queue.h"

int main(void) {
    printf("--- C-based VMonitor2 Data Acquisition ---\n");

    // 1. データキューを作成する
    printf("Creating data queue...\n");
    // 100秒分のデータを保持できるキューを作成
    SamplingQueue* data_queue = queue_create("MainQueue", 100);
    if (!data_queue) {
        fprintf(stderr, "Error: Failed to create data queue.\n");
        return 1;
    }

    // 2. サンプリングマネージャーを作成し、キューを渡す
    printf("Creating sampling manager...\n");
    SamplingManager* manager = sampling_manager_create(1, &data_queue);
    if (!manager) {
        fprintf(stderr, "Error: Failed to create sampling manager.\n");
        queue_destroy(data_queue);
        return 1;
    }

    // 3. マネージャーを開始する (データ収集スレッドが起動する)
    printf("Starting sampling manager...\n");
    if (!sampling_manager_start(manager)) {
        fprintf(stderr, "Error: Failed to start sampling manager.\n");
        sampling_manager_destroy(manager);
        queue_destroy(data_queue);
        return 1;
    }

    // 4. ユーザーの入力を待つ
    printf("\n>>> Data acquisition is running in the background.\n");
    printf(">>> Press Enter to stop and exit.\n");
    getchar();

    // 5. マネージャーを停止する
    printf("\nStopping sampling manager...\n");
    sampling_manager_stop(manager);

    // 6. リソースを解放する
    printf("Cleaning up resources...\n");
    sampling_manager_destroy(manager);
    queue_destroy(data_queue);

    printf("Application finished cleanly.\n");
    return 0;
}

#ifndef COMMAND_SERVER_H
#define COMMAND_SERVER_H

#include "sampling_manager.h"
#include <stdbool.h>
#include <pthread.h>

// サーバーの状態を保持する構造体
typedef struct {
    int port;                       // サーバーのポート番号
    int server_fd;                  // リスニング用ソケットファイルディスクリプタ
    SamplingManager* manager;       // データとデバイスを管理するサンプリングマネージャ
    pthread_t server_thread_id;     // クライアント接続を待ち受けるスレッドのID
    volatile bool running;          // サーバーの実行状態を制御するフラグ
} CommandServer;

/**
 * @brief コマンドサーバーを生成し、初期化する
 *
 * @param port サーバーがリッスンするポート番号
 * @param manager サンプリングマネージャへのポインタ
 * @return CommandServer* 成功した場合はサーバーオブジェクトへのポインタ、失敗した場合はNULL
 */
CommandServer* command_server_create(int port, SamplingManager* manager);

/**
 * @brief サーバーを起動し、クライアントからの接続待ち受けを開始する
 *
 * @param server 開始するサーバーオブジェクト
 * @return bool 成功した場合はtrue、失敗した場合はfalse
 */
bool command_server_start(CommandServer* server);

/**
 * @brief サーバーを停止する
 *
 * @param server 停止するサーバーオブジェクト
 */
void command_server_stop(CommandServer* server);

/**
 * @brief サーバーオブジェクトを破棄し、関連するリソースを解放する
 *
 * @param server 破棄するサーバーオブジェクト
 */
void command_server_destroy(CommandServer* server);

#endif // COMMAND_SERVER_H

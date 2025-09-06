#ifndef VMONITOR2_BOARD_H
#define VMONITOR2_BOARD_H

#include "device_interface.h" // For DeviceVTable

/**
 * @brief VMonitor2Boardデバイスの新しいインスタンスを作成します。
 *
 * @return void* 作成された具象デバイスオブジェクトへのポインタ。
 *               このハンドルはSamplingDeviceによって管理されるべきです。
 */
#include "config_manager.h" // For AppConfig

/**
 * @brief VMonitor2Boardデバイスの新しいインスタンスを作成します。
 *
 * @param app_config アプリケーション設定へのポインタ
 * @return void* 作成された具象デバイスオブジェクトへのポインタ。
 */
void* vmonitor2_board_new(const AppConfig* app_config);

/**
 * @brief VMonitor2Boardの仮想テーブルを取得します。
 *
 * @return const DeviceVTable* VMonitor2Board用の仮想テーブルへのポインタ。
 */
const DeviceVTable* vmonitor2_board_vtable(void);

#endif // VMONITOR2_BOARD_H

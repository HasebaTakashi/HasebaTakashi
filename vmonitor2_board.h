#ifndef VMONITOR2_BOARD_H
#define VMONITOR2_BOARD_H

#include "device_interface.h" // For DeviceVTable

/**
 * @brief VMonitor2Boardデバイスの新しいインスタンスを作成します。
 *
 * @return void* 作成された具象デバイスオブジェクトへのポインタ。
 *               このハンドルはSamplingDeviceによって管理されるべきです。
 */
void* vmonitor2_board_new(void);

/**
 * @brief VMonitor2Boardの仮想テーブルを取得します。
 *
 * @return const DeviceVTable* VMonitor2Board用の仮想テーブルへのポインタ。
 */
const DeviceVTable* vmonitor2_board_vtable(void);

#endif // VMONITOR2_BOARD_H

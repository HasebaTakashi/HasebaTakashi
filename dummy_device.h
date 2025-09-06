#ifndef DUMMY_DEVICE_H
#define DUMMY_DEVICE_H

#include "device_interface.h"

/**
 * @brief DummyDeviceの新しいインスタンスを作成します。
 *
 * @return void* 作成された具象デバイスオブジェクトへのポインタ。
 */
void* dummy_device_new(void);

/**
 * @brief DummyDeviceの仮想テーブルを取得します。
 *
 * @return const DeviceVTable* DummyDevice用の仮想テーブルへのポインタ。
 */
const DeviceVTable* dummy_device_vtable(void);

#endif // DUMMY_DEVICE_H

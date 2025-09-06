#ifndef DEVICE_TYPES_H
#define DEVICE_TYPES_H

// RubyのDeviceTypeモジュールに対応するデバイス種別のenum
typedef enum {
    DOCTOR_BOARD_ID = 1,
    WD_MICRO_BOARD_ID = 2,
    MICRO_LOGGER_BOARD_ID = 3,
    VMONITOR_BOARD_ID = 4,
    LIB3DH_ID = 5,
    ADT7410_ID = 6,
    WS_SENSOR_ID = 7,
    VMONITOR2_BOARD_ID = 8,
    DUMMY_DEVICE_ID = 99
} DeviceType;

#endif // DEVICE_TYPES_H

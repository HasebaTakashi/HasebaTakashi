#ifndef COMMANDS_H
#define COMMANDS_H

// --- サーバーコマンド文字列定義 ---
#define CMD_CHECK_VERSION "CheckVersion"
#define CMD_GET_CH_ID "GetChannelID"
#define CMD_GET_CH_INFO "GetChannelInfo"
#define CMD_CHECK_DATA "CheckData"
#define CMD_GET_DATA "GetData"
#define CMD_GET_DATA_NO "GetDataNo"
#define CMD_START_SAMPLING "StartSampling"
#define CMD_SET_DATA_TIME "SetDataTime"
#define CMD_SET_DATA_TIME_2 "SetDataTime2"
#define CMD_CLEAR_BUFFER "BufferClear"
#define CMD_GET_TERMINAL_VOLTAGE "GetTerminalVoltage"
#define CMD_GET_ALL_TERMINAL_VOLTAGES "GetAllTerminalVoltages"
#define CMD_GET_GAIN "GetGain"
#define CMD_SET_GAIN "SetGain"
#define CMD_GET_CH_DATA "GetChData"
#define CMD_GET_DI "GetDI"
#define CMD_GET_DO "GetDO"
#define CMD_SET_DO "SetDO"

// --- コマンド処理のレスポンスコード ---
#define CMD_RESPONSE_OK 0
#define CMD_ERR_BUFFER_NO -1
#define CMD_ERR_ARGUMENTS -2
#define CMD_ERR_UNKNOWN -3

#endif // COMMANDS_H

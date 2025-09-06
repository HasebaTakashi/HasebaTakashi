#ifndef APP_CONFIG_H
#define APP_CONFIG_H

// RubyのDeviceTypeモジュールで定義されていたデバイスID
#define VMONITOR2_BOARD 8

// vmonitor2_board.rb からの定数
// チャンネル数 (アナログ16ch + パルスカウンタ1ch)
#define VMONITOR2_CH_NO 17
// アナログチャンネル数
#define VMONITOR2_AD_CH_NO (VMONITOR2_CH_NO - 1)
// サンプリング周波数
#define VMONITOR2_SAMPLING_FREQUENCY 25600

// Rubyコードのapp_settingから読み込まれていた設定値
// 将来的には設定ファイルから読み込むことを想定
// パルスカウントしきい値 (mV)
#define PULSE_COUNT_THRESHOLD 12000
// コマンド未受信リブートタイプ (0x00:なし, 0x01:リセット, 0x02:電源OFF/ON)
#define IDLE_REBOOT_TYPE 0x02
// コマンド未受信リブートタイムアウト (秒)
#define IDLE_REBOOT_TIMEOUT 900

// sampling_manager.rb からの定数
// データ確認の間隔 (マイクロ秒)
#define CHECK_DATA_INTERVAL_USEC 300000 // 0.3秒
// データ無しタイムアウト秒数 (秒)
#define NO_DATA_TIME_OUT_SEC 1800

#endif // APP_CONFIG_H

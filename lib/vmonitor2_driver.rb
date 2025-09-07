# frozen_string_literal: true

# File: vmonitor2_driver.rb
# Author: Takashi Haseba
# Date: 2025-01-09
#
# Description:
#   This module is vmonitor2 library wrapper.
#
# Changes: 2025-04-24 Add GetVersion,GetIdleReboot,SetIdleReboot

require 'fiddle/import'

# ボードライブラリのラッパー
module VMonitor2Driver
  extend Fiddle::Importer
  dlload './lib/libvmonitor2.so'

  # デバイスオープン
  extern 'int VM2_Open()'
  # デバイスクローズ
  extern 'int VM2_Close()'
  # ADボードリセット
  extern 'int VM2_Reset(unsigned int dwDelaySec)'
  # 装置電源ONOFF
  extern 'int VM2_Restart(unsigned int dwDelaySec)'
  # デバイス初期化
  extern 'int VM2_Initialize()'
  # サンプリング開始
  extern 'int VM2_StartSampling()'
  # サンプリング停止
  extern 'int VM2_StopSampling()'
  # データ確認
  extern 'int VM2_CheckBuffer(int* pExist)'
  # データ取得
  extern 'int VM2_GetData(
                    short* pData1, short* pData2, short* pData3, short* pData4,
                    short* pData5, short* pData6, short* pData7, short* pData8,
                    short* pData9, short* pData10, short* pData11, short* pData12,
                    short* pData13, short* pData14, short* pData15, short* pData16,
                    int* plPulse)'
  # ステータスの取得
  extern 'int VM2_GetStatus(unsigned int* pdwStatus)'
  # 詳細エラーコードの取得
  extern 'int VM2_GetDetailErrorCode(unsigned int* pdwCode)'
  # ゲイン取得
  extern 'int VM2_GetGain(int ch, unsigned int* pdwGain)'
  # ゲイン設定
  extern 'int VM2_SetGain(int ch, unsigned int dwGain)'
  # 入力種別取得
  extern 'int VM2_GetInputSelect(int ch, unsigned int* pdwSelect)'
  # 入力種別設定
  extern 'int VM2_SetInputSelect(int ch, unsigned int dwSelect)'
  # 回転パルスしきい値取得（mVで取得）
  extern 'int VM2_GetPulseVoltage(unsigned int* pdwVol)'
  # 回転パルスしきい値設定（mVで設定）
  extern 'int VM2_SetPulseVoltage(unsigned int dwVol)'
  # 端子台電圧取得（mVで取得）
  extern 'int VM2_GetTerminalAd(int ch, unsigned int* pdwAd)'
  # デジタル入力状態を取得する
  extern 'int VM2_GetDigitalIn(int ch, unsigned int* pdwDigitalIn)'
  # デジタル出力状態を取得する
  extern 'int VM2_GetDigitalOut(int ch, unsigned int* pdwDigitalOut)'
  # デジタル出力状態を設定する
  extern 'int VM2_SetDigitalOut(int ch, unsigned int dwDigitalOut)'
  # バージョン取得
  extern 'int VM2_GetVersion(unsigned int* pdwVersion)'
  # コマンド未受信リセット設定取得
  extern 'int VM2_GetIdleReboot(unsigned int* pdwReboot, unsigned int* pdwPeriod)'
  # コマンド未受信リセット設定
  extern 'int VM2_SetIdleReboot(unsigned int dwReboot, unsigned int dwPeriod)'
end

# frozen_string_literal: false

require 'singleton'
require 'json'

# アプリケーション設定
class AnalyzeAppSettingManager
  include Singleton

  # サンプリングサーバのIPアドレス
  attr_accessor :sampling_server_ip
  # サンプリングサーバのポート番号
  attr_accessor :sampling_server_port
  # MQTTホストIPアドレス
  attr_accessor :mqtt_host_ip
  # MQTTポート番号
  attr_accessor :mqtt_port
  # MQTTデータ送信トピック
  attr_accessor :mqtt_send_topic
  # 計測秒数
  attr_accessor :measurement_sec
  # 計測インターバル秒数
  attr_accessor :measurement_interval_sec
  # 波形データを出力するか?
  attr_accessor :is_output_data
  # 波形データファイルの最大数
  attr_accessor :max_output_file_no
  # 波形データの出力秒数
  attr_accessor :output_data_sec
  # 波形データの書き出しパス
  attr_accessor :output_data_path
  # トリガタイプ
  attr_accessor :trigger_type
  # プリトリガ秒数
  attr_accessor :pretrigger_sec
  # トリガデバイスID
  attr_accessor :trigger_device_id
  # トリガチャンネルID
  attr_accessor :trigger_channel_id

  # 初期化
  def initialize
    # 設定ファイル名
    @setting_file_name = 'setting/analyze_app_setting.json'
    # サンプリングサーバのIPアドレス
    @sampling_server_ip = ''
    # サンプリングサーバのポート番号
    @sampling_server_port = 0
    # MQTTホストIPアドレス
    @mqtt_host_ip = ''
    # MQTTポート番号
    @mqtt_port = 0
    # MQTTデータ送信トピック
    @mqtt_send_topic = ''
    # 計測秒数
    @measurement_sec = 0
    # 計測インターバル秒数
    @measurement_interval_sec = 10
    # データ出力するか?
    @is_output_data = 0
    # データ出力ファイルの最大数
    @max_output_file_no = 0
    # 波形データの出力秒数
    @output_data_sec = 0
    # 書き出しデータのパス
    @output_data_path = ''
    # トリガタイプ
    @trigger_type = 0
    # プリトリガ秒数
    @pretrigger_sec = 0
     # トリガデバイスID
    @trigger_device_id = 0
    # トリガチャンネルID
    @trigger_channel_id = 0
  end

  # 設定読み込み
  #
  # @return [AnalyzerAppSettingManager] 設定インスタンスを返す
  def read_setting
    return unless File.exist?(@setting_file_name)

    file_data = ''
    # ファイルオープン
    File.open(@setting_file_name, 'r') do |f|
      # json読み出し
      file_data = f.read.strip
    end
    # ハッシュ配列に変換
    setting_data = JSON.parse(file_data)

    @sampling_server_ip = setting_data['SamplingServerIP']
    @sampling_server_port = setting_data['SamplingServerPort'].to_i
    @mqtt_host_ip = setting_data['MQTTHostIP']
    @mqtt_port = setting_data['MQTTPort'].to_i
    @mqtt_send_topic = setting_data['MQTTSendTopic']
    @measurement_sec = setting_data['MeasurementSec'].to_i
    @measurement_interval_sec = setting_data['MeasurementIntervalSec'].to_f
    @is_output_data = setting_data['OutputData'].to_i
    @max_output_file_no = setting_data['MaxOutputFileNo'].to_i
    @output_data_sec = setting_data['OutputDataSec'].to_i
    @trigger_type = setting_data['TriggerType'].to_i
    @pretrigger_sec = setting_data['PreTriggerSec'].to_i
    @trigger_device_id = setting_data['TriggerInputDeviceID'].to_i
    @trigger_channel_id = setting_data['TriggerInputChannelNo'].to_i
    @output_data_path = setting_data['OutputDataPath']


    self
  end

  # 設定書き込み
  def write_setting
    setting_data_hash = {}

    setting_data_hash['SamplingServerIP'] = @sampling_server_ip
    setting_data_hash['SamplingServerPort'] = @sampling_server_port
    setting_data_hash['MQTTHostIP'] = @mqtt_host_ip
    setting_data_hash['MQTTPort'] = @mqtt_port
    setting_data_hash['MQTTSendTopic'] = @mqtt_send_topic
    setting_data_hash['MeasurementSec'] = @measurement_sec
    setting_data_hash['MeasurementIntervalSec'] = @measurement_interval_sec
    setting_data_hash['OutputData'] = @is_output_data
    setting_data_hash['MaxOutputFileNo'] = @max_output_file_no
    setting_data_hash['OutputDataSec'] = @output_data_sec
    setting_data_hash['TriggerType'] = @trigger_type
    setting_data_hash['PreTriggerSec'] = @pretrigger_sec
    setting_data_hash['TriggerInputDeviceID'] = @trigger_device_id
    setting_data_hash['TriggerInputChannelNo'] = @trigger_channel_id
    setting_data_hash['OutputDataPath'] = @output_data_path

    setting_data = JSON.pretty_generate(setting_data_hash)

    # ファイルオープン
    File.open(@setting_file_name, 'w') do |f|
      # json書き出し
      f.puts(setting_data)
    end
  end
end

# frozen_string_literal: true

require 'sinatra'

require_relative 'sampling_app_setting'
require_relative 'device_info'
require_relative 'device_setting'
require_relative 'channel_setting'
require_relative 'analyze_app_setting'
require_relative 'analyze_setting'
# require_relative 'sampling_app_setting_controller' # NOTE: 存在しないためコメントアウト
# require_relative 'analyze_app_setting_controller' # NOTE: 存在しないためコメントアウト

# サーバ設定
# ENV['RACK_ENV'] = 'development'
ENV['RACK_ENV'] = 'production'
set :bind, '0.0.0.0'
set :port, 39000

# バージョン
version = '1.0.0'

# 設定読み出し
SamplingAppSettingManager.instance.read_setting
DeviceInfoManager.instance.read_setting
DeviceSettingManager.instance.read_setting
ChannelSettingManager.instance.read_setting
AnalyzeAppSettingManager.instance.read_setting
AnalyzeSettingManager.instance.read_setting

# サーバ処理
# メイン画面
get '/' do
  @title = 'Menu'
  @version = version
  @device_name = DeviceInfoManager.instance.device_name
  erb :index
end

# 再起動
get '/reboot' do
  system('sudo systemctl reboot')
  redirect '/'
end

# frozen_string_literal: true

# 分析アプリの設定
get '/analyzeappsetting' do
  @title = 'AnalyzeAppSetting'
  @analyze_app_setting = AnalyzeAppSettingManager.instance
  erb :analyze_app_setting
end

# 分析アプリの設定編集
post '/editanalyzeapp' do
  # サンプリングサーバのIPアドレス
  AnalyzeAppSettingManager.instance.sampling_server_ip = params['samplingserverip']
  # サンプリングサーバのポート番号
  AnalyzeAppSettingManager.instance.sampling_server_port = params['samplingserverport'].to_i
  # MQTTのホストIP
  AnalyzeAppSettingManager.instance.mqtt_host_ip = params['mqtthostip']
  # MQTTのポート番号
  AnalyzeAppSettingManager.instance.mqtt_port = params['mqttport'].to_i
  # MQTTの送信トピック
  AnalyzeAppSettingManager.instance.mqtt_send_topic = params['mqttsendtopic']
  # 計測間隔
  AnalyzeAppSettingManager.instance.measurement_interval_sec = params['measurementintervalsec'].to_i
  # 計測秒数
  AnalyzeAppSettingManager.instance.measurement_sec = params['measurementsec'].to_i
  # プレトリガ秒数
  AnalyzeAppSettingManager.instance.pretrigger_sec = params['pretriggersec'].to_i
  # トリガ入力のデバイスID
  AnalyzeAppSettingManager.instance.trigger_device_id = params['triggerinputdeviceid'].to_i
  # トリガ入力のチャンネル番号
  AnalyzeAppSettingManager.instance.trigger_channel_id = params['triggerinputchannelid'].to_i
  # データ出力するか?
  AnalyzeAppSettingManager.instance.is_output_data = params['isoutputdata'].to_i
  # データ出力ファイルの最大数
  AnalyzeAppSettingManager.instance.max_output_file_no = params['maxoutputfileno'].to_i
  # 波形データの出力秒数
  AnalyzeAppSettingManager.instance.output_data_sec = params['outputdatasec'].to_i
  # 書き出しデータのパス
  AnalyzeAppSettingManager.instance.output_data_path = params['outputdatapath']
  # トリガタイプ


  AnalyzeAppSettingManager.instance.write_setting
  redirect '/analyzeappsetting'
end

# 分析設定
get '/analyzesetting' do
  @title = 'AnalyzeSetting'
  @analyze_settings = AnalyzeSettingManager.instance.analyze_setting_list
  erb :analyze_setting
end

# 分析編集
post '/editanalyze' do
  analyze_settings = AnalyzeSettingManager.instance.analyze_setting_list

  analyze_settings.each do |setting|
    # 分析名
    key = "analyzename#{setting.analyze_id}"
    setting.analyze_name = params[key]
    # データチャンネル
    key = "datachannelid#{setting.analyze_id}"
    setting.data_channel_id = params[key].to_i
    # 分析タイプ
    key = "analyzetype#{setting.analyze_id}"
    setting.analyze_type = params[key].to_i
    # パス
    key = "analyzepath#{setting.analyze_id}"
    setting.analyze_path = params[key]
    # コマンド
    key = "analyzecommand#{setting.analyze_id}"
    setting.analyze_command = params[key]
    # 引数
    key = "analyzearguments#{setting.analyze_id}"
    setting.analyze_arguments = params[key]
    # 有効無効
    key = "analyzeenable#{setting.analyze_id}"
    setting.enable_analyze = params[key].to_i
  end

  AnalyzeSettingManager.instance.analyze_setting_list = analyze_settings
  AnalyzeSettingManager.instance.write_setting

  redirect '/analyzesetting'
end

# 分析追加
get '/addanalyze' do
  analyze_settings = AnalyzeSettingManager.instance.analyze_setting_list

  # 追加データID
  new_analyze_id = 0
  exist_id = []
  analyze_settings.each do |setting|
    exist_id << setting.analyze_id
  end
  # 追加IDを検索
  (1...1000).each do |i|
   next if exist_id.include?(i)

   new_analyze_id = i
   break
  end

  added_analyze = AnalyzeSetting.new(
    new_analyze_id,
    "New Analyzer#{new_analyze_id}",
    1,
    '',
    '',
    '',
    1,
    1
  )

  analyze_settings << added_analyze

  AnalyzeSettingManager.instance.analyze_setting_list = analyze_settings
  AnalyzeSettingManager.instance.write_setting

  redirect '/analyzesetting'
end

# 分析削除
get '/deleteanalyzer/:id' do
  delete_id = params[:id].to_i

  analyze_settings = AnalyzeSettingManager.instance.analyze_setting_list
  # 指定IDの分析を削除
  analyze_settings.delete_if { |setting| setting.analyze_id == delete_id }


  AnalyzeSettingManager.instance.analyze_setting_list = analyze_settings
  AnalyzeSettingManager.instance.write_setting

  redirect '/analyzesetting'
end

# サンプリングサーバ設定
get '/samplingappsetting' do
  @title = 'SamplingAppSetting'
  @sampling_app_setting = SamplingAppSettingManager.instance
  erb :sampling_app_setting
end

# サンプリングサーバの設定編集
post '/editsamplingapp' do
  p params
  # バッファ数
  SamplingAppSettingManager.instance.buffer_no = params['bufferno'].to_i
  # バッファサイズ
  SamplingAppSettingManager.instance.buffer_no.times do |i|
    SamplingAppSettingManager.instance.buffer_size_hash[i + 1] = params["buffer#{i + 1}size"].to_i
  end
  # サンプリングサーバのIPアドレス
  SamplingAppSettingManager.instance.server_ip = params['serverip']
  # サンプリングサーバのポート番号
  SamplingAppSettingManager.instance.server_port = params['serverport'].to_i
  # サーバタイプ
  SamplingAppSettingManager.instance.server_type = params['servertype'].to_i

  SamplingAppSettingManager.instance.write_setting
  redirect '/samplingappsetting'
end

# デバイス設定
get '/devicesetting' do
  @title = 'DeviceSetting'
  @device_settings = DeviceSettingManager.instance.device_setting_list
  erb :device_setting
end

# デバイス編集
post '/editdevice' do
  device_settings = DeviceSettingManager.instance.device_setting_list

  device_settings.each do |setting|
    # 有効無効
    key = "deviceenable#{setting.device_id}"
    setting.enable_device = params[key].to_i
    p setting
  end

  DeviceSettingManager.instance.device_setting_list = device_settings
  DeviceSettingManager.instance.write_setting

  redirect '/devicesetting'
end

# チャンネル設定
get '/channelsetting' do
  @title = 'ChannelSetting'
  @channel_settings = ChannelSettingManager.instance.channel_setting_list
  erb :channel_setting
end

# チャンネル編集
post '/editchannel' do
  channel_settings = ChannelSettingManager.instance.channel_setting_list

  channel_settings.each do |setting|
    # チャンネル名
    key = "channelname#{setting.channel_id}"
    setting.channel_name = params[key]
    # デバイスID
    key = "channeldeviceid#{setting.channel_id}"
    setting.device_id = params[key].to_i
    # インデックス
    key = "channelindex#{setting.channel_id}"
    setting.ch_index = params[key].to_i
    # サンプリング周波数
    key = "channelsamplingfrequency#{setting.channel_id}"
    setting.sampling_freq = params[key].to_i
    # ゲイン
    key = "channelgain#{setting.channel_id}"
    setting.gain = params[key].to_f
    # 最大レンジ
    key = "channelmaxrange#{setting.channel_id}"
    setting.max_range = params[key].to_f
    # 最小レンジ
    key = "channelminrange#{setting.channel_id}"
    setting.min_range = params[key].to_f
    # 解像度
    key = "channelresolution#{setting.channel_id}"
    setting.resolution = params[key].to_i
    # 入力種別
    key = "inputtype#{setting.channel_id}"
    setting.input_type = params[key].to_i
  end

  ChannelSettingManager.instance.channel_setting_list = channel_settings
  ChannelSettingManager.instance.write_setting

  redirect '/channelsetting'
end

# frozen_string_literal: false

require 'singleton'
require 'json'

# 分析設定
class AnalyzeSettingManager
  include Singleton

  # 分析設定リスト
  attr_accessor :analyze_setting_list

  # 初期化
  def initialize
    # 設定ファイル名
    @setting_file_name = 'setting/analyze_setting.json'
    # 分析設定リスト
    @analyze_setting_list = []
  end

  # 設定読み込み
  #
  # @return [Array] 読み込んだ分析設定（AnalyzerSetting）の配列
  def read_setting
    @analyze_setting_list.clear
    return unless File.exist?(@setting_file_name)

    file_data = ''
    # ファイルオープン
    File.open(@setting_file_name, 'r') do |f|
      # json読み出し
      file_data = f.read.strip
    end
    # ハッシュ配列に変換
    p setting_data = JSON.parse(file_data)

    # デバイス設定インスタンス生成
    setting_data.each do |st|
      puts(st)
      # 渡されるのは、グループ名と設定のハッシュなので、添字１を指定
      analyze_setting = AnalyzeSetting.new(
        st[1]['AnalyzeID'].to_i,
        st[1]['AnalyzeName'],
        st[1]['DataChannelID'].to_i,
        st[1]['AnalyzeType'].to_i,
        st[1]['AnalyzePath'],
        st[1]['AnalyzeCommand'],
        st[1]['AnalyzeArguments'],
        st[1]['EnableAnalyze'].to_i
      )
      # リストに登録
      @analyze_setting_list.push(analyze_setting)
    end
    @analyze_setting_list
  end

  # 設定書き込み
  def write_setting
    setting_data_hash = {}
    @analyze_setting_list.each_with_index do |setting, i|
      tmp_data = {}
      tmp_data['AnalyzeID'] = setting.analyze_id
      tmp_data['AnalyzeName'] = setting.analyze_name
      tmp_data['DataChannelID'] = setting.data_channel_id
      tmp_data['AnalyzeType'] = setting.analyze_type
      tmp_data['AnalyzePath'] = setting.analyze_path
      tmp_data['AnalyzeCommand'] = setting.analyze_command
      tmp_data['AnalyzeArguments'] = setting.analyze_arguments
      tmp_data['EnableAnalyze'] = setting.enable_analyze
      setting_data_hash["Analyze#{i + 1}"] = tmp_data
    end

    setting_data = JSON.pretty_generate(setting_data_hash)

    # ファイルオープン
    File.open(@setting_file_name, 'w') do |f|
      # json書き出し
      f.puts(setting_data)
    end
  end
end

# 分析設定
class AnalyzeSetting
  # 分析ID
  attr_accessor :analyze_id
  # 分析名
  attr_accessor :analyze_name
  # 分析データのチャンネルID
  attr_accessor :data_channel_id
  # 分析種別
  attr_accessor :analyze_type
  # カスタム分析のパス
  attr_accessor :analyze_path
  # カスタム分析のコマンド
  attr_accessor :analyze_command
  # 分析引数
  attr_accessor :analyze_arguments
  # 分析が有効か？
  attr_accessor :enable_analyze

  def initialize(id, name, data_channel_id, analyzer_type, analyzer_path, analyzer_command, analyzer_arguments, enable)
    # 分析ID
    @analyze_id = id
    # 分析名
    @analyze_name = name
    # 分析データのチャンネルID
    @data_channel_id = data_channel_id
    # 分析種別
    @analyze_type = analyzer_type
    # カスタム分析のパス
    @analyze_path = analyzer_path
    # カスタム分析のコマンド
    @analyze_command = analyzer_command
    # 実行時の引数
    @analyze_arguments = analyzer_arguments
    # 有効
    @enable_analyze = enable
  end
end

# frozen_string_literal: false

# File: channel_setting.rb
# Author: Takashi Haseba
# Date: 2024-07-29
#
# Description:
#   This script is read channel setting.
#
# Changes:

require 'singleton'
require 'json'

# チャンネル設定
class ChannelSettingManager
  include Singleton

  # チャンネル設定リスト
  attr_accessor :channel_setting_list

  # 初期化
  def initialize
    # 設定ファイル名
    @setting_file_name = 'setting/channel_setting.json'
    # チャンネル設定リスト
    @channel_setting_list = []
  end

  # 設定読み込み
  def read_setting
    @channel_setting_list.clear
    return unless File.exist?(@setting_file_name)

    file_data = ''
    # ファイルオープン
    File.open(@setting_file_name, 'r') do |f|
      # json読み出し
      file_data = f.read.strip
    end

    # ハッシュ配列に変換
    setting_data = JSON.parse(file_data)

    # デバイス設定インスタンス生成
    setting_data.each do |st|
      puts("#{st}")
      # p st
      # 渡されるのは、グループ名と設定のハッシュなので、添字１を指定
      p channel_setting = ChannelSetting.new(
        st[1]['ChannelID'].to_i,
        st[1]['ChannelName'],
        st[1]['DeviceID'].to_i,
        st[1]['ChannelIndex'].to_i,
        st[1]['SamplingFrequency'].to_i,
        st[1]['Gain'].to_f,
        st[1]['MaxRange'].to_f,
        st[1]['MinRange'].to_f,
        st[1]['Resolution'].to_i,
        st[1]['InputType'].to_i
      )
      # リストに登録
      @channel_setting_list.push(channel_setting)
    end
    @channel_setting_list
  end

  # 設定書き込み
  def write_setting
    setting_data_hash = {}
    @channel_setting_list.each_with_index do |setting, i|
      tmp_data = {}
      tmp_data['ChannelID'] = setting.channel_id
      tmp_data['ChannelName'] = setting.channel_name
      tmp_data['DeviceID'] = setting.device_id
      tmp_data['ChannelIndex'] = setting.ch_index
      tmp_data['SamplingFrequency'] = setting.sampling_freq
      tmp_data['Gain'] = setting.gain
      tmp_data['MaxRange'] = setting.max_range
      tmp_data['MinRange'] = setting.min_range
      tmp_data['Resolution'] = setting.resolution
      tmp_data['InputType'] = setting.input_type
      setting_data_hash["Channel#{i + 1}"] = tmp_data
    end

    setting_data = JSON.pretty_generate(setting_data_hash)

    # ファイルオープン
    File.open(@setting_file_name, 'w') do |f|
      # json書き出し
      f.puts(setting_data)
    end
  end
end

# チャンネル設定情報
class ChannelSetting
  # チャンネルID
  attr_accessor :channel_id
  # チャンネル名
  attr_accessor :channel_name
  # このチャンネルが属するデバイスのID
  attr_accessor :device_id
  # このチャンネルの属するデバイスでのインデックス
  attr_accessor :ch_index
  # チャンネルのサンプリング周波数
  attr_accessor :sampling_freq
  # チャンネルのゲイン
  attr_accessor :gain
  # チャンネルの最大レンジ
  attr_accessor :max_range
  # チャンネルの最小レンジ
  attr_accessor :min_range
  # チャンネルの分解能
  attr_accessor :resolution
  # 入力種別
  attr_accessor :input_type

  # 初期化
  def initialize(id, name, device_id, ch_index, sampling_freq, gain, max_range, min_range, resolution, input_type)
    # チャンネルID
    @channel_id = id
    # チャンネル名
    @channel_name = name
    # デバイスID
    @device_id = device_id
    # デバイス内のチャンネルインデックス
    @ch_index = ch_index
    # サンプリング周波数
    @sampling_freq = sampling_freq
    # ゲイン
    @gain = gain
    # 最大レンジ
    @max_range = max_range
    # 最小レンジ
    @min_range = min_range
    # 解像度
    @resolution = resolution
    # 入力種別
    @input_type = input_type
  end
end

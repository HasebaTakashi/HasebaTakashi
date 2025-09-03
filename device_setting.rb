# frozen_string_literal: false

# File: device_setting.rb
# Author: Takashi Haseba
# Date: 2024-07-29
#
# Description:
#   This script is read device setting.
#
# Changes:

require 'singleton'
require 'json'

# デバイス設定
class DeviceSettingManager
  include Singleton

  # デバイス設定リスト
  attr_accessor :device_setting_list

  # 初期化
  def initialize
    # 設定ファイル名
    @setting_file_name = 'setting/device_setting.json'
    # デバイス設定リスト
    @device_setting_list = []
  end

  # 設定読み込み
  def read_setting
    @device_setting_list.clear
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
      device_setting = DeviceSetting.new(
        st[1]['DeviceID'].to_i,
        st[1]['DeviceName'],
        st[1]['ChannelNo'].to_i,
        st[1]['EnableDevice'].to_i
      )
      # リストに登録
      @device_setting_list.push(device_setting)
    end
    @device_setting_list
  end

  # 設定書き込み
  def write_setting
    setting_data_hash = {}
    @device_setting_list.each_with_index do |setting, i|
      tmp_data = {}
      tmp_data['DeviceID'] = setting.device_id
      tmp_data['DeviceName'] = setting.device_name
      tmp_data['ChannelNo'] = setting.channel_no
      tmp_data['EnableDevice'] = setting.enable_device
      setting_data_hash["Device#{i + 1}"] = tmp_data
    end

    setting_data = JSON.pretty_generate(setting_data_hash)

    # ファイルオープン
    File.open(@setting_file_name, 'w') do |f|
      # json書き出し
      f.puts(setting_data)
    end
  end
end

# デバイス設定情報
class DeviceSetting
  # デバイスID
  attr_accessor :device_id
  # デバイス名
  attr_accessor :device_name
  # デバイスのチャンネル数
  attr_accessor :channel_no
  # デバイスの有効無効
  attr_accessor :enable_device

  # 初期化
  def initialize(id, name, channel_no, enable)
    # デバイスID
    @device_id = id
    # デバイス名
    @device_name = name
    # チャンネル数
    @channel_no = channel_no
    # 有効
    @enable_device = enable
  end
end

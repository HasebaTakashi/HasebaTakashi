# frozen_string_literal: false

require 'singleton'
require 'json'

# require_relative '../logger/applogger'

# デバイス情報ファイル操作
class DeviceInfoManager
  include Singleton
  attr_accessor :device_name

  def initialize
    # 設定ファイル名
    @setting_file_name = 'setting/device_info.json'
    # デバイス名
    @device_name = ''
  end

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

    # 情報をセット
    @device_name = setting_data['DeviceName']
  end

  # 設定書き込み
  def write_setting
    setting_data_hash = {}
    # 情報をセット
    setting_data_hash['DeviceName'] = @device_name
    setting_data = JSON.pretty_generate(setting_data_hash)

    # ファイルオープン
    File.open(@setting_file_name, 'w') do |f|
      # json書き出し
      f.puts(setting_data)
    end
  end
end

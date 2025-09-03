# frozen_string_literal: false

# File: app_setting.rb
# Author: Takashi Haseba
# Date: 2024-07-29
#
# Description:
#   This script is read application setting.
#
# Changes:

require 'singleton'
require 'json'

# アプリケーション設定
class SamplingAppSettingManager
  include Singleton

  # デフォルトのバッファサイズ
  DEFAULT_BUFFER_SIZE = 30

  # バッファ数
  attr_accessor :buffer_no
  # サーバタイプ（1:Socket,2:druby）
  attr_accessor :server_type
  # サーバIPアドレス
  attr_accessor :server_ip
  # サーバポート番号
  attr_accessor :server_port
  # 各バッファのデータ数のハッシュ
  attr_accessor :buffer_size_hash

  # 初期化（newできないので、最初のgetinstanceで呼ばれる）
  def initialize
    # 設定ファイル名
    @setting_file_name = 'setting/app_setting.json'
    # バッファ数
    @buffer_no = 0
    # 各バッファのデータ数のハッシュ
    @buffer_size_hash = {}
    # サーバタイプ
    @server_type = 0
    # サーバIPアドレス
    @server_ip = ''
    # サーバポート番号
    @server_port = 0
  end

  # バッファサイズ取得
  def buffer_size(buffer_no)
    @buffer_size_hash[buffer_no]
  end

  # バッファサイズのキーを返す
  def buffer_size_keys
    @buffer_size_hash.keys
  end

  # 設定読み込み
  def read_setting
    return unless File.exist?(@setting_file_name)
    file_data = ''
    # ファイルオープン
    File.open(@setting_file_name, 'r') do |f|
      # json読み出し
      file_data = f.read.strip
    end
    # ハッシュ配列に変換
    p setting_data = JSON.parse(file_data)

    @buffer_no = setting_data['BufferNo'].to_i
    # バッファのサイズを取得（無い設定は、デフォルトを入れる）
    @buffer_no.times do |i|
      size = setting_data["Buffer#{i + 1}Size"].to_i
      size = DEFAULT_BUFFER_SIZE if size.zero?
      @buffer_size_hash[i + 1] = size
    end
    p "BufferSizeHash:#{@buffer_size_hash}"
    p @server_type = setting_data['ServerType'].to_i
    p @server_ip = setting_data['ServerIP']
    p @server_port = setting_data['ServerPort'].to_i

    self
  end

  # 設定書き込み
  def write_setting
    setting_data_hash = {}

    setting_data_hash['BufferNo'] = @buffer_no
    @buffer_size_hash.each_pair do |k, v|
      setting_data_hash["Buffer#{k}Size"] = v
    end
    setting_data_hash['ServerType'] = @server_type
    setting_data_hash['ServerIP'] = @server_ip
    setting_data_hash['ServerPort'] = @server_port

    setting_data = JSON.pretty_generate(setting_data_hash)

    # ファイルオープン
    File.open(@setting_file_name, 'w') do |f|
      # json書き出し
      f.puts(setting_data)
    end
  end
end

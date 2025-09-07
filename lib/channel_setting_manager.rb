# frozen_string_literal: true

require 'json'
require 'fileutils'
require_relative 'channel_setting'

# ChannelSettingManager: Manages channel-specific settings.
# It loads a list of channels from a JSON file and handles backup and recovery.
class ChannelSettingManager
  attr_reader :channel_settings

  def initialize(file_path)
    @file_path = file_path
    @backup_path = "#{file_path}.bak"
    @raw_settings = []
    @channel_settings = []
    load_settings
  end

  # Find a channel setting by its ID
  def find_by_id(id)
    @channel_settings.find { |cs| cs.channel_id == id }
  end

  private

  def load_settings
    if File.exist?(@file_path)
      load_from_primary
    elsif File.exist?(@backup_path)
      puts "Primary channel settings file not found. Restoring from backup."
      load_from_backup(true)
    else
      raise "Channel settings file and backup file not found: #{@file_path}"
    end
    assign_settings
  end

  def load_from_primary
    begin
      content = File.read(@file_path)
      @raw_settings = JSON.parse(content)
      FileUtils.cp(@file_path, @backup_path)
    rescue JSON::ParserError => e
      puts "Error parsing channel settings file: #{e.message}. Attempting to load from backup."
      load_from_backup(true)
    end
  end

  def load_from_backup(restore_primary)
    unless File.exist?(@backup_path)
      raise "Backup channel settings file not found, and primary file is corrupted or missing."
    end

    content = File.read(@backup_path)
    @raw_settings = JSON.parse(content)

    FileUtils.cp(@backup_path, @file_path) if restore_primary
  end

  def assign_settings
    @channel_settings = @raw_settings.map do |s|
      ChannelSetting.new(
        s['channel_id'],
        s['channel_name'],
        s['device_id'],
        s['channel_index'],
        s['sampling_frequency'],
        s['gain'],
        s['max_range'],
        s['min_range'],
        s['resolution'],
        s['input_type']
      )
    end
  end
end

# frozen_string_literal: true

require 'json'
require 'fileutils'

# AppSettingManager: Manages application-wide settings.
# It loads settings from a JSON file and handles backup and recovery.
class AppSettingManager
  attr_reader :tcp_port, :sampling_queue_size, :pulse_count_threshold,
              :no_command_reboot_type, :no_command_reboot_timeout,
              :log_file_path, :log_level

  def initialize(file_path)
    @file_path = file_path
    @backup_path = "#{file_path}.bak"
    @settings = {}
    load_settings
  end

  private

  def load_settings
    if File.exist?(@file_path)
      load_from_primary
    elsif File.exist?(@backup_path)
      puts "Primary settings file not found. Restoring from backup."
      load_from_backup(true) # force restore
    else
      raise "Settings file and backup file not found: #{@file_path}"
    end
    assign_settings
  end

  def load_from_primary
    begin
      content = File.read(@file_path)
      @settings = JSON.parse(content)
      # On successful read, create a backup
      FileUtils.cp(@file_path, @backup_path)
    rescue JSON::ParserError => e
      puts "Error parsing settings file: #{e.message}. Attempting to load from backup."
      load_from_backup(true) # force restore
    end
  end

  def load_from_backup(restore_primary)
    unless File.exist?(@backup_path)
      raise "Backup settings file not found, and primary file is corrupted or missing."
    end

    content = File.read(@backup_path)
    @settings = JSON.parse(content) # If backup is also corrupt, this will raise an error.

    # Restore the primary file from the backup
    FileUtils.cp(@backup_path, @file_path) if restore_primary
  end

  def assign_settings
    @tcp_port = @settings['tcp_port']
    @sampling_queue_size = @settings['sampling_queue_size']
    @pulse_count_threshold = @settings['pulse_count_threshold']
    @no_command_reboot_type = @settings['no_command_reboot_type']
    @no_command_reboot_timeout = @settings['no_command_reboot_timeout']
    @log_file_path = @settings['log_file_path']
    @log_level = @settings['log_level']
  end
end

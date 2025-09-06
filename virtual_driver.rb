# frozen_string_literal: true

#
# VirtualDriver: Main Application
#
# This is the main entry point for the VirtualDriver application.
# It loads all necessary components, initializes them, and starts the main loop.
#

# Load all component classes
# Note: The order of requiring files can be important if there are dependencies.
# For now, this simple glob is fine.
Dir["#{File.dirname(__FILE__)}/lib/*.rb"].each { |file| require_relative file }

class VirtualDriver
  def initialize
    # 1. Initialize settings
    @app_settings = AppSettingManager.new('config/app_setting.json')

    # 2. Configure the logger
    @logger = AppLogger.instance
    @logger.configure(
      log_path: @app_settings.log_file_path,
      level: @app_settings.log_level
    )

    @logger.info('VirtualDriver initializing...')

    # 3. Load device and channel configurations
    @device_settings_manager = DeviceSettingManager.new('config/device_setting.json')
    @logger.info("Loaded #{@device_settings_manager.device_settings.count} device(s).")

    @channel_settings_manager = ChannelSettingManager.new('config/channel_setting.json')
    @logger.info("Loaded #{@channel_settings_manager.channel_settings.count} channel(s).")

    @logger.info('Initialization complete.')
  rescue => e
    # If anything goes wrong during init, log to STDOUT as logger may not be configured
    puts "FATAL: Failed to initialize VirtualDriver: #{e.message}"
    puts e.backtrace
    # Also try to log to file if logger was initialized
    AppLogger.instance.fatal("Failed to initialize VirtualDriver: #{e.message}") if @logger
    exit 1
  end

  def run
    @logger.info('VirtualDriver starting main loop...')
    # TODO: Implement main application loop
    # - Start CommandServer
    # - Start SamplingManager
    # - Watch for vd_stop.txt

    # For now, just sleep to simulate a running process
    sleep(1) while true
  end

  def self.start
    new.run
  end
end

if __FILE__ == $PROGRAM_NAME
  puts 'Starting VirtualDriver application...'
  VirtualDriver.start
end

# frozen_string_literal: true

Dir["#{File.dirname(__FILE__)}/lib/*.rb"].each { |file| require_relative file }

class VirtualDriver
  STOP_FILE = 'vd_stop.txt'

  def initialize
    @app_settings = AppSettingManager.new('config/app_setting.json')

    @logger = AppLogger.instance
    @logger.configure(
      log_path: @app_settings.log_file_path,
      level: @app_settings.log_level
    )

    @logger.info('VirtualDriver initializing...')

    @device_settings_manager = DeviceSettingManager.new('config/device_setting.json')
    @logger.info("Loaded #{@device_settings_manager.device_settings.count} device(s).")

    @channel_settings_manager = ChannelSettingManager.new('config/channel_setting.json')
    @logger.info("Loaded #{@channel_settings_manager.channel_settings.count} channel(s).")

    # Initialize core components
    @sampling_queue = SamplingQueue.new(@app_settings.sampling_queue_size)
    @sampling_manager = SamplingManager.new(@app_settings, @device_settings_manager, @channel_settings_manager, @sampling_queue)
    @command_server = CommandServer.new(@app_settings.tcp_port, @app_settings, @device_settings_manager, @channel_settings_manager, @sampling_queue, @sampling_manager)

    @logger.info('Initialization complete.')
  rescue => e
    puts "FATAL: Failed to initialize VirtualDriver: #{e.message}"
    puts e.backtrace
    AppLogger.instance.fatal("Failed to initialize VirtualDriver: #{e.message}") if @logger
    exit 1
  end

  def run
    @logger.info('VirtualDriver starting services...')
    @sampling_manager.start
    @command_server.start

    # Graceful shutdown setup
    Signal.trap('INT') { shutdown }
    Signal.trap('TERM') { shutdown }

    @logger.info('VirtualDriver running. Press Ctrl+C or create vd_stop.txt to exit.')

    # Main loop: watch for stop file
    while !File.exist?(STOP_FILE)
      sleep(1)
    end

    shutdown
  end

  def shutdown
    @logger.info('Shutdown signal received. Stopping services...')

    @command_server.stop
    @sampling_manager.stop

    File.delete(STOP_FILE) if File.exist?(STOP_FILE)

    @logger.info('VirtualDriver has shut down gracefully.')
    exit 0
  end

  def self.start
    new.run
  end
end

if __FILE__ == $PROGRAM_NAME
  puts 'Starting VirtualDriver application...'
  # Clean up stop file on start
  File.delete(VirtualDriver::STOP_FILE) if File.exist?(VirtualDriver::STOP_FILE)
  VirtualDriver.start
end

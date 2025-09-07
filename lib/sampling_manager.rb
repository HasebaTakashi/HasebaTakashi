# frozen_string_literal: true

require 'thread'

# SamplingManager: Core Logic / Orchestrator (Corrected Final Version)
# This class manages sampling devices. It runs a processing loop that
# polls the hardware, and when a 1-second data block is ready, it fetches
# it and pushes it to the sampling queue.
class SamplingManager
  attr_reader :devices

  def initialize(app_settings, device_settings_manager, channel_settings_manager, sampling_queue)
    @logger = AppLogger.instance
    @app_settings = app_settings
    @sampling_queue = sampling_queue
    @running = false
    @mutex = Mutex.new

    # Initialize devices
    @devices = device_settings_manager.device_settings.map do |ds|
      next unless ds.enable_device == 1
      SamplingDevice.new(ds)
    end.compact

    # Initialize channels and associate them with devices
    @channels = {}
    channel_settings_manager.channel_settings.each do |cs|
      channel = SamplingChannel.new(cs)
      @channels[cs.channel_id] = channel

      device = @devices.find { |d| d.device_id == cs.device_id }
      device&.register_channel(channel)
    end

    # Dynamically create pulse channels for each device
    @devices.each do |device|
        pulse_channel_id = (device.device_id * 1000) + 999
        freq = @channels.values.find { |c| c.device_id == device.device_id }&.sampling_frequency || 1000
        pulse_setting = ChannelSetting.new(pulse_channel_id, "Pulse-CH-#{device.device_id}", device.device_id, 16, freq, 1, 0, 0, 0, "Pulse")
        pulse_channel = SamplingChannel.new(pulse_setting)
        @channels[pulse_channel_id] = pulse_channel
        device.register_channel(pulse_channel)
    end

    @sampling_frequency = @channels.values.first&.sampling_frequency || 1000
    @logger.info("SamplingManager initialized with #{@devices.count} devices and #{@channels.count} channels.")
  end

  def start
    @mutex.synchronize { return if @running; @running = true }

    @logger.info("Starting SamplingManager...")
    @devices.each(&:open)
    @devices.each(&:start_sampling)

    @processing_thread = Thread.new { processing_loop }
    @logger.info("SamplingManager started.")
  end

  def stop
    @mutex.synchronize { return unless @running; @running = false }

    @logger.info("Stopping SamplingManager...")
    @processing_thread&.join
    @devices.each(&:stop_sampling)
    @devices.each(&:close)
    @logger.info("SamplingManager stopped.")
  end

  private

  def processing_loop
    while @running
      @devices.each do |device|
        # If a 1-second block is not ready in the hardware buffer, skip.
        next unless device.check_data

        @logger.debug("Data found for device #{device.device_id}. Fetching...")

        # The frequency is needed by the board to know how much memory to allocate
        device_frequency = @channels.values.find { |c| c.device_id == device.device_id }&.sampling_frequency || 1000

        # get_data now returns a hash of {channel_index => [samples]}
        block_data = device.get_data(sampling_frequency: device_frequency)
        next if block_data.nil?

        timestamp = Time.now

        # Create the SamplingData object. The data needs to be mapped from
        # channel_index to channel_id.
        data_for_queue = {}
        block_data.each do |channel_index, samples|
          # Find the channel_id corresponding to this device and channel_index
          channel = device.channels.find { |c| c.channel_index == channel_index }
          if channel
            data_for_queue[channel.channel_id] = samples
          else
            @logger.warn("Data received for unmapped channel index #{channel_index} on device #{device.device_id}")
          end
        end

        sampling_data_object = SamplingData.new(timestamp, data_for_queue)
        @sampling_queue.push(sampling_data_object)

        @logger.info("Queued new 1-second data block for device #{device.device_id}.")
      end

      # Poll the hardware buffer at a reasonable rate (e.g., 10 times a second)
      sleep(0.1)
    end
  rescue => e
    @logger.error("Error in processing loop: #{e.message}")
    @logger.error(e.backtrace.join("\n"))
  end
end

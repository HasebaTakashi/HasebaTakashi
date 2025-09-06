# frozen_string_literal: true

require 'thread'

# SamplingManager: Core Logic / Orchestrator
# This class manages all sampling devices and channels, runs the data
# acquisition and data blocking loops, and pushes completed data blocks
# to the sampling queue.
class SamplingManager
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
        # As per spec, 1 pulse channel per device. Let's assign it a special ID.
        # Let's use a convention: device_id * 1000 + 999
        pulse_channel_id = (device.device_id * 1000) + 999
        pulse_setting = ChannelSetting.new(pulse_channel_id, "Pulse-CH-#{device.device_id}", device.device_id, 16, 1000, 1, 0, 0, 0, "Pulse")
        pulse_channel = SamplingChannel.new(pulse_setting)
        @channels[pulse_channel_id] = pulse_channel
        device.register_channel(pulse_channel)
    end

    @logger.info("SamplingManager initialized with #{@devices.count} devices and #{@channels.count} channels.")
  end

  def start
    @mutex.synchronize do
      return if @running
      @running = true
    end

    @logger.info("Starting SamplingManager...")
    @devices.each(&:open)
    @devices.each(&:start_sampling)

    @polling_thread = Thread.new { polling_loop }
    @blocking_thread = Thread.new { blocking_loop }
    @logger.info("SamplingManager started.")
  end

  def stop
    @mutex.synchronize do
      return unless @running
      @running = false
    end

    @logger.info("Stopping SamplingManager...")
    @polling_thread&.join
    @blocking_thread&.join
    @devices.each(&:stop_sampling)
    @devices.each(&:close)
    @logger.info("SamplingManager stopped.")
  end

  private

  def polling_loop
    while @running
      @devices.each do |device|
        next unless device.check_data

        data = device.get_data
        next if data.nil?

        # Distribute data to channels. The `get_data` method returns an array
        # where the index corresponds to the hardware channel index (0-15 for analog, 16 for pulse).
        data.each_with_index do |sample, index|
          # Find the channel registered to this device that has the matching hardware index.
          target_channel = device.channels.find { |c| c.channel_index == index }
          if target_channel
            target_channel.add_data(sample)
          else
            @logger.warn("Received data for unconfigured channel index #{index} on device #{device.device_id}")
          end
        end
      end
      # This sleep is critical. It determines the polling rate.
      # This should ideally match the hardware's data production rate.
      # For a 1000Hz sampling frequency, we need to poll at least that fast.
      sleep(0.0005)
    end
  rescue => e
    @logger.error("Error in polling loop: #{e.message}")
    @logger.error(e.backtrace.join("\n"))
  end

  def blocking_loop
    while @running
      sleep(1)

      timestamp = Time.now
      data_block = SamplingData.new(timestamp)

      @channels.each do |id, channel|
        data_block.data[id] = channel.flush
      end

      @sampling_queue.push(data_block)
      @logger.debug("Created and queued new data block for timestamp #{timestamp}.")
    end
  rescue => e
    @logger.error("Error in blocking loop: #{e.message}")
    @logger.error(e.backtrace.join("\n"))
  end
end

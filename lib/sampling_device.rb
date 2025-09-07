# frozen_string_literal: true

require_relative 'vmonitor2_board'
require_relative 'dummy_device'

# SamplingDevice: A generic device abstraction layer.
# It instantiates a specific board driver based on device settings
# and provides a unified, explicit interface to it.
class SamplingDevice
  attr_reader :device_id, :channels

  # @param device_setting [DeviceSetting] The configuration for this device.
  def initialize(device_setting)
    @device_id = device_setting.device_id
    @channels = []

    # Use the 'device_type' field from settings to determine which board to use.
    device_type = device_setting.respond_to?(:device_type) ? device_setting.device_type : 'VMonitor2'

    case device_type
    when 'VMonitor2'
      @board = VMonitor2Board.new
    when 'Dummy'
      @board = DummyDevice.new
    else
      raise "Unknown device type: #{device_type}"
    end
  end

  # Register a sampling channel with this device
  def register_channel(channel)
    @channels << channel
  end

  # --- Public Interface ---
  # These methods provide a stable, abstract interface for controlling the device.

  def open
    @board.open
  end

  def close
    @board.close
  end

  def start_sampling
    @board.start_sampling
  end

  def stop_sampling
    @board.stop_sampling
  end

  # Checks if a 1-second data block is ready.
  def check_data
    # This is where we map the abstract `check_data` call to the specific
    # implementation of the board.
    @board.check_data
  end

  # Gets a 1-second data block from the hardware.
  # @param sampling_frequency [Integer] The number of samples to retrieve per channel.
  def get_data(sampling_frequency:)
    @board.get_data(sampling_frequency: sampling_frequency)
  end

  def get_terminal_ad(channel)
    @board.get_terminal_ad(channel)
  end

  def get_gain(channel)
    @board.get_gain(channel)
  end

  def set_gain(channel, gain)
    @board.set_gain(channel, gain)
  end

  def get_digital_in(channel)
    @board.get_digital_in(channel)
  end

  def get_digital_out(channel)
    @board.get_digital_out(channel)
  end

  def set_digital_out(channel, value)
    @board.set_digital_out(channel, value)
  end
end

# frozen_string_literal: true

require_relative 'vmonitor2_board'

# SamplingDevice: A generic device abstraction layer.
# It instantiates a specific board driver based on device settings
# and provides a unified interface to it.
class SamplingDevice
  attr_reader :device_id, :channels

  # @param device_setting [DeviceSetting] The configuration for this device.
  def initialize(device_setting)
    @device_id = device_setting.device_id
    @channels = []

    # In a real application, device_setting would have a 'type' field.
    # For now, we hardcode to VMonitor2Board.
    # Note: I added 'device_type' to the JSON earlier. Let's pretend to use it.
    device_type = device_setting.respond_to?(:device_type) ? device_setting.device_type : 'VMonitor2'

    case device_type
    when 'VMonitor2'
      @board = VMonitor2Board.new
    else
      raise "Unknown device type: #{device_type}"
    end
  end

  # Register a sampling channel with this device
  def register_channel(channel)
    @channels << channel
  end

  # Delegate hardware-specific calls to the wrapped board instance
  def method_missing(method_name, *args, &block)
    if @board.respond_to?(method_name)
      @board.send(method_name, *args, &block)
    else
      super
    end
  end

  def respond_to_missing?(method_name, include_private = false)
    @board.respond_to?(method_name) || super
  end
end

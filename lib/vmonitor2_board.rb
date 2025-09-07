# frozen_string_literal: true

require 'fiddle/import'
require_relative 'vmonitor2_driver'

# VMonitor2Board: Specific Device Hardware Abstraction (Corrected)
# Wraps the VMonitor2Driver, handling pointer manipulation for getting
# 1-second data blocks.
class VMonitor2Board
  ANALOG_CHANNELS = 16
  PULSE_CHANNELS = 1

  def open
    VMonitor2Driver.VM2_Open
  end

  def close
    VMonitor2Driver.VM2_Close
  end

  def initialize_device
    VMonitor2Driver.VM2_Initialize
  end

  def start_sampling
    VMonitor2Driver.VM2_StartSampling
  end

  def stop_sampling
    VMonitor2Driver.VM2_StopSampling
  end

  def check_data
    exist_ptr = Fiddle::Pointer.malloc(Fiddle::SIZEOF_INT)
    VMonitor2Driver.VM2_CheckBuffer(exist_ptr)
    exist_ptr.to_s(Fiddle::SIZEOF_INT).unpack1('i') == 1
  end

  # Gets a 1-second data block from the hardware.
  # @param sampling_frequency [Integer] The number of samples to retrieve per channel.
  # @return [Hash, nil] A hash mapping channel index (0-16) to an array of samples,
  #                     or nil if the call fails.
  def get_data(sampling_frequency:)
    # Allocate memory for 1s of data for each channel
    analog_pointers = Array.new(ANALOG_CHANNELS) do
      Fiddle::Pointer.malloc(Fiddle::SIZEOF_SHORT * sampling_frequency)
    end
    pulse_pointer = Fiddle::Pointer.malloc(Fiddle::SIZEOF_INT * sampling_frequency)

    # Call the C function with all the pointers
    result = VMonitor2Driver.VM2_GetData(
      *analog_pointers,
      pulse_pointer
    )
    return nil if result != 0 # Check for success

    # Unpack the data from memory
    data_hash = {}
    analog_pointers.each_with_index do |ptr, i|
      data_hash[i] = ptr.to_s(Fiddle::SIZEOF_SHORT * sampling_frequency).unpack("s<#{sampling_frequency}")
    end

    pulse_channel_index = ANALOG_CHANNELS # 16
    data_hash[pulse_channel_index] = pulse_pointer.to_s(Fiddle::SIZEOF_INT * sampling_frequency).unpack("l<#{sampling_frequency}")

    data_hash
  end

  # --- Other methods remain the same as they deal with single values ---
  def set_gain(channel, gain)
    VMonitor2Driver.VM2_SetGain(channel, gain)
  end

  def get_gain(channel)
    gain_ptr = Fiddle::Pointer.malloc(Fiddle::SIZEOF_INT)
    result = VMonitor2Driver.VM2_GetGain(channel, gain_ptr)
    return nil if result != 0
    gain_ptr.to_s(Fiddle::SIZEOF_INT).unpack1('I')
  end

  def get_terminal_ad(channel)
    ad_ptr = Fiddle::Pointer.malloc(Fiddle::SIZEOF_INT)
    result = VMonitor2Driver.VM2_GetTerminalAd(channel, ad_ptr)
    return nil if result != 0
    ad_ptr.to_s(Fiddle::SIZEOF_INT).unpack1('I')
  end

  def get_digital_in(channel)
    di_ptr = Fiddle::Pointer.malloc(Fiddle::SIZEOF_INT)
    result = VMonitor2Driver.VM2_GetDigitalIn(channel, di_ptr)
    return nil if result != 0
    di_ptr.to_s(Fiddle::SIZEOF_INT).unpack1('I')
  end

  def get_digital_out(channel)
    do_ptr = Fiddle::Pointer.malloc(Fiddle::SIZEOF_INT)
    result = VMonitor2Driver.VM2_GetDigitalOut(channel, do_ptr)
    return nil if result != 0
    do_ptr.to_s(Fiddle::SIZEOF_INT).unpack1('I')
  end

  def set_digital_out(channel, value)
    VMonitor2Driver.VM2_SetDigitalOut(channel, value)
  end
end

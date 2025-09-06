# frozen_string_literal: true

require 'fiddle/import'
require_relative 'vmonitor2_driver'

# VMonitor2Board: Specific Device Hardware Abstraction
# This class wraps the VMonitor2Driver module to provide an object-oriented
# interface to the VMonitor2 hardware. It handles the low-level details
# of pointer manipulation for Fiddle.
class VMonitor2Board
  ANALOG_CHANNELS = 16
  PULSE_CHANNELS = 1
  TOTAL_CHANNELS = ANALOG_CHANNELS + PULSE_CHANNELS

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

  # Checks if there is data in the hardware buffer.
  # @return [Boolean] true if data exists, false otherwise.
  def check_data
    exist_ptr = Fiddle::Pointer.malloc(Fiddle::SIZEOF_INT)
    VMonitor2Driver.VM2_CheckBuffer(exist_ptr)
    exist_ptr.to_s(Fiddle::SIZEOF_INT).unpack1('i') == 1
  end

  # Gets the latest data block from the hardware.
  # @return [Array, nil] An array containing 16 analog values and 1 pulse value,
  #                      or nil if the call fails.
  def get_data
    # Allocate memory for the data pointers
    analog_data_ptr = Fiddle::Pointer.malloc(Fiddle::SIZEOF_SHORT * ANALOG_CHANNELS)
    pulse_data_ptr = Fiddle::Pointer.malloc(Fiddle::SIZEOF_INT)

    # The VM2_GetData function expects an array of pointers, but since Fiddle
    # can't handle arrays of pointers directly in a simple way, we pass each
    # pointer individually. We create an array of pointers to pass to the C function.
    # The C function expects short* pData1, short* pData2, ...
    # We can simulate this by creating pointers to each element of our allocated block.

    # A simpler interpretation that often works is that the C function just wants
    # a pointer to a contiguous block of memory. Let's try that first.
    # The driver seems to expect 16 separate pointers, not one block.
    # Let's create 16 pointers to our single block of memory.
    ptrs = ANALOG_CHANNELS.times.map do |i|
      analog_data_ptr + (i * Fiddle::SIZEOF_SHORT)
    end

    # Call the C function
    result = VMonitor2Driver.VM2_GetData(
      *ptrs,
      pulse_data_ptr
    )
    return nil if result != 0 # Check for success

    # Unpack the data from memory
    analog_values = analog_data_ptr.to_s(Fiddle::SIZEOF_SHORT * ANALOG_CHANNELS).unpack("s<#{ANALOG_CHANNELS}")
    pulse_value = pulse_data_ptr.to_s(Fiddle::SIZEOF_INT).unpack1('l')

    analog_values + [pulse_value]
  end

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

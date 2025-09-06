# frozen_string_literal: true

require 'thread'

# SamplingChannel: A thread-safe buffer for a single channel's data.
# It accumulates samples and provides a way to flush them periodically.
class SamplingChannel
  attr_reader :channel_id, :device_id, :sampling_frequency, :channel_index

  # @param channel_setting [ChannelSetting] The configuration for this channel.
  def initialize(channel_setting)
    @channel_id = channel_setting.channel_id
    @device_id = channel_setting.device_id
    @sampling_frequency = channel_setting.sampling_frequency
    @channel_index = channel_setting.channel_index

    @buffer = []
    @mutex = Mutex.new
  end

  # Adds a single data sample to the buffer.
  # @param sample [Numeric] The data sample to add.
  def add_data(sample)
    @mutex.synchronize do
      @buffer << sample
    end
  end

  # Returns all data from the buffer and clears it.
  # @return [Array<Numeric>] The data that was in the buffer.
  def flush
    @mutex.synchronize do
      data_to_return = @buffer.dup
      @buffer.clear
      data_to_return
    end
  end

  # Returns the number of samples currently in the buffer.
  def buffer_size
    @mutex.synchronize do
      @buffer.size
    end
  end
end

# frozen_string_literal: true

require 'thread'

# SamplingQueue: A thread-safe, fixed-size circular buffer (queue).
# It stores SamplingData objects. When the queue is full, the oldest
# element is overwritten.
class SamplingQueue
  attr_reader :max_size

  # @param max_size [Integer] The maximum number of items in the queue.
  def initialize(max_size)
    @max_size = max_size
    @queue = []
    @mutex = Mutex.new
  end

  # Adds an item to the queue. If the queue is full, the oldest item is removed.
  # @param item [SamplingData] The item to add.
  def push(item)
    @mutex.synchronize do
      @queue.shift if @queue.size >= @max_size
      @queue.push(item)
    end
  end
  alias << push

  # Retrieves the latest item added to the queue without removing it.
  # @return [SamplingData, nil] The latest item, or nil if the queue is empty.
  def latest
    @mutex.synchronize do
      @queue.last
    end
  end

  # Returns all data blocks with a timestamp after the given time.
  # @param timestamp [Time] The timestamp to compare against.
  # @return [Array<SamplingData>] A new array with the filtered data.
  def get_data_since(timestamp)
    @mutex.synchronize do
      @queue.select { |d| d.timestamp > timestamp }
    end
  end

  # Clears all data from the queue.
  def clear
    @mutex.synchronize do
      @queue.clear
    end
  end

  # Clears data older than the specified timestamp.
  # @param timestamp [Time] The timestamp to compare against.
  def clear_older_than(timestamp)
    @mutex.synchronize do
      @queue.reject! { |d| d.timestamp <= timestamp }
    end
  end

  # Returns the number of items in the queue.
  def size
    @mutex.synchronize do
      @queue.size
    end
  end
end

# frozen_string_literal: true

# SamplingData: Represents a block of data for all channels over a 1-second interval.
#
# @!attribute timestamp
#   @return [Time] The timestamp for the beginning of the data block.
# @!attribute data
#   @return [Hash{Integer => Array<Numeric>}] A hash where keys are channel IDs
#     and values are arrays of samples for that second.
SamplingData = Struct.new(
  :timestamp,
  :data
) do
  def initialize(timestamp, data = {})
    super(timestamp, data)
  end
end

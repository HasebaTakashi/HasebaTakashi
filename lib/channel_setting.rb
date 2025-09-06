# frozen_string_literal: true

# ChannelSetting: Per-Channel Configuration Data
ChannelSetting = Struct.new(
  :channel_id,
  :channel_name,
  :device_id,
  :channel_index,
  :sampling_frequency,
  :gain,
  :max_range,
  :min_range,
  :resolution,
  :input_type
)

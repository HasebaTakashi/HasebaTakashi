# frozen_string_literal: true

# DeviceSetting: Per-Device Configuration Data
DeviceSetting = Struct.new(
  :device_id,
  :device_name,
  :channel_no,
  :enable_device,
  :device_type
)

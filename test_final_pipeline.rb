# frozen_string_literal: true

# Load all libs
Dir["#{File.dirname(__FILE__)}/lib/*.rb"].each { |file| require_relative file }

puts '--- Testing Final Corrected Data Acquisition Pipeline ---'

# 1. Initialize all components
puts 'Initializing components...'
app_settings = AppSettingManager.new('config/app_setting.json')
device_settings = DeviceSettingManager.new('config/device_setting.json')
channel_settings = ChannelSettingManager.new('config/channel_setting.json')

logger = AppLogger.instance
logger.configure(log_path: app_settings.log_file_path, level: 'INFO')

queue = SamplingQueue.new(app_settings.sampling_queue_size)
manager = SamplingManager.new(app_settings, device_settings, channel_settings, queue)

# 2. Run the manager for a few seconds
puts 'Starting SamplingManager for 5 seconds...'
manager.start
sleep(5)
puts 'Stopping SamplingManager...'
manager.stop
puts 'Manager stopped.'

# 3. Inspect the queue
puts "\n--- Inspecting SamplingQueue ---"
puts "Queue size: #{queue.size}"

unless queue.size > 0
  puts "ERROR: Queue is empty. No data blocks were generated."
  exit 1
end
puts "Test passed: Queue contains data."

latest_data = queue.latest
puts "Latest data block timestamp: #{latest_data.timestamp}"
puts "Number of channels in block: #{latest_data.data.keys.size}"

# 4. Verify data integrity for all channels in the latest block
puts "\n--- Verifying Data Integrity ---"
all_channels_ok = true

channel_settings.channel_settings.each do |cs|
  channel_data = latest_data.data[cs.channel_id]

  if !channel_data
    puts "ERROR: No data found for channel ##{cs.channel_id}"
    all_channels_ok = false
    next
  end

  if channel_data.size != cs.sampling_frequency
    puts "ERROR: For channel ##{cs.channel_id}, expected #{cs.sampling_frequency} samples, but found #{channel_data.size}."
    all_channels_ok = false
    next
  end

  # Verify the data pattern
  errors = 0
  channel_data.each_with_index do |sample, i|
    expected_sample = (cs.channel_index * 100) + i
    if sample != expected_sample
      errors += 1
    end
  end

  if errors.zero?
    puts "Test passed: Data integrity for channel ##{cs.channel_id} is OK."
  else
    puts "ERROR: For channel ##{cs.channel_id}, found #{errors} mismatches in data pattern."
    all_channels_ok = false
  end
end

# Verify pulse channel (hardcoded ID based on SamplingManager logic)
pulse_channel_id = (device_settings.device_settings.first.device_id * 1000) + 999
pulse_data = latest_data.data[pulse_channel_id]
if pulse_data
  errors = 0
  pulse_data.each_with_index do |sample, i|
    if sample != i
      errors += 1
    end
  end
  if errors.zero?
    puts "Test passed: Data integrity for pulse channel ##{pulse_channel_id} is OK."
  else
    puts "ERROR: For pulse channel ##{pulse_channel_id}, found #{errors} mismatches."
    all_channels_ok = false
  end
else
  puts "ERROR: No data found for pulse channel ##{pulse_channel_id}"
  all_channels_ok = false
end


if all_channels_ok
  puts "\n--- Final Pipeline Test Complete: SUCCESS ---"
else
  puts "\n--- Final Pipeline Test Complete: FAILED ---"
  exit 1
end

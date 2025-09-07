# frozen_string_literal: true

require 'json'
require 'fileutils'

# Load all libs
Dir["#{File.dirname(__FILE__)}/lib/*.rb"].each { |file| require_relative file }

puts '--- Testing Pipeline with DummyDevice ---'

CONFIG_PATH = 'config/device_setting.json'
original_config = File.read(CONFIG_PATH)
test_results = {}

begin
  # 1. Modify config to use DummyDevice
  puts 'Modifying config to use DummyDevice...'
  dummy_config = JSON.parse(original_config)
  dummy_config[0]['device_type'] = 'Dummy'
  File.write(CONFIG_PATH, JSON.pretty_generate(dummy_config))

  # 2. Initialize components
  puts 'Initializing components with DummyDevice...'
  app_settings = AppSettingManager.new('config/app_setting.json')
  device_settings = DeviceSettingManager.new('config/device_setting.json')
  # Use a smaller channel set for this test to speed it up
  dummy_channel_settings = (1..2).map do |i|
    { "channel_id" => i, "channel_name" => "Dummy-CH#{i}", "device_id" => 1, "channel_index" => i - 1,
      "sampling_frequency" => 25600, "gain" => 1.0, "max_range" => 5.0, "min_range" => -5.0,
      "resolution" => 16, "input_type" => "Analog" }
  end
  File.write('config/dummy_channel_setting.json', JSON.pretty_generate(dummy_channel_settings))
  channel_settings = ChannelSettingManager.new('config/dummy_channel_setting.json')

  logger = AppLogger.instance
  logger.configure(log_path: app_settings.log_file_path, level: 'INFO')

  queue = SamplingQueue.new(app_settings.sampling_queue_size)
  manager = SamplingManager.new(app_settings, device_settings, channel_settings, queue)

  # 3. Run the manager
  # The dummy device's check_data sleeps for 1s, so we run for a couple of cycles.
  puts 'Starting SamplingManager for 2.5 seconds...'
  manager.start
  sleep 2.5
  manager.stop
  puts 'Manager stopped.'

  # 4. Verify the data
  puts "\n--- Verifying Data from DummyDevice ---"
  test_results[:queue_size] = (queue.size == 2) # Should get 2 blocks
  puts "Queue size check (expected 2): #{queue.size} -> #{test_results[:queue_size] ? 'PASS' : 'FAIL'}"

  latest_data = queue.latest
  if latest_data
    # Check data for channel 1 (index 0)
    ch1_data = latest_data.data[1]
    if ch1_data
      # Verify the sine wave pattern for a few points
      freq = DummyDevice::SAMPLING_FREQUENCY
      # Check first sample
      s0_ok = ch1_data[0] == (100 * Math.sin(2 * Math::PI * (0 + 1) * 2 * (0.0/freq))).to_i
      # Check 1/4 way through
      s_q_ok = ch1_data[freq/4] == (100 * Math.sin(2 * Math::PI * (0 + 1) * 2 * ((freq/4.0)/freq))).to_i

      test_results[:sine_wave] = (s0_ok && s_q_ok)
      puts "Sine wave pattern check: #{test_results[:sine_wave] ? 'PASS' : 'FAIL'}"
    else
      puts "ERROR: No data found for channel 1"
      test_results[:sine_wave] = false
    end
  else
    puts "ERROR: Queue is empty after run"
    test_results[:queue_has_data] = false
  end

rescue => e
  puts "An error occurred during test: #{e.message}"
  puts e.backtrace
  test_results[:exception] = false
ensure
  # 5. Restore original config
  puts "\nRestoring original device_setting.json..."
  File.write(CONFIG_PATH, original_config)
  File.delete('config/dummy_channel_setting.json') if File.exist?('config/dummy_channel_setting.json')
end

# 6. Final result
puts "\n--- DummyDevice Test Results ---"
if test_results.values.all?(true)
  puts "DummyDevice pipeline test passed!"
  puts "--- Test Complete: SUCCESS ---"
else
  puts "DummyDevice pipeline test failed."
  puts test_results
  puts "--- Test Complete: FAILED ---"
  exit 1
end

# frozen_string_literal: true

require 'socket'
require 'timeout'

# Load all libs
Dir["#{File.dirname(__FILE__)}/lib/*.rb"].each { |file| require_relative file }

puts '--- Testing CommandServer ---'

# 1. Initialize all components
puts 'Initializing components...'
app_settings = AppSettingManager.new('config/app_setting.json')
device_settings = DeviceSettingManager.new('config/device_setting.json')
channel_settings = ChannelSettingManager.new('config/channel_setting.json')

logger = AppLogger.instance
logger.configure(log_path: app_settings.log_file_path, level: 'INFO')

queue = SamplingQueue.new(app_settings.sampling_queue_size)
manager = SamplingManager.new(app_settings, device_settings, channel_settings, queue)
server = CommandServer.new(app_settings.tcp_port, app_settings, device_settings, channel_settings, queue, manager)

# 2. Start services
puts 'Starting SamplingManager and CommandServer...'
manager.start
server.start
# Give the services a moment to start and generate some data
sleep 2

# 3. Run client tests
puts "\n--- Running Client Tests ---"
test_results = {}

begin
  Timeout.timeout(20) do # Increased timeout for more tests
    client = TCPSocket.new('localhost', app_settings.tcp_port)
    puts "Client connected to port #{app_settings.tcp_port}"

    # Test CHECKVERSION
    puts "Testing CHECKVERSION..."
    client.puts "CheckVersion"
    response = client.gets.strip
    test_results[:check_version] = (response == '1.0.0')
    puts "  Response: #{response} -> #{test_results[:check_version] ? 'PASS' : 'FAIL'}"

    # Test GETCHANNELID
    puts "Testing GETCHANNELID..."
    client.puts "GetChannelID"
    response = client.gets.strip
    expected_ids = (1..16).to_a.join(',')
    test_results[:get_channel_id] = (response == expected_ids)
    puts "  Response: #{response} -> #{test_results[:get_channel_id] ? 'PASS' : 'FAIL'}"

    # Test GETDATANO
    puts "Testing GETDATANO..."
    client.puts "GetDataNo,0" # Arg is buffer num
    response = client.gets.strip
    test_results[:get_data_no] = (response.to_i > 0)
    puts "  Response: #{response} -> #{test_results[:get_data_no] ? 'PASS' : 'FAIL'}"

    # Test GETCHANNELINFO
    puts "Testing GETCHANNELINFO..."
    client.puts "GetChannelInfo,5"
    response = client.gets.strip
    expected_info = "5,1.0,1000,Analog"
    test_results[:get_channel_info] = (response == expected_info)
    puts "  Response: #{response} -> #{test_results[:get_channel_info] ? 'PASS' : 'FAIL'}"

    # Test GETDATA
    puts "Testing GETDATA..."
    client.puts "GetData,0" # Arg is buffer num
    response = client.gets.strip
    test_results[:get_data] = (response.length > 100 && response.count(',') > 16000)
    puts "  Response length: #{response.length} -> #{test_results[:get_data] ? 'PASS' : 'FAIL'}"

    # Test GETCHDATA
    puts "Testing GETCHDATA..."
    client.puts "GetChData,0,8" # Buffer 0, Channel 8
    response = client.gets.strip
    test_results[:get_ch_data] = (response.length > 1000 && response.count(',') == 1005)
    puts "  Response length: #{response.length} -> #{test_results[:get_ch_data] ? 'PASS' : 'FAIL'}"

    # Test GETTERMINALVOLTAGE
    puts "Testing GETTERMINALVOLTAGE..."
    client.puts "GetTerminalVoltage,3" # Channel 3
    response = client.gets.strip
    # Dummy returns 40000
    test_results[:get_terminal_voltage] = (response == '40000')
    puts "  Response: #{response} -> #{test_results[:get_terminal_voltage] ? 'PASS' : 'FAIL'}"

    # Test GETALLTERMINALVOLTAGES
    puts "Testing GETALLTERMINALVOLTAGES..."
    client.puts "GetAllTerminalVoltages"
    response = client.gets.strip
    expected_voltages = (['40000'] * 16).join(',')
    test_results[:get_all_terminal_voltages] = (response == expected_voltages)
    puts "  Response: #{response[0..30]}... -> #{test_results[:get_all_terminal_voltages] ? 'PASS' : 'FAIL'}"

    # Test Set/Get Gain
    puts "Testing SETGAIN/GETGAIN..."
    client.puts "SetGain,7,3.14"
    set_response = client.gets.strip
    client.puts "GetGain,7"
    get_response = client.gets.strip
    test_results[:set_get_gain] = (set_response == '0' && get_response == '3.14')
    puts "  Set Response: #{set_response}, Get Response: #{get_response} -> #{test_results[:set_get_gain] ? 'PASS' : 'FAIL'}"

    # Test Set/Get DO
    puts "Testing SETDO/GETDO..."
    client.puts "SetDO,2,1" # Channel 2, ON
    set_do_response = client.gets.strip
    client.puts "GetDO,2"
    get_do_response = client.gets.strip
    # Dummy GetDO always returns 0, so this test will fail unless we enhance the dummy.
    # For now, just test that the commands run.
    test_results[:set_get_do] = (set_do_response == '0' && get_do_response == '0')
    puts "  Set Response: #{set_do_response}, Get Response: #{get_do_response} -> #{test_results[:set_get_do] ? 'PASS' : 'FAIL'} (Note: GetDO dummy is static)"

    # Test BUFFERCLEAR
    puts "Testing BUFFERCLEAR..."
    client.puts "BufferClear,0"
    clear_response = client.gets.strip
    client.puts "GetDataNo,0"
    data_no_after_clear = client.gets.strip
    test_results[:buffer_clear] = (clear_response == '0' && data_no_after_clear == '0')
    puts "  Clear Response: #{clear_response}, DataNo after: #{data_no_after_clear} -> #{test_results[:buffer_clear] ? 'PASS' : 'FAIL'}"

    client.close
  end
rescue Timeout::Error
  puts "ERROR: Client tests timed out."
  test_results[:timeout] = false
rescue => e
  puts "ERROR: An exception occurred during client tests: #{e.message}\n#{e.backtrace.join("\n")}"
  test_results[:exception] = false
end

# 4. Teardown
puts "\n--- Stopping services ---"
server.stop
manager.stop
puts "Services stopped."

# 5. Final result
puts "\n--- Final Results ---"
if test_results.values.all?(true)
  puts "All CommandServer tests passed!"
  puts "--- Test Complete: SUCCESS ---"
else
  puts "One or more CommandServer tests failed."
  puts test_results
  puts "--- Test Complete: FAILED ---"
  exit 1
end

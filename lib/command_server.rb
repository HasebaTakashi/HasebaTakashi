# frozen_string_literal: true

require 'socket'
require 'thread'
require 'time'

# CommandServer: TCP/IP Listener
class CommandServer
  def initialize(port, app_settings, device_settings, channel_settings, queue, sampling_manager)
    @port = port
    @app_settings = app_settings
    @device_settings = device_settings
    @channel_settings = channel_settings
    @queue = queue
    @sampling_manager = sampling_manager
    @server = nil
    @clients = []
    @mutex = Mutex.new
    @logger = AppLogger.instance
  end

  def start
    return if @server

    @server = TCPServer.new(@port)
    @logger.info("CommandServer started on port #{@port}")

    @main_thread = Thread.new do
      loop do
        client_socket = @server.accept
        @mutex.synchronize { @clients << client_socket }

        Thread.new { handle_client(client_socket) }
      end
    end
  end

  def stop
    @main_thread&.kill

    @mutex.synchronize do
      @clients.each { |c| c.close unless c.closed? }
      @clients.clear
    end

    @server&.close
    @server = nil
    @logger.info("CommandServer stopped.")
  end

  private

  def handle_client(client)
    client_addr = client.peeraddr.join(':')
    @logger.info("Client connected: #{client_addr}")

    loop do
      request = client.gets&.strip
      break if request.nil? # Client disconnected

      @logger.debug("Received from #{client_addr}: #{request}")

      command, *args = request.split(',')
      response = process_command(command, args)

      client.puts(response)
      @logger.debug("Sent to #{client_addr}: #{response}")
    end
  rescue => e
    @logger.error("Error handling client #{client_addr}: #{e.message}")
  ensure
    @logger.info("Client disconnected: #{client_addr}")
    @mutex.synchronize { @clients.delete(client) }
    client.close unless client.closed?
  end

  def process_command(command, args)
    # The spec uses case-sensitive commands, but it's safer to be case-insensitive.
    case command.upcase
    when 'CHECKVERSION'
      '1.0.0'

    when 'GETCHANNELID'
      @channel_settings.channel_settings.map(&:channel_id).join(',')

    when 'GETCHANNELINFO'
      ch_id = args[0].to_i
      info = @channel_settings.find_by_id(ch_id)
      if info
        [info.channel_id, info.gain, info.sampling_frequency, info.input_type].join(',')
      else
        '-1'
      end

    when 'CHECKDATA'
      @queue.size > 0 ? '1' : '0'

    when 'GETDATA'
      data_block = @queue.latest
      if data_block
        all_values = @channel_settings.channel_settings.map do |cs|
          data_block.data[cs.channel_id] || []
        end.flatten
        ts = data_block.timestamp
        timestamp_str = [ts.year, ts.month, ts.day, ts.hour, ts.min, ts.sec].join(',')
        (all_values + [timestamp_str]).join(',')
      else
        ''
      end

    when 'GETDATANO'
      @queue.size.to_s

    when 'STARTSAMPLING'
      @queue.clear
      '0'

    when 'BUFFERCLEAR'
      @queue.clear
      '0'

    when 'SETDATATIME'
      unix_time = args[1].to_i
      @queue.clear_older_than(Time.at(unix_time))
      '0'

    when 'SETDATATIME2'
      time_args = args[1..6].map(&:to_i)
      @queue.clear_older_than(Time.new(*time_args))
      '0'

    when 'GETTERMINALVOLTAGE'
      ch_id = args[0].to_i
      ch_setting = @channel_settings.find_by_id(ch_id)
      if ch_setting && @sampling_manager.devices.first
        voltage = @sampling_manager.devices.first.get_terminal_ad(ch_setting.channel_index)
        voltage ? voltage.to_s : '-1'
      else
        '-1'
      end

    when 'GETALLTERMINALVOLTAGES'
      device = @sampling_manager.devices.first
      if device
        voltages = @channel_settings.channel_settings.map do |cs|
          device.get_terminal_ad(cs.channel_index) || -1
        end
        voltages.join(',')
      else
        '-1'
      end

    when 'GETGAIN'
      ch_id = args[0].to_i
      ch_setting = @channel_settings.find_by_id(ch_id)
      ch_setting ? ch_setting.gain.to_s : '-1'

    when 'SETGAIN'
      ch_id = args[0].to_i
      new_gain = args[1].to_f
      ch_setting = @channel_settings.find_by_id(ch_id)
      if ch_setting
        ch_setting.gain = new_gain
        '0'
      else
        '-1'
      end

    when 'GETDI'
      ch_num = args[0].to_i
      val = @sampling_manager.devices.first&.get_digital_in(ch_num)
      val ? val.to_s : '-1'

    when 'GETDO'
      ch_num = args[0].to_i
      val = @sampling_manager.devices.first&.get_digital_out(ch_num)
      val ? val.to_s : '-1'

    when 'SETDO'
      ch_num = args[0].to_i
      value = args[1].to_i
      @sampling_manager.devices.first&.set_digital_out(ch_num, value)
      '0'

    when 'GETCHDATA'
      ch_id = args[1].to_i
      data_block = @queue.latest
      if data_block && data_block.data.key?(ch_id)
        values = data_block.data[ch_id]
        ts = data_block.timestamp
        timestamp_str = [ts.year, ts.month, ts.day, ts.hour, ts.min, ts.sec].join(',')
        (values + [timestamp_str]).join(',')
      else
        ''
      end

    else
      '-1'
    end
  rescue => e
    @logger.error("Error processing command '#{command}': #{e.message}\n#{e.backtrace.join("\n")}")
    '-1'
  end
end

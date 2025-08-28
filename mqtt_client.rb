# frozen_string_literal: true

require 'mqtt'

# MQTT通信
class MqttClient
  def initialize(connection, topics)
    @client = MQTT::Client.new
    p connection
    @client.host = connection
    @client.port = 1883
    @topics = topics
  end

  def connect
    @client.connect
    @topics.each do |topic|
      p topic
      @client.subscribe(topic)
    end
    mqtt_connect_check
  end

  def recieve_start(&block)
    # デバイスからのデータ受信を開始する
    puts 'MQTT Subscribe Start'

    Thread.new do
      # トピック受信
      @client.get do |topic, message|
        # トピック名確認
        p "recieve Topic: #{topic}"
        p "Message: #{message}"
        block.call(topic, message)
      end
    end
  end

  def disconnect
    @client.disconnect
  end

  def send_data(topic, message)
    @client.publish(topic, message)
  end

  private

  def mqtt_connect_check
    Thread.new do
      loop do
        sleep 10
        if @client.connected?
          puts('MQTT Connected')
        else
          puts('MQTT Disconnected')
          connect
        end
      end
    end
  end
end

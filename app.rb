require 'sinatra'
require 'sinatra-websocket'
require 'mqtt'
require 'json'
require 'thread'

# SinatraのサーバーをThinに設定（WebSocketに必要）
set :server, 'thin'
# 接続中のWebSocketクライアントを格納する配列
set :sockets, []
# MQTTクライアントのインスタンスを格納
set :mqtt_client, nil
# MQTTメッセージを処理するためのキュー
set :mqtt_queue, Queue.new

# MQTTクライアントのセットアップとメッセージ受信のためのスレッドを開始
def setup_mqtt_client
  Thread.new do
    begin
      client = MQTT::Client.connect(
        host: 'localhost', # MQTTブローカーのホスト
        port: 1883
      )
      settings.mqtt_client = client
      puts "MQTT client connected."

      # 指定トピックを購読
      client.subscribe('/data/display', '/data/config')
      puts "Subscribed to /data/display and /data/config"

      # メッセージ受信ループ
      client.get do |topic, message|
        puts "Received MQTT message on topic '#{topic}': #{message}"
        # 受信したメッセージをキューに追加
        settings.mqtt_queue << { topic: topic, message: message }.to_json
      end
    rescue MQTT::Exception => e
      puts "MQTT Error: #{e.message}"
      puts "Retrying MQTT connection in 5 seconds..."
      sleep 5
      retry
    end
  end
end

# WebSocketにメッセージを送信するためのスレッドを開始
def start_websocket_sender
  Thread.new do
    loop do
      # キューからメッセージを取り出し、全クライアントに送信
      message_to_send = settings.mqtt_queue.pop
      settings.sockets.each do |s|
        s.send(message_to_send)
        puts "Sent to websocket client: #{message_to_send}"
      end
    end
  end
end

# アプリケーション設定
configure do
  setup_mqtt_client
  start_websocket_sender
end

# ルートパスへのGETリクエストを処理
get '/' do
  if request.websocket?
    request.websocket do |ws|
      # WebSocket接続が開かれたときの処理
      ws.onopen do
        puts "WebSocket connection opened."
        settings.sockets << ws
      end

      # WebSocketでメッセージを受信したときの処理
      ws.onmessage do |msg|
        puts "Received WebSocket message: #{msg}"
        begin
          # フロントエンドからのボタンイベントなどをMQTTで発行
          data = JSON.parse(msg)
          if data['event'] == 'button_press' && settings.mqtt_client
            topic = '/event/button'
            payload = { button_id: data['id'] }.to_json
            settings.mqtt_client.publish(topic, payload)
            puts "Published to MQTT topic '#{topic}': #{payload}"
          end
        rescue JSON::ParserError => e
          puts "Failed to parse WebSocket message as JSON: #{e.message}"
        end
      end

      # WebSocket接続が閉じられたときの処理
      ws.onclose do
        puts "WebSocket connection closed."
        settings.sockets.delete(ws)
      end
    end
  else
    # 通常のHTTPリクエストの場合はindex.htmlを返す
    File.read(File.join('public', 'index.html'))
  end
end